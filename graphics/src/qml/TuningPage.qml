// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Performance: how the card picks its speed, the kind of work it is tuned
// for and the most power it may draw. Nothing changes until Apply; the
// limits are the card's own, read from it.

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
    signal changed(var pending)

    readonly property var levels: [
        { value: "auto", text: i18nc("@item performance mode", "Automatic (recommended)") },
        { value: "low", text: i18nc("@item performance mode", "Power saving") },
        { value: "high", text: i18nc("@item performance mode", "Maximum performance") },
    ]
    readonly property var profiles: limits.profiles || []
    readonly property bool usesProfile: pending.profile >= 0 && pending.profile !== limits.defaultProfile
    readonly property int capShown: pending.powerCapW > 0 ? pending.powerCapW
                                  : limits.capDefaultW > 0 ? limits.capDefaultW : limits.capW

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
            text: i18nc("@title", "Performance")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        SettingsCard {
            Layout.fillWidth: true
            implicitWidth: 320
            visible: page.limits.level !== ""
            enabled: !page.usesProfile
            title: i18nc("@label", "Performance mode")
            description: page.usesProfile
                ? i18nc("@info", "The workload profile below sets the card's speed. Set it back to Default to choose a mode here.")
                : i18nc("@info", "Automatic speeds the card up when there is work and lets it rest when there is none.")
            ChoiceButton {
                implicitWidth: 240
                choices: page.levels.map(l => l.text)
                currentIndex: Math.max(0, page.levels.findIndex(l => l.value === page.pending.performance))
                onChosen: (index) => page.set("performance", page.levels[index].value)
            }
        }

        SettingsCard {
            Layout.fillWidth: true
            implicitWidth: 320
            visible: page.profiles.length > 0
            title: i18nc("@label", "Workload profile")
            description: i18nc("@info", "Tunes how fast the card reacts for one kind of work. Default suits most people.")
            ChoiceButton {
                implicitWidth: 240
                choices: page.profiles.map(p => Fmt.profileName(p.name))
                currentIndex: Math.max(0, page.profiles.findIndex(p => p.index === (page.pending.profile >= 0 ? page.pending.profile : page.limits.defaultProfile)))
                onChosen: (index) => {
                    const p = page.profiles[index].index
                    page.set("profile", p === page.limits.defaultProfile ? -1 : p)
                }
            }
        }

        Card {
            Layout.fillWidth: true
            visible: page.limits.hasPowerCap

            ColumnLayout {
                id: power
                anchors.fill: parent
                spacing: Tokens.s2

                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        QQC2.Label {
                            text: i18nc("@label", "Power limit")
                            color: Tokens.textPrimary
                            font: Tokens.body
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            text: i18nc("@info", "The most power the card may draw. Lower runs cooler and quieter, higher can be a little faster.")
                            color: Tokens.textSecondary
                            font: Tokens.caption
                        }
                    }
                    QQC2.Label {
                        text: Fmt.watts(slider.value)
                        color: Tokens.textPrimary
                        font: Tokens.bodyStrong
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tokens.s4
                    QQC2.Label {
                        text: Fmt.watts(page.limits.capMinW ?? 0)
                        color: Tokens.textTertiary
                        font: Tokens.caption
                    }
                    LimitSlider {
                        id: slider
                        Layout.fillWidth: true
                        from: page.limits.capMinW ?? 0
                        to: page.limits.capMaxW ?? 1
                        value: page.capShown
                        mark: page.limits.capDefaultW ?? -1
                        onMoved: page.set("powerCapW", Math.round(value))
                    }
                    QQC2.Label {
                        text: Fmt.watts(page.limits.capMaxW ?? 0)
                        color: Tokens.textTertiary
                        font: Tokens.caption
                    }
                    AccentButton {
                        accent: false
                        visible: page.limits.capDefaultW > 0
                        enabled: page.pending.powerCapW > 0 && page.pending.powerCapW !== page.limits.capDefaultW
                        text: i18nc("@action:button the card's own power limit", "Default")
                        onClicked: page.set("powerCapW", 0)
                    }
                }
                QQC2.Label {
                    visible: page.limits.capDefaultW > 0
                    text: i18nc("@info", "The card's own limit is %1.", Fmt.watts(page.limits.capDefaultW))
                    color: Tokens.textTertiary
                    font: Tokens.caption
                }
            }
        }

        InfoBar {
            Layout.fillWidth: true
            Layout.topMargin: Tokens.s3
            visible: page.limits.level === "" && page.profiles.length === 0 && !page.limits.hasPowerCap
            closable: false
            message: i18nc("@info", "This card doesn't let its performance settings be changed.")
        }
    }
}
