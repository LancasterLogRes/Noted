import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {
	color: Qt.rgba(0.9, 0.9, 0.9, 1)

	property alias handleX: handle.x
	property alias handleY: handle.y
	property vector3d yScaleUser: Qt.vector3d(0, 1, 0)
	property vector3d yScale: yMode == 0 || !parent.visible ? yScaleUser : graphHarness.highlightedGraph.yScaleHint
	property int yMode: 1		// 0 -> user, 1 -> highlighted graph, 2-> all

	property int maxHandleX
	property int index
	property alias currentIndex: list.currentIndex

	function swap(x) {
		var d;
		d = yMode; yMode = x.yMode; x.yMode = d;
		d = yScaleUser; yScaleUser = x.yScaleUser; x.yScaleUser = d;
		d = handleX; handleX = x.handleX; x.handleX = d;
		d = handleY; handleY = x.handleY; x.handleY = d;
	}

	function reset() {
		yMode = 1
		yScaleUser = Qt.vector3d(0, 1, 0)
		handleX = 200
		handleY = 200
	}

	function zoomScale(y, q) {
		var pivot = yScale.x + yScale.y * (height - y) / height
		console.log(pivot)
		var d = q * yScale.y
		yScaleUser.x = pivot - q * yScale.y * (height - y) / height
		yScaleUser.y = d
		yMode = 0
	}

	height: handle.y + handle.height / 2

	Column {
		id: col

		anchors.fill: parent
		anchors.margins: 5

		Rectangle {
			id: title
			color: Qt.rgba(1, 1, 1, 0.2)
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
				model: timelineGraphs
				anchors.fill: parent
				highlight: highlight
				highlightFollowsCurrentItem: false

				delegate: Item {
					width: list.width
					height: 30
					RowLayout {
						anchors.fill: parent
						anchors.margins: 4
						spacing: 4
						Text {
							id: listItemText
							Layout.fillWidth: true
							text: model.name
							elide: Text.ElideRight
							MouseArea {
								anchors.fill: parent
								onClicked: list.currentIndex = index
							}
						}
						Button { text: "<"; onClicked: graphs.exportGraph(model.url) }
						Row {
							anchors.top: parent.top
							anchors.bottom: parent.bottom
							Button { text: "~"; selected: graphHarness.children[index].yMode == 1; onClicked: graphHarness.children[index].yMode = immGraphHarness.children[index].yMode = 1 }
							Button { text: "S"; selected: graphHarness.children[index].yMode == 0; onClicked: graphHarness.children[index].yMode = immGraphHarness.children[index].yMode = 0 }
						}
						Button { text: 'X'; onClicked: timelineGraphs.remove(index); }
					}
				}
			}
		}
	}

	Rectangle {
		id: handle
		x: 200
		y: 200

		width: 30
		height: 30
		color: handleMouseArea.pressed ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(0.9, 0.9, 0.9, 1)
		border.color: 'white';
		border.width: 5;
		MouseArea {
			id: handleMouseArea
			anchors.fill: parent
			drag.target: handle
			drag.axis: Drag.XAndYAxis
			drag.minimumX: 100
			drag.maximumX: col.parent.maxHandleX - width / 2
			drag.minimumY: title.anchors.margins * 2 + title.height + list.anchors.margins * 2 + 40 - height / 2 + col.anchors.margins * 2
		}
	}
}
