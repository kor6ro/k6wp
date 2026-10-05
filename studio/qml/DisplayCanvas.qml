// DisplayCanvas.qml — plan row 20: the read-only proportional monitor canvas.
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
// READ-ONLY by design (Q2): these rects mirror the Windows display topology
// and are NEVER repositioned from here — this file contains no drag attached
// properties, only the handler-less DropAreas that row 23 wires for
// drop-to-assign. No topology selector, no fit-mode control, no writable
// state of any kind.
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
            // Row 23 hook: per-rect drop highlight. Nothing sets it yet.
            property bool dropActive: false

            x: root.mapX(entry.x)
            y: root.mapY(entry.y)
            width: root.mapW(entry.width)
            height: root.mapH(entry.height)
            radius: 4
            clip: true
            color: degraded ? Qt.rgba(0.86, 0.2, 0.2, 0.16)
                            : Material.dialogColor
            border.width: primaryMonitor ? 2 : 1
            border.color: primaryMonitor
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

            Image {
                id: assignedThumb
                objectName: "assignedThumb"
                visible: monitorRect.thumbSource.length > 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: assignedLabel.top
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

            Label {
                id: assignedLabel
                objectName: "assignedLabel"
                visible: monitorRect.hasAssignment
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.bottomMargin: 4
                elide: Text.ElideMiddle
                font.pixelSize: 10
                color: monitorRect.degraded ? "red" : Material.foreground
                text: root.fileName(monitorRect.assignedPath)
            }

            // Anything not exactly "covered" (clipped-*, headless, or a
            // missing token) is surfaced, never silent.
            Label {
                id: coverageWarning
                objectName: "coverageWarning"
                visible: String(monitorRect.entry.coverage || "")
                         !== "covered"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: assignedLabel.top
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                anchors.bottomMargin: 2
                elide: Text.ElideRight
                font.pixelSize: 10
                color: "orange"
                text: qsTr("Cakupan: %1 \u2014 cek engine.log")
                      .arg(String(monitorRect.entry.coverage || "tidak diketahui"))
            }

            // Handler-less on purpose: row 23 adds onEntered / onExited /
            // onDropped (and the refusal messages). The keys filter is already
            // here so a foreign file drag never matches these areas.
            DropArea {
                id: monitorDrop
                objectName: "monitorDrop"
                anchors.fill: parent
                keys: ["application/x-k6wp-assignment"]
            }
        }
    }
}
