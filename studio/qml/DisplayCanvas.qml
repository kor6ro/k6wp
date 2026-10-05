// DisplayCanvas.qml — plan rows 20 + 23: the read-only proportional monitor
// canvas with drop-to-assign.
//
// One Rectangle per monitor entry, positioned and sized by mapping the union
// bounding box of every entry's x/y/width/height into this Item's area with
// the ASPECT RATIO PRESERVED and CENTERED on both axes. That is what makes a
// 1:1 next to a scaled monitor, or a portrait next to a landscape one, read
// correctly at a glance (IS-2).
//
// Data enters through COMPONENT PROPERTIES, never through the Studio singleton:
//
//   displaysModel : the Studio.displays list (row 19 BuildDisplayEntries), one
//                   map per monitor with {key, label, x, y, width, height,
//                   isPrimary, orientation, scalePercent, refreshHz,
//                   resolutionLabel, assignedPath, assignedExists, coverage}.
//                   Main.qml (row 21) binds this to Studio.displays.
//   posterSource  : the video path `posterPath` belongs to.
//   posterPath    : the cached Thumbnailer JPEG for `posterSource`; shown as
//                   the assigned thumbnail only for a rect whose assignedPath
//                   matches posterSource (filename is the fallback).
//
// Taking the model as a property is what keeps the offscreen
// display_canvas_test hermetic: it feeds plain JS fixtures and needs no IPC
// engine behind the singleton.
//
// Row 23 adds drop-to-assign on the per-rect DropAreas:
//
//   displayCapability / duplicateModeNotice / busy mirror the row-19
//   StudioBridge properties; fileExistsProbe is an optional injected
//   function(path) -> bool because this component has no filesystem access;
//   refusalMessage backs the visible refusalLabel. A drop that clears every
//   gate emits assignRequested(key, path); Main.qml (row 21) connects it to
//   Studio.assignVideoToMonitor. clearRequested(key) is the clear-affordance
//   contract row 21/24 connect to Studio.clearMonitorAssignment.
//
// Row 24 completes the assignment state surface: the assigned filename +
// cached thumbnail, the 'ganti' re-arm affordance (raises the drop highlight;
// never mutates assignment state), the 'hapus' button (emits
// clearRequested(monitorKey)), the 'file tidak ditemukan' label for a
// vanished file, the 'cek engine.log' coverage warning, and the
// 'tarik video ke sini' hint while assignedPath is empty. Every string is
// qsTr() Indonesian (row 25 catalogues them).
//
// READ-ONLY by design (Q2): these rects mirror the Windows display topology
// and are NEVER repositioned from here — this file contains no drag attached
// properties, only the DropArea handlers that accept assignment drops. No
// topology selector, no fit-mode control, no writable state of any kind.
//
// Spacing follows the Material 8dp rhythm where there is room; the map itself
// is pixel-exact virtual-desktop geometry, not a spacing token.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
    id: root
    objectName: "displayCanvas"

    // --- component property contract (bound by Main.qml, row 21) -----------
    property var displaysModel: []
    property string posterSource: ""
    property string posterPath: ""

    // --- row 23 drop contract (Main.qml row 21 binds these + the signals) --
    // Mirrors StudioBridge's displayCapability / duplicateModeNotice / busy
    // (studio_bridge.hpp:195-198, :158) so Main.qml can bind 1:1.
    property bool displayCapability: true
    property string duplicateModeNotice: ""
    property bool busy: false

    // Optional injected existence probe: null means "cannot verify, accept"
    // (this pure-QML component cannot stat the filesystem); a host that can
    // overrides it with function(path) -> bool, and a drop the probe reports
    // missing is refused with the file-missing message.
    property var fileExistsProbe: null

    // Message of the last refused drop; also rendered by refusalLabel so a
    // refusal is always visible (never a silent failure).
    property string refusalMessage: ""

    // Emitted only after every gate passes; row 21 connects these to
    // Studio.assignVideoToMonitor / Studio.clearMonitorAssignment.
    signal assignRequested(string key, string path)
    signal clearRequested(string key)

    // --- geometry -----------------------------------------------------------
    // Usable entries only: positive width/height and finite x/y. A zero-size
    // or malformed entry is dropped instead of drawn as a degenerate rect, so
    // an all-invalid model renders nothing and must raise no QML error.
    readonly property var validDisplays: {
        const out = []
        const list = displaysModel
        if (list && list.length) {
            for (let i = 0; i < list.length; ++i) {
                const m = list[i]
                if (!m)
                    continue
                const x = Number(m.x)
                const y = Number(m.y)
                const w = Number(m.width)
                const h = Number(m.height)
                if (w > 0 && h > 0 && x === x && y === y)
                    out.push(m)
            }
        }
        return out
    }

    // Union bounding box of the virtual desktop, in virtual-screen pixels.
    // valid=false for an empty model; width/height are then positive by
    // construction for every valid monitor, so the fit below can never divide
    // by zero (a 1-element union is maxX == x + w > x).
    readonly property var union: {
        const list = validDisplays
        let minX = Infinity
        let minY = Infinity
        let maxX = -Infinity
        let maxY = -Infinity
        for (let i = 0; i < list.length; ++i) {
            const m = list[i]
            const x = Number(m.x)
            const y = Number(m.y)
            const w = Number(m.width)
            const h = Number(m.height)
            if (x < minX) minX = x
            if (y < minY) minY = y
            if (x + w > maxX) maxX = x + w
            if (y + h > maxY) maxY = y + h
        }
        const ok = minX !== Infinity && minY !== Infinity &&
                   maxX > minX && maxY > minY
        return {
            valid: ok,
            x: ok ? minX : 0,
            y: ok ? minY : 0,
            width: ok ? maxX - minX : 0,
            height: ok ? maxY - minY : 0
        }
    }

    // Aspect-preserving, centered fit: ONE scale for both axes (the min of the
    // two ratios) plus the centering offsets. An empty / zero-size area or an
    // invalid union yields valid=false and the Repeater stays empty.
    readonly property var fit: {
        const b = union
        if (!b.valid || !(width > 0) || !(height > 0))
            return { valid: false, scale: 0, offsetX: 0, offsetY: 0 }
        const scale = Math.min(width / b.width, height / b.height)
        return {
            valid: scale > 0,
            scale: scale,
            offsetX: (width - b.width * scale) / 2,
            offsetY: (height - b.height * scale) / 2
        }
    }

    // Virtual-desktop pixels -> canvas pixels (relative to this Item).
    function mapX(vx) { return fit.offsetX + (Number(vx) - union.x) * fit.scale }
    function mapY(vy) { return fit.offsetY + (Number(vy) - union.y) * fit.scale }
    function mapW(vw) { return Number(vw) * fit.scale }
    function mapH(vh) { return Number(vh) * fit.scale }

    // Cached poster lookup: only the exact video the parent cached a poster
    // for gets a thumbnail; every other assignment falls back to the filename.
    function thumbFor(path) {
        const p = String(path === undefined || path === null ? "" : path)
        if (p.length === 0 || root.posterPath.length === 0)
            return ""
        return p === root.posterSource ? root.posterPath : ""
    }

    // Basename for the assigned label (Windows or POSIX separators).
    function fileName(path) {
        const p = String(path === undefined || path === null ? "" : path)
        const slash = Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\"))
        return slash >= 0 ? p.slice(slash + 1) : p
    }

    // --- row 23 drop handling ----------------------------------------------
    // The per-rect DropArea handlers delegate here so every gate is testable
    // offscreen without synthesising native drag events. The reader in the
    // onDropped handler is DragEvent.getDataAsString: the row-22 drag sources
    // set only the custom assignment MIME, and the drop's text property (which
    // resolves only text/plain) would always be an empty string.

    function rectForKey(key) {
        const wanted = String(key)
        for (let i = 0; i < monitorRepeater.count; ++i) {
            const rect = monitorRepeater.itemAt(i)
            // qmllint disable missing-property
            // monitorKey is a delegate property; itemAt() is statically
            // typed QQuickItem, so qmllint cannot see the dynamic member.
            if (rect && rect["monitorKey"] === wanted)
                return rect
            // qmllint enable missing-property
        }
        return null
    }

    // Existence probe seam: null means "cannot verify, accept"; a host can
    // inject a function(path) -> bool (kept as a local binding so qmllint does
    // not have to resolve a var property as a call target).
    function droppedPathExists(path) {
        const probe = root.fileExistsProbe
        if (probe === null)
            return true
        return !!probe(path)
    }

    // onEntered / onExited path: toggle the per-rect accent outline.
    function setDropActive(key, active) {
        const rect = rectForKey(key)
        if (rect)
            rect.dropActive = active === true
    }

    function clearDropHighlights() {
        for (let i = 0; i < monitorRepeater.count; ++i) {
            const rect = monitorRepeater.itemAt(i)
            if (rect)
                rect.dropActive = false
        }
    }

    // Drop gate: every refusal sets refusalMessage and emits nothing; only a
    // path that clears all gates reaches assignRequested.
    function handleDrop(key, path) {
        root.refusalMessage = ""
        clearDropHighlights()
        const p = String(path === undefined || path === null ? "" : path)
        if (p.length === 0) {
            root.refusalMessage =
                qsTr("Drop tidak dikenali - tarik video dari pustaka atau playlist.")
            return
        }
        if (!root.displayCapability) {
            root.refusalMessage =
                qsTr("Engine lama - perbarui engine untuk fitur ini")
            return
        }
        if (root.duplicateModeNotice !== "") {
            root.refusalMessage = root.duplicateModeNotice
            return
        }
        if (root.busy) {
            root.refusalMessage =
                qsTr("Studio sedang sibuk - coba lagi sebentar lagi.")
            return
        }
        if (!droppedPathExists(p)) {
            root.refusalMessage =
                qsTr("File tidak ditemukan - video sudah dipindahkan atau dihapus.")
            return
        }
        root.assignRequested(String(key), p)
    }

    Repeater {
        id: monitorRepeater
        objectName: "monitorRepeater"
        model: root.fit.valid ? root.validDisplays : []

        delegate: Rectangle {
            id: monitorRect
            objectName: "monitorRect"
            required property var modelData

            readonly property var entry: modelData
            readonly property string monitorKey: String(entry.key || "")
            readonly property bool primaryMonitor: entry.isPrimary === true
            readonly property string assignedPath:
                String(entry.assignedPath || "")
            readonly property bool hasAssignment: assignedPath.length > 0
            // "Degraded" = an assignment whose file is gone. An unassigned
            // monitor is not degraded (assignedExists is false for both).
            readonly property bool degraded:
                hasAssignment && entry.assignedExists !== true
            readonly property string thumbSource: root.thumbFor(assignedPath)
            // Row 23: raised by root.setDropActive while a compatible drag is
            // over this rect (DropArea onEntered/onExited) and cleared by an
            // accepted or refused drop; drives the outline below.
            property bool dropActive: false

            x: root.mapX(entry.x)
            y: root.mapY(entry.y)
            width: root.mapW(entry.width)
            height: root.mapH(entry.height)
            radius: 4
            clip: true
            color: degraded ? Qt.rgba(0.86, 0.2, 0.2, 0.16)
                            : Material.dialogColor
            border.width: monitorRect.dropActive ? 3 : (primaryMonitor ? 2 : 1)
            border.color: monitorRect.dropActive || primaryMonitor
                          ? Material.accent
                          : Qt.rgba(Material.foreground.r,
                                    Material.foreground.g,
                                    Material.foreground.b, 0.35)

            // Degraded tint under the content, so the text stays readable.
            Rectangle {
                id: degradedOverlay
                objectName: "degradedOverlay"
                visible: monitorRect.degraded
                anchors.fill: parent
                color: Qt.rgba(0.8, 0.1, 0.1, 0.12)
            }

            Label {
                id: titleLabel
                objectName: "monitorTitle"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 6
                anchors.topMargin: 4
                anchors.rightMargin: monitorRect.primaryMonitor ? 54 : 6
                elide: Text.ElideRight
                font.pixelSize: 11
                font.bold: true
                text: String(monitorRect.entry.label || "")
            }

            Rectangle {
                id: primaryBadge
                objectName: "primaryBadge"
                visible: monitorRect.primaryMonitor
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 4
                anchors.rightMargin: 4
                width: badgeLabel.implicitWidth + 10
                height: badgeLabel.implicitHeight + 4
                radius: height / 2
                color: Material.accent

                Label {
                    id: badgeLabel
                    anchors.centerIn: parent
                    text: qsTr("UTAMA")
                    font.pixelSize: 9
                    color: "white"
                }
            }

            Label {
                id: resolutionLabel
                objectName: "monitorResolution"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: titleLabel.bottom
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.topMargin: 2
                elide: Text.ElideRight
                font.pixelSize: 10
                opacity: 0.85
                text: String(monitorRect.entry.resolutionLabel || "")
            }

            Label {
                id: infoLine
                objectName: "monitorInfoLine"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: resolutionLabel.bottom
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.topMargin: 1
                elide: Text.ElideRight
                font.pixelSize: 10
                opacity: 0.7
                text: String(monitorRect.entry.orientation || "")
                      + " \u00B7 " + String(monitorRect.entry.scalePercent || 100)
                      + "% \u00B7 " + String(monitorRect.entry.refreshHz || 0)
                      + " Hz"
            }

            // Row 24: an unassigned rect advertises itself as a drop target
            // instead of staying mute; the hint disappears the moment an
            // assignment exists.
            Label {
                id: dropHint
                objectName: "dropHint"
                visible: !monitorRect.hasAssignment
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: infoLine.bottom
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.topMargin: 8
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                font.pixelSize: 10
                opacity: 0.65
                text: qsTr("tarik video ke sini")
            }

            Image {
                id: assignedThumb
                objectName: "assignedThumb"
                visible: monitorRect.thumbSource.length > 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: assignActions.visible ? assignActions.top
                                                      : assignmentStatus.top
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.bottomMargin: 2
                height: visible ? Math.max(18,
                                           Math.min(implicitHeight,
                                                    monitorRect.height * 0.35))
                                : 0
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                source: monitorRect.thumbSource
                sourceSize.width: 320
                sourceSize.height: 180
            }

            // Row 24: the per-monitor status stack, bottom-anchored so the map
            // geometry above it never shifts. A positioner skips invisible
            // children, so an unassigned + covered rect collapses this to
            // nothing. Top-to-bottom: coverage warning, missing-file label,
            // assigned filename.
            Column {
                id: assignmentStatus
                objectName: "assignmentStatus"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.bottomMargin: 4
                spacing: 2

                // Anything not exactly "covered" (clipped-*, headless, or a
                // missing token) is surfaced, never silent.
                Label {
                    id: coverageWarning
                    objectName: "coverageWarning"
                    visible: String(monitorRect.entry.coverage || "")
                             !== "covered"
                    width: assignmentStatus.width
                    elide: Text.ElideRight
                    font.pixelSize: 10
                    color: "orange"
                    text: qsTr("Cakupan: %1 \u2014 cek engine.log")
                          .arg(String(monitorRect.entry.coverage || "tidak diketahui"))
                }

                // Row 24 degraded state: the assignment points at a file that
                // no longer exists. Naming it explicitly (playlist's exists
                // wording) keeps a vanished file from being silent.
                Label {
                    id: degradedLabel
                    objectName: "degradedLabel"
                    visible: monitorRect.degraded
                    width: assignmentStatus.width
                    elide: Text.ElideRight
                    font.pixelSize: 10
                    font.bold: true
                    color: Material.color(Material.Red)
                    text: qsTr("file tidak ditemukan")
                }

                Label {
                    id: assignedLabel
                    objectName: "assignedLabel"
                    visible: monitorRect.hasAssignment
                    width: assignmentStatus.width
                    elide: Text.ElideMiddle
                    font.pixelSize: 10
                    color: monitorRect.degraded ? Material.color(Material.Red)
                                                : Material.foreground
                    text: root.fileName(monitorRect.assignedPath)
                }
            }

            // Row 24 affordances, right-aligned above the status stack. Both
            // stay hidden until an assignment exists.
            Row {
                id: assignActions
                objectName: "assignActions"
                visible: monitorRect.hasAssignment
                anchors.right: parent.right
                anchors.bottom: assignmentStatus.top
                anchors.rightMargin: 6
                anchors.bottomMargin: 2
                spacing: 4

                Button {
                    id: replaceButton
                    objectName: "replaceAffordance"
                    text: qsTr("Ganti")
                    flat: true
                    padding: 4
                    font.pixelSize: 10
                    // Row 24 "ganti": re-arm this rect for a replacement drop
                    // by raising the same dropActive highlight a drag-over
                    // raises (and focusing the DropArea when it has a window).
                    // This touches NO assignment state - only a real drop
                    // through root.handleDrop can replace the file.
                    onClicked: {
                        monitorRect.dropActive = true
                        monitorDrop.forceActiveFocus()
                    }
                }

                Button {
                    id: clearButton
                    objectName: "clearButton"
                    text: qsTr("Hapus")
                    flat: true
                    padding: 4
                    font.pixelSize: 10
                    // Row 24 "hapus": emit the already-declared root signal;
                    // Main.qml (row 21) connects it to
                    // Studio.clearMonitorAssignment.
                    onClicked: root.clearRequested(monitorRect.monitorKey)
                }
            }

            // Row 23: the keys filter stays so a foreign file drag never
            // matches these areas; the handlers below only ever see the
            // assignment MIME and delegate every gate to root.handleDrop.
            DropArea {
                id: monitorDrop
                objectName: "monitorDrop"
                anchors.fill: parent
                keys: ["application/x-k6wp-assignment"]

                onEntered: root.setDropActive(monitorRect.monitorKey, true)
                onExited: root.setDropActive(monitorRect.monitorKey, false)
                // B2: read the path from the custom MIME. The drop's text
                // property resolves only text/plain, which row 22 never sets,
                // so it would be an empty string here.
                onDropped: (drop) => root.handleDrop(
                    monitorRect.monitorKey,
                    drop ? drop.getDataAsString("application/x-k6wp-assignment")
                         : "")
            }
        }
    }

    // Row 23 refusal feedback: every refused drop surfaces its Indonesian
    // message here (IS-7 collision, IS-4 capability gate, busy, missing file,
    // empty payload), so no refusal is silent.
    Rectangle {
        id: refusalBanner
        objectName: "refusalBanner"
        visible: root.refusalMessage.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        width: Math.min(refusalText.implicitWidth + 20,
                        Math.max(120, root.width - 16))
        height: refusalText.implicitHeight + 10
        radius: height / 2
        color: Material.dialogColor
        border.width: 1
        border.color: Qt.rgba(Material.foreground.r, Material.foreground.g,
                              Material.foreground.b, 0.25)
        z: 10

        Label {
            id: refusalText
            objectName: "refusalLabel"
            anchors.centerIn: parent
            width: Math.max(0, parent.width - 16)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: 11
            color: Material.color(Material.Red)
            text: root.refusalMessage
        }
    }
}
