import QtQuick 2.0

MouseArea {
	id: dragArea
	property string url
	property string name
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
			property alias url: dragArea.url
			property alias name: dragArea.name
			Text { id: tx; text: parent.name; anchors.centerIn: parent }
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
					PropertyChanges {
						target: footer
						graphSpecDrag: true
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
