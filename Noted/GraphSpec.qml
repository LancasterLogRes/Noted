import QtQuick 2.0

MouseArea {
	id: dragArea
	property string url: "RhythmDetectorFloat/Loudness"
	property string graphType: "Chart"
	drag.target: graphSpec
	width: graphSpec.width; height: graphSpec.height
	onReleased: if (graphSpec.Drag.drop() !== Qt.IgnoreAction) console.log("Accepted!");
	Rectangle {
		color: 'lightgray'
		border { width: 2; color: 'black' }
		anchors.fill: parent
		radius: graphSpec.radius
		Rectangle {
			id: graphSpec
			objectName: "DragRect" + dragArea.url
			property string url: dragArea.url
			property string graphType: dragArea.graphType
			Text { id: tx; text: dragArea.url; anchors.centerIn: parent }
			width: tx.width + 10; height: tx.height + 10
			border { width: 2; color: 'black' }
			anchors {
				horizontalCenter: parent.horizontalCenter;
				verticalCenter: parent.verticalCenter
			}
			radius: 5
			gradient: Gradient {
				GradientStop { position: 0; color: "lightgray" }
				GradientStop { position: 1; color: "gray" }
			}
			Drag.keys: "Qt"
			Drag.active: dragArea.drag.active
			Drag.hotSpot.x: width / 2
			Drag.hotSpot.y: height / 2
			states: [
				State {
					when: graphSpec.Drag.active
					PropertyChanges {
						target: graphSpec
						opacity: 0.5
					}
					AnchorChanges {
						target: graphSpec
						anchors.horizontalCenter: undefined
						anchors.verticalCenter: undefined
					}
				}
			]
		}
	}
}
