import QtQuick 2.0
import com.llr 1.0

Timelines {
    id: timelines
    objectName: 'timelines'
    offset: 0
    pitch: 50

    onOffsetChanged: console.log("Offset changed" + offset)
    onPitchChanged: console.log("Pitch changed" + pitch)
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

        Item {
            objectName: 'Loudness'
            id: graphSet
            anchors.left: parent.left
            anchors.right: parent.right
            height: 100
            Rectangle {
                Text { text: parent.parent.objectName }
                // control panel
                width: 200
                id: panel
                color: 'gray'
                height: parent.height;
                anchors.left: parent.left

                property real yFrom: 0
                property real yDelta: 1
                property int yMode: 0

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.bottom
                    width: 30
                    height: 30
                    color: 'black'
                    radius: 10
                }
            }
            YLabels {
                // yaxis labels
                id: scale
                width: 50
                anchors.left: panel.right
                height: parent.height;
                yFrom: panel.yFrom
                yDelta: panel.yDelta
            }
            Item {
                height: parent.height;
                anchors.right: parent.right
                anchors.left: scale.right
                YScale {
                    anchors.fill: parent
                    yFrom: panel.yFrom
                    yDelta: panel.yDelta
                }
                Chart {
                    ec: "RhythmDetectorFloat"
                    graph: "TatumPhase"
                    offset: timelines.offset + mapToItem(timelines, 0, 0).x * pitch
                    pitch: timelines.pitch
                    anchors.fill: parent
//                    Row { spacing: 20; Text { text: parent.parent.offset;  } Text { text: parent.parent.pitch; } Text { text: timelines.offset + mapToItem(timelines, 0, 0).x * pitch; } }
                }
                Chart {
                    ec: "RhythmDetectorFloat"
                    graph: "Loudness"
                    offset: timelines.offset + mapToItem(timelines, 0, 0).x * pitch
                    pitch: timelines.pitch
                    anchors.fill: parent
                }
            }
        }
        spacing: 20
    }
}
