import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {
	color: Qt.rgba(0.9, 0.9, 0.9, 1)

	property alias handleY: handle.y
	property int index
	property alias currentIndex: list.currentIndex

	property vector3d yScaleUser: Qt.vector3d(0, 1, 0)
	property real yFrom: yMode == 0 || !parent.visible ? yScaleUser.x : graphHarness.highlightedGraph.yScaleHint.x
	property real yDelta: yMode == 0 || !parent.visible ? yScaleUser.y : graphHarness.highlightedGraph.yScaleHint.y
	property int yMode: 1		// 0 -> user, 1 -> highlighted graph, 2-> all

	height: handle.y + handle.height / 2

	Column {
		id: col

		anchors.fill: parent
		anchors.margins: 5

		Rectangle {
			id: title
			color: Qt.rgba(1, 1, 1, 0.9)
			height: 30
			anchors.left: parent.left
			anchors.right: parent.right
			Row {
				anchors.right: parent.right
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				anchors.margins: 4
				spacing: 4
				Row {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					Button { text: ":"; selected: col.parent.yMode == 1; onClicked: col.parent.yMode = 1 }
					Button { text: "O"; selected: col.parent.yMode == 0; onClicked: col.parent.yMode = 0 }
				}
				Button { text: "X"; onClicked: graphListView.kill(col.parent.index) }
			}
		}
		spacing: 5

		Rectangle {
			height: parent.height - title.height - parent.anchors.margins
			anchors.left: parent.left
			anchors.right: parent.right
			color: col.parent.color

			Component {
				id: highlight
				Rectangle {
					width: list.width; height: 30
					color: 'white'
					y: list.currentItem.y
					Behavior on y { SpringAnimation { spring: 10; damping: 1 } }
				}
			}

			ListView {
				id: list
				clip: true
				model: graphs
				anchors.fill: parent
				highlight: highlight
				highlightFollowsCurrentItem: false

				delegate: Item {
					width: list.width
					height: 30
					RowLayout {
						anchors.fill: parent
						anchors.margins: 5
						spacing: 5
						Text {
							id: listItemText
							Layout.fillWidth: true
							text: graphUrl
							elide: Text.ElideRight
							MouseArea {
								anchors.fill: parent
								onClicked: list.currentIndex = index
							}
						}
						Button { text: 'X'; onClicked: graphs.remove(index); }
					}
				}
			}
		}
	}

	Rectangle {
		id: handle
		y: 200

		anchors.horizontalCenter: parent.horizontalCenter
		width: 30
		height: 30
		color: handleMouseArea.pressed ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(0.9, 0.9, 0.9, 1)
		border.color: 'white';
		border.width: 5;
		MouseArea {
			id: handleMouseArea
			anchors.fill: parent
			drag.target: handle
			drag.axis: Drag.YAxis
			drag.minimumY: title.anchors.margins * 2 + title.height + list.anchors.margins * 2 + 40 - height / 2 + col.anchors.margins * 2
		}
	}
}
