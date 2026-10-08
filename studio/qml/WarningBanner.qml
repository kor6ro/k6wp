// C-15 warning banner (plan todo 12): a problem sentence plus the exact
// action button(s) the copy deck pairs with it.
//
// It renders ONLY while its problem is real:
//   * kind "duplicate" - Studio.duplicateModeNotice non-empty (IS-7 clone
//     mode): [Buka Pengaturan Layar Windows].
//   * kind "coverage"  - at least one display whose get_state coverage token
//     is known and not "covered" (empty = no data yet, e.g. engine off / old
//     engine): [Muat ulang wallpaper].
//   * kind "missing"   - the collection reports a broken entry (the caller
//     counts from the Library model): [Cari file] [Hapus dari koleksi].
//
// No problem -> no banner (B-STATE). Buttons are 40px targets with
// Accessible.name; the glyph + sentence carry the state, never colour alone.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: warningBanner

    // "duplicate" | "coverage" | "missing" ("" renders nothing).
    property string kind: ""
    // Coverage source; the page feeds the filtered Studio.displays list.
    property var displays: []
    // Missing-file summary, fed by the collection page's probe.
    property int missingCount: 0
    property int firstMissingRow: -1

    readonly property bool duplicateTroubled:
        Studio.duplicateModeNotice.length > 0
    readonly property bool coverageTroubled: {
        const all = displays || []
        for (let i = 0; i < all.length; ++i) {
            const token = String(all[i].coverage)
            // Empty coverage = the engine has not reported (yet); only a
            // known token other than "covered" is a real problem.
            if (token.length > 0 && token !== "covered")
                return true
        }
        return false
    }
    readonly property bool missingTroubled: missingCount > 0
    readonly property bool troubled: kind === "duplicate" ? duplicateTroubled
                                   : kind === "coverage" ? coverageTroubled
                                   : kind === "missing" ? missingTroubled
                                   : false

    readonly property color ink: kind === "missing" ? Theme.statusError
                                                    : Theme.statusPaused
    readonly property color tint: kind === "missing" ? Theme.statusErrorTint
                                                     : Theme.statusPausedTint

    visible: troubled
    implicitHeight: Math.max(bannerRow.implicitHeight + 2 * padding, 40)
    padding: Theme.space2

    background: Rectangle {
        radius: Theme.radiusM
        color: warningBanner.tint
    }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: bannerLabel.text

    // Inline banner button: token colours, a shared Theme.glyph icon, 40px
    // target, Enter/Space activation and a visible focus ring in ink (never
    // the blue accent); pressing steps the surface.
    component BannerButton: Button {
        id: bannerButton
        property string glyph: ""
        implicitHeight: 40
        // Task 33: zero vertical padding so the contentItem spans the full
        // 40dp control (Material's 14dp verticalPadding squeezed it to 12dp).
        topPadding: 0
        bottomPadding: 0
        focusPolicy: Qt.StrongFocus
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        Accessible.name: text
        Keys.onReturnPressed: {
            bannerButton.clicked()
            event.accepted = true
        }
        Keys.onEnterPressed: {
            bannerButton.clicked()
            event.accepted = true
        }
        contentItem: RowLayout {
            spacing: Theme.space1

            Text {
                visible: bannerButton.glyph.length > 0
                Layout.alignment: Qt.AlignVCenter
                text: bannerButton.glyph
                font.family: Theme.glyphFont
                font.pixelSize: Theme.fontM
                color: Theme.text
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: bannerButton.text
                font.pixelSize: Theme.fontM
                color: Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: Theme.radiusS
            color: bannerButton.down ? Theme.pressedSurface
                   : (bannerButton.hovered || bannerButton.activeFocus
                      ? Theme.surface2 : Theme.surface)
            border.width: bannerButton.activeFocus ? 2 : 1
            border.color: bannerButton.activeFocus ? Theme.focusRing
                                                   : Theme.surface2
        }
    }

    RowLayout {
        id: bannerRow
        anchors.fill: parent
        spacing: Theme.space2

        // Glyph + label: state is never colour alone.
        Label {
            text: Theme.glyph.warning
            font.family: Theme.glyphFont
            color: warningBanner.ink
            font.pixelSize: Theme.fontM
            Accessible.ignored: true
        }

        Label {
            id: bannerLabel
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.text
            font.pixelSize: Theme.fontM
            text: warningBanner.kind === "duplicate"
                  ? qsTr("Layar kamu dalam mode duplikat, jadi video yang sama tampil di semua layar.")
                  : warningBanner.kind === "coverage"
                    ? qsTr("Ada layar yang gambarnya terpotong atau tidak tampil.")
                    : warningBanner.kind === "missing"
                      ? qsTr("%1 video tidak ketemu (mungkin dipindah).").arg(warningBanner.missingCount)
                      : ""
        }

        // C-15 duplicate action.
        BannerButton {
            objectName: "bannerDisplaySettings"
            visible: warningBanner.kind === "duplicate"
            text: qsTr("Buka Pengaturan Layar Windows")
            glyph: Theme.glyph.external
            onClicked: Studio.openWindowsDisplaySettings()
        }

        // C-15 coverage action: re-apply the video that is already active.
        BannerButton {
            objectName: "bannerReloadWallpaper"
            visible: warningBanner.kind === "coverage"
            enabled: Studio.activeVideoPath.length > 0 && !Studio.busy
            text: qsTr("Muat ulang wallpaper")
            glyph: Theme.glyph.refresh
            onClicked: Studio.applyWallpaper(Studio.activeVideoPath)
        }

        // C-15 missing-file actions, bound to the first broken entry.
        BannerButton {
            objectName: "bannerFindFile"
            visible: warningBanner.kind === "missing"
            text: qsTr("Cari file")
            glyph: Theme.glyph.external
            onClicked: Library.openLocationAt(warningBanner.firstMissingRow)
        }

        BannerButton {
            objectName: "bannerRemoveEntry"
            visible: warningBanner.kind === "missing"
            text: qsTr("Hapus dari koleksi")
            glyph: Theme.glyph.remove
            onClicked: Library.removeAt(warningBanner.firstMissingRow, false)
        }
    }
}
