import QtQuick 2.0
import QtGraphicalEffects 1.0
import com.llr 1.0

Timelines {
    id: timelines
    objectName: 'timelines'
    offset: ViewMan.offset
    pitch: ViewMan.pitch

//    Row { spacing: 20; Text { text: timelines.offset;  } Text { text: timelines.pitch; } }
    Column {
        anchors.fill: parent
        XLabels {
            id: header
            anchors.left: parent.left
            anchors.right: parent.right
            height: 30
            offset: timelines.offset
            pitch: timelines.pitch
        }

        GraphTimeline {}
        spacing: 20
    }
}
