// ToastBar: window-bottom compress progress toast (plan todo 13, S6).
//
// Placement contract (brief B-WIREFRAME(f) overlay matrix): anchored to the
// window's bottom edge, NEVER above the preview hole. The native mpv surface
// lives in the upper portion of the Wallpaper page; this toast sits at the
// very bottom of the root Rectangle, below every page content, so it can
// never cover the preview.
//
// D3 default text rule: progress shows a percent when one is available
// (Compress.progress > 0), and the status text alone otherwise. B11
// friendlyProgressText is the pure QML function that applies that rule.
//
// C-10 outcome copy: when a job ends without a result (cancel or failure),
// the toast briefly replaces its progress text with the verbatim C-10
// sentence so the user learns the original file is untouched.
//
// Cancel paths preserved (brief §4.2 / keputusan 4): the Batal button calls
// Compress.cancel() exactly as the old compressor tab did; consent rejection
// (C-9 "Batal" -> resolveConsent(false)) surfaces C-10 through the
// consentCancelled signal from AppDialogs.
//
// Accessibility: both controls are >= 40px, carry Accessible.name, and
// handle Enter/Space. Colors come from Theme.* tokens only.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: toastBar

    // Set by Main when the user presses Batal, so onJobFinishedWithoutResult
    // can distinguish a cancel from a failure and show the right C-10 line.
    property bool cancelPending: false

    // C-10 outcome overlay: shown briefly after cancel / failure / consent
    // rejection. Cleared by the timer below.
    property string outcomeText: ""
    property bool outcomeVisible: false

    readonly property bool jobActive: Compress.running || Compress.pending > 0

    // Anchor to the window bottom edge - the layout-position contract.
    // horizontalCenter keeps it clear of the sidebar; the bottom margin keeps
    // it off the very edge. It is a sibling of the Pane in Main.qml, so it
    // floats above page content but below any Dialog overlay.
    z: 100
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: Theme.space3
    implicitWidth: Math.min(parent.width - 2 * Theme.space4,
                            toastContent.implicitWidth + 2 * Theme.space3)
    implicitHeight: toastContent.implicitHeight + 2 * Theme.space2
    radius: Theme.radiusM
    color: Theme.surface
    border.width: 1
    border.color: Theme.surface2
    visible: jobActive || outcomeVisible

    // Elevation via a drop shadow approximation (Theme.elev2 tokens): a
    // slightly larger dark rectangle behind the surface. GPU-composited
    // (transform/opacity only where animated); static here.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius + 2
        color: Theme.bg
        opacity: Theme.elev2.opacity
        z: -1
    }

    // B11 / D3: pure function. Percent when progress is available, status
    // text alone otherwise. Kept as a QML function because the task forbids
    // touching any C++ file; the rule itself is the tested D3 contract.
    function friendlyProgressText(progress, statusText) {
        const p = Number(progress)
        if (!isNaN(p) && p > 0)
            return String(Math.round(p)) + "%"
        const t = String(statusText === undefined || statusText === null
                          ? "" : statusText)
        return t
    }

    // C-10 outcome display (cancel / failure / consent-reject).
    function showOutcome(text) {
        outcomeText = String(text === undefined || text === null ? "" : text)
        outcomeVisible = true
        outcomeTimer.restart()
    }

    // The single cancel entry point. cancelPending is what lets Main's
    // onJobFinishedWithoutResult pick the C-10 cancel sentence.
    function cancelJob() {
        cancelPending = true
        Compress.cancel()
    }

    Timer {
        id: outcomeTimer
        interval: 6000
        repeat: false
        onTriggered: toastBar.outcomeVisible = false
    }

    RowLayout {
        id: toastContent
        anchors.fill: parent
        anchors.margins: Theme.space2
        spacing: Theme.space2

        // Decorative glyph: the text label below carries the accessible name.
        Label {
            text: toastBar.outcomeVisible ? Theme.glyph.warning
                                          : Theme.glyph.hourglass
            font.family: Theme.glyphFont
            color: toastBar.outcomeVisible ? Theme.statusPaused
                                           : Theme.accent
            font.pixelSize: Theme.fontL
            Accessible.ignored: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space1

            // Primary line: C-10 outcome when showing, D3 progress otherwise.
            Label {
                objectName: "toastBarText"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
                font.pixelSize: Theme.fontM
                font.weight: Theme.fontWeightSemibold
                color: Theme.text
                text: toastBar.outcomeVisible
                      ? toastBar.outcomeText
                      : toastBar.friendlyProgressText(Compress.progress,
                                                      Compress.statusText)
                Accessible.name: text
            }

            // Progress bar: only while a job is actually running. Indeterminate
            // while queued but not yet reporting (same rule the old page used).
            ProgressBar {
                Layout.fillWidth: true
                visible: !toastBar.outcomeVisible && Compress.running
                from: 0
                to: 100
                indeterminate: Compress.running && Compress.progress === 0
                value: Compress.progress
            }

            // Secondary line: ETA + detail, only while running and not in the
            // outcome overlay. Kept at metadata size (Theme.fontS).
            Label {
                Layout.fillWidth: true
                visible: !toastBar.outcomeVisible && Compress.running
                elide: Text.ElideRight
                font.pixelSize: Theme.fontS
                color: Theme.text2
                text: {
                    const parts = []
                    if (Compress.etaText.length > 0)
                        parts.push(Compress.etaText)
                    if (Compress.detailText.length > 0)
                        parts.push(Compress.detailText)
                    return parts.join("  •  ")
                }
                Accessible.ignored: true
            }
        }

        // Batal button (C-8 progress copy: "Menyiapkan video… {p}% [Batal]").
        // >= 40px, Enter/Space, Accessible.name. Hidden during the outcome
        // overlay - there is nothing left to cancel. Task 31: hover/focus no
        // longer flips the fill to the blue accent; a press darkens the red
        // one step and focus keeps the ink border.
        Button {
            id: cancelButton
            objectName: "toastBarCancel"
            visible: !toastBar.outcomeVisible && Compress.running
            implicitHeight: 40
            // Task 33: zero vertical padding keeps the 40dp content box
            // centred (Material's verticalPadding otherwise squeezed it).
            topPadding: 0
            bottomPadding: 0
            focusPolicy: Qt.StrongFocus
            text: qsTr("Batal")
            // `glyph`, not `icon`: Button.icon is a FINAL QQuickIcon property.
            property string glyph: Theme.glyph.close
            Accessible.name: text
            onClicked: toastBar.cancelJob()

            Keys.onReturnPressed: {
                toastBar.cancelJob()
                event.accepted = true
            }
            Keys.onEnterPressed: {
                toastBar.cancelJob()
                event.accepted = true
            }

            contentItem: RowLayout {
                spacing: Theme.space1

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: cancelButton.glyph
                    font.family: Theme.glyphFont
                    font.pixelSize: Theme.fontM
                    color: Theme.accentText
                    verticalAlignment: Text.AlignVCenter
                    Accessible.ignored: true
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: cancelButton.text
                    font.pixelSize: Theme.fontM
                    font.weight: Theme.fontWeightSemibold
                    color: Theme.accentText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            background: Rectangle {
                radius: Theme.radiusS
                color: cancelButton.down
                       ? Qt.darker(Theme.statusError, 1.2)
                       : Theme.statusError
                border.width: cancelButton.activeFocus ? 2 : 0
                border.color: Theme.accentText
            }
        }
    }
}
