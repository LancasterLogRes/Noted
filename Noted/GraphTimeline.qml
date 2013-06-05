import QtQuick 2.0
import QtGraphicalEffects 1.0
import com.llr 1.0

Item {
	objectName: 'Loudness'
	id: graphSet
	anchors.left: parent.left
	anchors.right: parent.right
	height: handle.y + handle.height / 2

	ListModel {
		id: graphs
		ListElement { graphUrl: "wave" }
		ListElement { graphUrl: "RhythmDetectorFloat/Loudness" }
	}

	Rectangle {
		DropArea {
			id: graphDrop
			objectName: graphSet.objectName + "Drop"
			anchors.fill: parent
			onDropped: {
				console.log("Dropped " + drop.source.objectName + " on " + Drag.source.objectName);
				graphs.append({'graphUrl': drop.source.url})
				drop.accept();
			}
		}

		// control panel
		width: 200
		id: panel
		color: 'lightgray'
		height: parent.height;
		anchors.left: parent.left

		border { color: 'darkgray'; width: 4 }
		states: [
			State {
				when: graphDrop.containsDrag
				PropertyChanges {
					target: panel
					border.color: 'black'
				}
			}
		]

		Text { id: title; scale: 0.8; text: parent.parent.objectName; anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 5 }
		Text { id: info; scale: 0.5; text: parent.yFrom + " " + parent.yDelta + " " + list.currentIndex + " " + graphHarness.highlightedGraph.yScaleHint + " " + graphHarness.highlightedGraph.url; anchors.top: title.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 3 }
		ListView {
			id: list
			model: graphs
			anchors.top: info.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			anchors.margins: 10
			delegate: Item {
				width: panel.width; height: listItemText.height * 1.1;
				Text { id: listItemText; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter; width: 200; text: graphUrl }
				MouseArea { anchors.fill: parent; onClicked: list.currentIndex = index; }
			}
			highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
			highlightFollowsCurrentItem: true

		}

		property vector3d yScaleUser: Qt.vector3d(0, 1, 0)
		property real yFrom: yMode == 0 ? yScaleUser.x : graphHarness.highlightedGraph.yScaleHint.x
		property real yDelta: yMode == 0 ? yScaleUser.y : graphHarness.highlightedGraph.yScaleHint.y
		property int yMode: 1		// 0 -> user, 1 -> highlighted graph, 2-> all

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
		height: parent.height
		yFrom: panel.yFrom
		yDelta: panel.yDelta
	}
	Item {
		id: graphHarness
		height: parent.height
		anchors.right: parent.right
		anchors.left: scale.right
		property Graph highlightedGraph: children[list.currentIndex + 2]
		MouseArea {
			anchors.fill: parent
			onWheel: view.zoomTimeline(mapToItem(timelines, wheel.x, wheel.y).x, Math.exp(-wheel.angleDelta.y / (wheel.modifiers & Qt.ControlModifier ? 24000.0 : wheel.modifiers & Qt.ShiftModifier ? 240.0 : 2400.0)))
		}
		YScale {
			anchors.fill: parent
			yFrom: panel.yFrom
			yDelta: panel.yDelta
		}
		Repeater {
			id: graphRepeater
			model: graphs
			Graph {
				Text { text: parent.url + parent.yScaleHint; y: index * 20 }
				url: graphUrl
				offset: localTime(timelines.offset, timelines.pitch)
				pitch: timelines.pitch
				anchors.fill: parent
				highlight: (index == list.currentIndex)
				yMode: 0		// 0 -> respect graphtimeline's y-scale, 1-> ignore y-scale and do best-fit
				yFrom: panel.yFrom
				yDelta: panel.yDelta
			}
		}
	}
}
