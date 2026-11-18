// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// sysfs trees of real cards, written into a folder of the test's own:
// what amdgpu (and the other drivers) show for a Polaris RX 580, a Navi
// 21 RX 6800 XT, a Navi 31 RX 7900 XTX and a Cezanne integrated GPU.

#pragma once

#include <QDir>
#include <QFile>
#include <QString>

namespace Fake
{

inline void put(const QString &path, const QByteArray &text)
{
    QDir().mkpath(path.left(path.lastIndexOf(QLatin1Char('/'))));
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(text);
}

inline QByteArray pciIds()
{
    return "# pci.ids\n"
           "1002  Advanced Micro Devices, Inc. [AMD/ATI]\n"
           "\t1638  Cezanne [Radeon Vega Series / Radeon Vega Mobile Series]\n"
           "\t67df  Ellesmere [Radeon RX 470/480/570/570X/580/580X/590]\n"
           "\t\t1002 0b37  Radeon RX 480\n"
           "\t73bf  Navi 21 [Radeon RX 6800/6800 XT / 6900 XT]\n"
           "\t744c  Navi 31 [Radeon RX 7900 XT/7900 XTX/7900 GRE/7900M]\n"
           "1022  Advanced Micro Devices, Inc. [AMD]\n"
           "\t1638  Something else\n"
           "10de  NVIDIA Corporation\n"
           "\t2206  GA102 [GeForce RTX 3080]\n"
           "8086  Intel Corporation\n"
           "\t9a49  TigerLake-LP GT2 [Iris Xe Graphics]\n";
}

struct Tree {
    QString dev;
    QString hw;
};

inline Tree card(const QString &sys, int n, const char *vendor, const char *device, const char *driver, const char *slot)
{
    const QString dev = sys + QStringLiteral("/class/drm/card%1/device").arg(n);
    put(dev + QStringLiteral("/vendor"), QByteArray("0x") + vendor + '\n');
    put(dev + QStringLiteral("/device"), QByteArray("0x") + device + '\n');
    put(dev + QStringLiteral("/uevent"), QByteArray("DRIVER=") + driver + "\nPCI_CLASS=30000\nPCI_ID=" + QByteArray(vendor).toUpper()
                                             + ':' + QByteArray(device).toUpper() + "\nPCI_SLOT_NAME=" + slot + '\n');
    // a connector of the card, which is not a card
    put(sys + QStringLiteral("/class/drm/card%1-DP-1/status").arg(n), "disconnected\n");
    return {dev, dev + QStringLiteral("/hwmon/hwmon%1").arg(n + 2)};
}

inline const char *polarisProfiles()
{
    return "NUM        MODE_NAME     SCLK_UP_HYST   SCLK_DOWN_HYST SCLK_ACTIVE_LEVEL     MCLK_UP_HYST   MCLK_DOWN_HYST MCLK_ACTIVE_LEVEL\n"
           "  0   BOOTUP_DEFAULT:        -              -              -              -              -              -\n"
           "  1 3D_FULL_SCREEN *:        0            100             30              0            100             10\n"
           "  2     POWER_SAVING:       10              0             30              -              -              -\n"
           "  3            VIDEO:        -              -              -             10             16             31\n"
           "  4               VR:        0             11             50              0            100             10\n"
           "  5          COMPUTE:        0              5             30              -              -              -\n"
           "  6           CUSTOM:        -              -              -              -              -              -\n";
}

// Polaris RX 580: one sensor, manual fan by pwm, a power limit
inline Tree polaris(const QString &sys, int n = 0, const char *slot = "0000:01:00.0")
{
    Tree t = card(sys, n, "1002", "67df", "amdgpu", slot);
    put(t.dev + "/gpu_busy_percent", "12\n");
    put(t.dev + "/mem_info_vram_total", "8589934592\n");
    put(t.dev + "/mem_info_vram_used", "524288000\n");
    put(t.dev + "/power_dpm_force_performance_level", "auto\n");
    put(t.dev + "/pp_dpm_sclk", "0: 300Mhz\n1: 600Mhz\n2: 900Mhz\n3: 1145Mhz\n4: 1215Mhz\n5: 1257Mhz\n6: 1300Mhz\n7: 1366Mhz *\n");
    put(t.dev + "/pp_dpm_mclk", "0: 300Mhz\n1: 2000Mhz *\n");
    put(t.dev + "/pp_power_profile_mode", polarisProfiles());
    put(t.hw + "/name", "amdgpu\n");
    put(t.hw + "/temp1_input", "45000\n");
    put(t.hw + "/temp1_label", "edge\n");
    put(t.hw + "/temp1_crit", "94000\n");
    put(t.hw + "/temp1_crit_hyst", "-273150\n");
    put(t.hw + "/fan1_input", "1200\n");
    put(t.hw + "/fan1_min", "0\n");
    put(t.hw + "/fan1_max", "3200\n");
    put(t.hw + "/pwm1", "80\n");
    put(t.hw + "/pwm1_enable", "2\n");
    put(t.hw + "/pwm1_min", "0\n");
    put(t.hw + "/pwm1_max", "255\n");
    put(t.hw + "/power1_average", "35000000\n");
    put(t.hw + "/power1_cap", "145000000\n");
    put(t.hw + "/power1_cap_min", "0\n");
    put(t.hw + "/power1_cap_max", "175000000\n");
    put(t.hw + "/freq1_input", "1366000000\n");
    put(t.hw + "/freq1_label", "sclk\n");
    put(t.hw + "/freq2_input", "2000000000\n");
    put(t.hw + "/freq2_label", "mclk\n");
    return t;
}

// Navi 21 RX 6800 XT: edge, junction and memory sensors, power1_input
inline Tree navi21(const QString &sys, int n = 0, const char *slot = "0000:03:00.0")
{
    Tree t = card(sys, n, "1002", "73bf", "amdgpu", slot);
    put(t.dev + "/gpu_busy_percent", "98\n");
    put(t.dev + "/mem_busy_percent", "41\n");
    put(t.dev + "/mem_info_vram_total", "17163091968\n");
    put(t.dev + "/mem_info_vram_used", "6442450944\n");
    put(t.dev + "/power_dpm_force_performance_level", "manual\n");
    put(t.dev + "/pp_dpm_sclk", "0: 500Mhz\n1: 2250Mhz *\n2: 2575Mhz\n");
    put(t.dev + "/pp_dpm_mclk", "0: 96Mhz\n1: 456Mhz\n2: 673Mhz\n3: 1000Mhz *\n");
    put(t.dev + "/pp_power_profile_mode",
        "NUM       MODE_NAME     CLOCK_TYPE(NAME) FPS MinFreqType MinActiveFreqType MinActiveFreq BoosterFreqType BoosterFreq PD_Data_limit_c PD_Data_error_coeff PD_Data_error_rate_coeff\n"
        " 0 BOOTUP_DEFAULT :\n"
        "                     0(       GFXCLK)       0       5       1       0       4     800 4587520  -65536       0\n"
        "                     1(       SOCCLK)       0       5       1       0       1       0 3276800  -65536  -65536\n"
        " 1 3D_FULL_SCREEN*:\n"
        "                     0(       GFXCLK)       0       5       1       0       4     650 4587520  -65536       0\n"
        " 2   POWER_SAVING :\n"
        " 3          VIDEO :\n"
        " 4             VR :\n"
        " 5        COMPUTE :\n"
        " 6         CUSTOM :\n");
    put(t.hw + "/temp1_input", "71000\n");
    put(t.hw + "/temp1_label", "edge\n");
    put(t.hw + "/temp1_crit", "100000\n");
    put(t.hw + "/temp2_input", "88000\n");
    put(t.hw + "/temp2_label", "junction\n");
    put(t.hw + "/temp2_crit", "110000\n");
    put(t.hw + "/temp3_input", "76000\n");
    put(t.hw + "/temp3_label", "mem\n");
    put(t.hw + "/temp3_crit", "105000\n");
    put(t.hw + "/fan1_input", "1850\n");
    put(t.hw + "/fan1_min", "0\n");
    put(t.hw + "/fan1_max", "3300\n");
    put(t.hw + "/pwm1", "153\n");
    put(t.hw + "/pwm1_enable", "1\n");
    put(t.hw + "/pwm1_min", "0\n");
    put(t.hw + "/pwm1_max", "255\n");
    put(t.hw + "/power1_input", "281000000\n");
    put(t.hw + "/power1_cap", "255000000\n");
    put(t.hw + "/power1_cap_min", "0\n");
    put(t.hw + "/power1_cap_max", "289000000\n");
    put(t.hw + "/power1_cap_default", "255000000\n");
    return t;
}

// Navi 31 RX 7900 XTX: the firmware follows a fan curve; profiles in columns
inline Tree navi31(const QString &sys, int n = 0, const char *slot = "0000:0c:00.0")
{
    Tree t = card(sys, n, "1002", "744c", "amdgpu", slot);
    put(t.dev + "/product_name", "AMD Radeon RX 7900 XTX\n");
    put(t.dev + "/gpu_busy_percent", "3\n");
    put(t.dev + "/mem_info_vram_total", "25753026560\n");
    put(t.dev + "/mem_info_vram_used", "1073741824\n");
    put(t.dev + "/power_dpm_force_performance_level", "auto\n");
    put(t.dev + "/pp_dpm_sclk", "0: 500Mhz\n1: 22Mhz *\n2: 2526Mhz\n");
    put(t.dev + "/pp_power_profile_mode",
        "                              0 BOOTUP_DEFAULT  1 3D_FULL_SCREEN* 2 POWER_SAVING    3 VIDEO           4 VR              5 COMPUTE         6 CUSTOM          7 WINDOW_3D       \n"
        "0(       GFXCLK)       0       5       1       0       4     800 4587520  -65536       0\n");
    put(t.dev + "/gpu_od/fan_ctrl/fan_curve",
        "OD_FAN_CURVE:\n0: 0C 0%\n1: 0C 0%\n2: 0C 0%\n3: 0C 0%\n4: 0C 0%\n"
        "OD_RANGE:\nFAN_CURVE(hotspot temp): 25C 100C\nFAN_CURVE(fan speed): 15% 100%\n");
    put(t.hw + "/temp1_input", "38000\n");
    put(t.hw + "/temp1_label", "edge\n");
    put(t.hw + "/temp1_crit", "100000\n");
    put(t.hw + "/temp2_input", "45000\n");
    put(t.hw + "/temp2_label", "junction\n");
    put(t.hw + "/temp2_crit", "110000\n");
    put(t.hw + "/fan1_input", "0\n");
    put(t.hw + "/fan1_min", "0\n");
    put(t.hw + "/fan1_max", "3500\n");
    put(t.hw + "/pwm1", "0\n");
    put(t.hw + "/pwm1_enable", "2\n");
    put(t.hw + "/power1_average", "21000000\n");
    put(t.hw + "/power1_cap", "339000000\n");
    put(t.hw + "/power1_cap_min", "305000000\n");
    put(t.hw + "/power1_cap_max", "402000000\n");
    put(t.hw + "/power1_cap_default", "339000000\n");
    put(t.hw + "/freq1_input", "22000000\n");
    put(t.hw + "/freq1_label", "sclk\n");
    return t;
}

// Cezanne integrated GPU: no fan, no power limit, shared memory
inline Tree cezanne(const QString &sys, int n = 0, const char *slot = "0000:05:00.0")
{
    Tree t = card(sys, n, "1002", "1638", "amdgpu", slot);
    put(t.dev + "/gpu_busy_percent", "4\n");
    put(t.dev + "/mem_info_vram_total", "536870912\n");
    put(t.dev + "/mem_info_vram_used", "201326592\n");
    put(t.dev + "/mem_info_gtt_total", "8273260544\n");
    put(t.dev + "/mem_info_gtt_used", "92274688\n");
    put(t.dev + "/power_dpm_force_performance_level", "auto\n");
    put(t.dev + "/pp_dpm_sclk", "0: 200Mhz\n1: 400Mhz *\n2: 2000Mhz\n");
    put(t.dev + "/pp_power_profile_mode",
        "NUM       MODE_NAME BUSY_SET_POINT FPS USE_RLC_BUSY MIN_ACTIVE_LEVEL\n"
        "  0 BOOTUP_DEFAULT :             70  60          0              0\n"
        "  1 3D_FULL_SCREEN*:             70  60          1              3\n"
        "  2   POWER_SAVING :             90  60          0              0\n"
        "  3          VIDEO :             70  60          0              0\n"
        "  4             VR :             70  90          0              0\n"
        "  5        COMPUTE :             30  60          0              6\n"
        "  6         CUSTOM :              0   0          0              0\n");
    put(t.hw + "/temp1_input", "52250\n");
    put(t.hw + "/temp1_label", "edge\n");
    put(t.hw + "/power1_average", "9000000\n");
    put(t.hw + "/freq1_input", "400000000\n");
    put(t.hw + "/freq1_label", "sclk\n");
    return t;
}

inline void nvidia(const QString &sys, int n = 1)
{
    card(sys, n, "10de", "2206", "nvidia", "0000:01:00.0");
}

inline void intel(const QString &sys, int n = 0)
{
    card(sys, n, "8086", "9a49", "i915", "0000:00:02.0");
}

} // namespace Fake
