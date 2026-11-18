// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Fan: the card drives its fan itself, or follows a curve set here.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

QQC2.ScrollView {
    id: page

    required property var model
    required property var pending
    readonly property var limits: model.limits
    readonly property var now: model.now
    signal changed(var pending)

    function set(key, value) {
        const p = Object.assign({}, pending)
        p[key] = value
        changed(p)
    }

    contentWidth: availableWidth
    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
    WheelScroll { target: page.contentItem }

    ColumnLayout {
        width: page.availableWidth
        spacing: Tokens.s2

        QQC2.Label {
            Layout.preferredHeight: Tokens.sizeControlHeight
            Layout.bottomMargin: Tokens.s3
            verticalAlignment: Text.AlignVCenter
            text: i18nc("@title", "Fan")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        InfoBar {
            Layout.fillWidth: true
            visible: !page.limits.canCurve
            closable: false
            message: page.limits.hasFan
                ? i18nc("@info", "This card drives its fan itself and doesn't let it be changed.")
                : i18nc("@info", "This graphics card has no fan of its own: it is cooled with the rest of the PC.")
        }

        SettingsCard {
            Layout.fillWidth: true
            implicitWidth: 320
            visible: page.limits.canCurve
            title: i18nc("@label", "Fan control")
            description: page.pending.fanCurve
                ? i18nc("@info", "The fan follows the curve below.")
                : i18nc("@info", "The card speeds its fan up and down by itself.")
            ChoiceButton {
                implicitWidth: 200
                choices: [i18nc("@item fan control", "Automatic"), i18nc("@item fan control", "Custom curve")]
                currentIndex: page.pending.fanCurve ? 1 : 0
                onChosen: (index) => page.set("fanCurve", index === 1)
            }
        }

        Card {
            Layout.fillWidth: true
            visible: page.limits.canCurve

            ColumnLayout {
                id: curveBox
                anchors.fill: parent
                spacing: Tokens.s3

                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: page.limits.junction ? i18nc("@label", "Fan speed by hotspot temperature")
                                                   : i18nc("@label", "Fan speed by temperature")
                        color: Tokens.textPrimary
                        font: Tokens.bodyStrong
                    }
                    QQC2.Label {
                        visible: page.now.curveTemp >= 0
                        text: page.now.fanPercent >= 0
                              ? i18nc("@info temperature and fan speed now", "Now %1, fan at %2", Fmt.celsius(page.now.curveTemp), Fmt.percent(page.now.fanPercent))
                              : i18nc("@info temperature now", "Now %1", Fmt.celsius(page.now.curveTemp))
                        color: Tokens.textSecondary
                        font: Tokens.body
                    }
                }

                CurveEditor {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                    enabled: page.pending.fanCurve
                    opacity: enabled ? 1 : 0.55
                    points: page.pending.curve || []
                    tempMin: page.limits.curveTempMin ?? 20
                    tempMax: page.limits.curveTempMax ?? 90
                    speedMin: page.limits.curveSpeedMin ?? 0
                    speedMax: page.limits.curveSpeedMax ?? 100
                    now: page.now.curveTemp ?? -1
                    onEdited: (points) => page.set("curve", points)
                }

                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: page.limits.firmwareCurve
                              ? i18nc("@info", "The card follows this curve itself. Drag a point, or click it and use the arrow keys.")
                              : i18nc("@info", "Above %1 the fan always runs at full speed. Drag a point, or click it and use the arrow keys.", Fmt.celsius(page.limits.curveTempMax ?? 90))
                        color: Tokens.textSecondary
                        font: Tokens.caption
                    }
                    AccentButton {
                        accent: false
                        enabled: page.pending.fanCurve
                        text: i18nc("@action:button", "Reset curve")
                        onClicked: page.set("curve", page.limits.defaultCurve)
                    }
                }
            }
        }
    }
}
