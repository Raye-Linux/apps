// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Windows programs and the processes of the layer that runs them, read
// from /proc/<pid>/exe and /proc/<pid>/cmdline as Raye's layer leaves them.

#include "appgroups.h"
#include "windowsprocess.h"

#include <QTest>

using namespace Raye::TaskManager;

namespace
{
const QString preloader = QStringLiteral("/opt/raye-wine/lib/wine/x86_64-unix/wine-preloader");

QByteArray cmd(const char *text)
{
    // the layer pads the command line with nuls
    return QByteArray(text) + QByteArray(40, '\0');
}
}

class TestWindowsProcess : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void programFromItsPath()
    {
        const WindowsProcess p = windowsProcess(preloader, cmd("C:\\Program Files\\Notepad++\\notepad++.exe"));
        QCOMPARE(p.role, WindowsRole::Program);
        QCOMPARE(p.file, QStringLiteral("notepad++.exe"));
    }

    void programStartedFromItsFolder()
    {
        const WindowsProcess p = windowsProcess(preloader, cmd("notepad++.exe"));
        QCOMPARE(p.role, WindowsRole::Program);
        QCOMPARE(p.file, QStringLiteral("notepad++.exe"));
    }

    void programWithArguments()
    {
        QByteArray line("C:\\Program Files\\7-Zip\\7zFM.exe");
        line += '\0';
        line += "Z:\\home\\me\\a.zip";
        QCOMPARE(windowsProcess(preloader, line).file, QStringLiteral("7zFM.exe"));
    }

    void layerHelpers()
    {
        for (const char *line : {"C:\\windows\\system32\\services.exe", "C:\\windows\\system32\\winedevice.exe",
                                 "C:\\windows\\system32\\explorer.exe", "c:\\WINDOWS\\system32\\svchost.exe",
                                 "C:\\windows\\syswow64\\rundll32.exe", "D:\\windows\\system32\\plugplay.exe"}) {
            QCOMPARE(windowsProcess(preloader, cmd(line)).role, WindowsRole::Support);
        }
        QCOMPARE(windowsProcess(QStringLiteral("/opt/raye-wine/bin/wineserver"), cmd("/opt/raye-wine/bin/wineserver")).role,
                 WindowsRole::Support);
        // a helper started by name, before it has its full path
        QCOMPARE(windowsProcess(preloader, cmd("start.exe")).role, WindowsRole::Support);
        QCOMPARE(windowsProcess(preloader, cmd("")).role, WindowsRole::Support);
    }

    void otherBuildsOfTheLayer()
    {
        QCOMPARE(windowsProcess(QStringLiteral("/usr/lib/wine/wine64-preloader"), cmd("C:\\Games\\game.exe")).role, WindowsRole::Program);
        QCOMPARE(windowsProcess(QStringLiteral("/usr/bin/wineserver"), cmd("wineserver")).role, WindowsRole::Support);
        QCOMPARE(windowsProcess(QStringLiteral("/usr/lib/wine/wine"), cmd("C:\\x\\y.exe")).role, WindowsRole::Program);
    }

    void ordinaryProcesses()
    {
        QCOMPARE(windowsProcess(QStringLiteral("/usr/bin/konsole"), cmd("konsole")).role, WindowsRole::None);
        // a name that only looks like one
        QCOMPARE(windowsProcess(QStringLiteral("/home/me/bin/winecellar"), cmd("C:\\x.exe")).role, WindowsRole::None);
        QCOMPARE(windowsProcess(QString(), cmd("C:\\x.exe")).role, WindowsRole::None);
    }

    void names()
    {
        QCOMPARE(windowsProgramName(QStringLiteral("notepad++.exe")), QStringLiteral("notepad++"));
        QCOMPARE(windowsProgramName(QStringLiteral("FL64.EXE")), QStringLiteral("FL64"));
        QCOMPARE(windowsProgramName(QStringLiteral("tool")), QStringLiteral("tool"));
        QCOMPARE(windowsProgramAppId(QStringLiteral("7zFM.exe")), QStringLiteral("windows:7zFM.exe"));
        QCOMPARE(windowsProgramFile(QStringLiteral("windows:notepad++.exe")), QStringLiteral("notepad++.exe"));
        QVERIFY(windowsProgramFile(QStringLiteral("org.kde.konsole")).isEmpty());
    }

    void genericLaunchers()
    {
        // opened with a double click: the scope names the opener, not the program
        QVERIFY(isGenericWindowsLauncher(QStringLiteral("raye-run-windows")));
        QVERIFY(isGenericWindowsLauncher(QStringLiteral("wine")));
        QVERIFY(isGenericWindowsLauncher(QStringLiteral("org.raye.windows.package")));
        QVERIFY(!isGenericWindowsLauncher(QStringLiteral("raye-win-notepad-notepad")));
        QVERIFY(!isGenericWindowsLauncher(QStringLiteral("org.kde.dolphin")));
    }

    void startupClassInDesktopFiles()
    {
        const DesktopEntry e = parseDesktopEntry("[Desktop Entry]\nName=Notepad++\nIcon=raye-win-npp\nStartupWMClass=notepad++.exe\n",
                                                 QStringLiteral("en"));
        QCOMPARE(e.startupWMClass, QStringLiteral("notepad++.exe"));
    }
};

QTEST_GUILESS_MAIN(TestWindowsProcess)
#include "test_windowsprocess.moc"
