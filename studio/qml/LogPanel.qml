// Shared log panel: serves both the Wallpaper right-rail log block and the
// Pengaturan log block. The caller wraps it in its own Pane/layout and passes
// the concatenated log text; the inner label + ScrollView + TextArea are
// identical in both call sites.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    id: logPanel
    property alias text: logArea.text
    property alias placeholderText: logArea.placeholderText

    Label {
        text: qsTr("Detail teknis (log)")
        font.bold: true
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        TextArea {
            id: logArea
            readOnly: true
            wrapMode: TextArea.NoWrap
            selectByMouse: true
        }
    }
}
