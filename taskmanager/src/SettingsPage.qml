// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// Settings: which page opens first, and how often the figures move.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.raye.ui

Item {
    id: page

    property var pages: []
    property int startPage: 0
    property string speed: "normal"

    signal startPageChosen(int index)
    signal speedChosen(string value)

    readonly property var speeds: [
        { value: "high", text: i18nc("@item update speed", "High") },
        { value: "normal", text: i18nc("@item update speed", "Normal") },
        { value: "low", text: i18nc("@item update speed", "Low") },
        { value: "paused", text: i18nc("@item update speed", "Paused") },
    ]

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Tokens.s2

        QQC2.Label {
            Layout.preferredHeight: Tokens.sizeControlHeight
            Layout.bottomMargin: Tokens.s3
            verticalAlignment: Text.AlignVCenter
            text: i18nc("@title", "Settings")
            color: Tokens.textPrimary
            font: Tokens.subtitle
        }

        SettingsCard {
            Layout.fillWidth: true
            title: i18nc("@label", "Default start page")
            description: i18nc("@info", "The page the Task Manager opens on.")
            ChoiceButton {
                choices: page.pages.map(p => p.text)
                currentIndex: Math.min(page.startPage, page.pages.length - 1)
                onChosen: (index) => page.startPageChosen(index)
            }
        }

        SettingsCard {
            Layout.fillWidth: true
            title: i18nc("@label", "Real time update speed")
            description: i18nc("@info", "How often the figures are read. Slower uses less of the processor.")
            ChoiceButton {
                choices: page.speeds.map(s => s.text)
                currentIndex: Math.max(0, page.speeds.findIndex(s => s.value === page.speed))
                onChosen: (index) => page.speedChosen(page.speeds[index].value)
            }
        }
    }
}
