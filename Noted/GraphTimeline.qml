import QtQuick 2.0
import QtGraphicalEffects 1.0
import com.llr 1.0

Item {
    objectName: 'Loudness'
    id: graphSet
    anchors.left: parent.left
    anchors.right: parent.right
    height: handle.y + handle.height / 2
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
            id: handle
            anchors.horizontalCenter: parent.horizontalCenter
            y: 200
            width: 30
            height: 30
            color: handleMouseArea.pressed ? 'black' : 'gray';
            radius: 10
            border.color: 'black';
            border.width: 2;
            MouseArea {
                id: handleMouseArea
                anchors.fill: parent
                drag.target: handle
                drag.axis: Drag.YAxis
                drag.minimumY: 30 - height / 2
            }
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
            url: "RhythmDetectorFloat/TatumPhase"
            offset: localTime(timelines.offset, timelines.pitch)
            pitch: timelines.pitch
            anchors.fill: parent
//                    Row { spacing: 20; Text { text: parent.parent.offset;  } Text { text: parent.parent.pitch; } Text { text: timelines.offset + mapToItem(timelines, 0, 0).x * pitch; } }
        }
        Chart {
            url: "RhythmDetectorFloat/Loudness"
            offset: localTime(timelines.offset, timelines.pitch)
            pitch: timelines.pitch
            anchors.fill: parent
        }
    }
}
