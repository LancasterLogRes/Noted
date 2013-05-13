import QtQuick 2.0
import com.llr 1.0

Timelines {
    offset: 0
    pitch: 50
    id: timelines
    Column {
        anchors.fill: parent
        Item {
            objectName: 'Loudness'
            anchors.left: parent.left;
            anchors.right: parent.right;
            height: 100
            Rectangle {
                Text { text: parent.parent.objectName }
                // control panel
                width: 200
                id: panel
                color: 'gray'
                height: parent.height;
                anchors.left: parent.left

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.bottom
                    width: 30
                    height: 30
                    color: black
                    radius: 10
                }
            }
            Rectangle {
                // yaxis
                id: scale
                width: 50
                color: 'black'
                anchors.left: panel.right
                height: parent.height;
            }
            Chart {
                ec: "RhythmDetectorFloat"
                graph: "TatumPhase"
                height: parent.height;
                anchors.right: parent.right
                anchors.left: scale.right
                offset: timelines.offset
                pitch: timelines.pitch
            }
            Chart {
                ec: "RhythmDetectorFloat"
                graph: "Loudness"
                height: parent.height;
                anchors.right: parent.right
                anchors.left: scale.right
                offset: timelines.offset
                pitch: timelines.pitch
            }
        }
        spacing: 20
    }
}
