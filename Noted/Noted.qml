import QtQuick 2.0
import com.llr 1.0

Item {
	Timelines {
		id: overview
		offset: Time.mul(audio.duration, -0.05)
		pitch: Time.div(audio.duration, width / 1.1)
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		height: 90
		TimeLabels {
			id: overviewLabels
			height: 30
			anchors.left: parent.left
			anchors.right: parent.right
			offset: overview.offset
			pitch: overview.pitch
		}
		Graph {
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: overviewLabels.bottom
			height: 60
			Interval {
				anchors.fill: parent
				offset: overview.offset
				pitch: overview.pitch
				begin: view.offset
				duration: Time.mul(timelines.width, view.pitch)
			}
			Cursor {
				anchors.fill: parent
				offset: overview.offset
				pitch: overview.pitch
				cursor: audio.cursor
				cursorWidth: audio.hop
			}
			url: 'wave'
			highlight: true
			yMode: 1
			offset: overview.offset
			pitch: overview.pitch
		}
	}

	Item {
		id: timelinesHolster
		anchors.top: overview.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: graphSpecs.top
		clip: true

		Timelines {
			id: timelines
			property int gutterWidth: gutterHandle.x + gutterHandle.width + 55
			objectName: 'timelines'
			offset: view.offset
			pitch: view.pitch
			anchors.fill: parent
			anchors.leftMargin: gutterWidth

			Rectangle {
				z: 2
				id: header
				anchors.top: parent.top
				anchors.left: parent.left
				anchors.leftMargin: -timelines.gutterWidth
				anchors.right: parent.right
				height: 30
				color: 'white'
				TimeLabels {
					anchors.fill: parent
					anchors.leftMargin: timelines.gutterWidth
					offset: timelines.offset
					pitch: timelines.pitch
				}
			}

			ListView {
				id: graphListView
				anchors.top: header.bottom
				anchors.bottom: parent.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.leftMargin: -timelines.gutterWidth
				anchors.topMargin: 10
				spacing: 20
				property int maxHandleX: Math.max(model.children[0].handleX, model.children[1].handleX, model.children[2].handleX, model.children[3].handleX, model.children[4].handleX, model.children[5].handleX, model.children[6].handleX, model.children[7].handleX, model.children[8].handleX, model.children[9].handleX)
				model: VisualItemModel {
					GraphTimeline { index: 0; visible: true; timelineGraphs: ListModel { ListElement { url: "wave"; name: "PCM" } } }
					GraphTimeline { index: 1 }
					GraphTimeline { index: 2 }
					GraphTimeline { index: 3 }
					GraphTimeline { index: 4 }
					GraphTimeline { index: 5 }
					GraphTimeline { index: 6 }
					GraphTimeline { index: 7 }
					GraphTimeline { index: 8 }
					GraphTimeline { index: 9 }
				}
				function append() {
					for (var i in model.children)
					{
						var item = model.children[i];
						if (!item.visible)
							return item
					}
					return 0
				}
				function kill(itemIndex) {
					for (var i = itemIndex; i < model.children.length && model.children[i].visible; ++i)
						if (i < model.children.length - 1)
							model.children[i].swap(model.children[i + 1])
						else
							model.children[i].kill()
					if (currentIndex <= itemIndex)
						currentIndex--
				}
			}
			Cursor {
				anchors.fill: parent
				offset: timelines.offset
				pitch: timelines.pitch
				cursor: audio.cursor
				cursorWidth: audio.hop
			}
		}

		MouseArea {
			anchors.top: timelinesHolster.top
			anchors.bottom: timelinesHolster.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.leftMargin: timelines.gutterWidth
			acceptedButtons: Qt.LeftButton | Qt.RightButton
			anchors.fill: parent
			onWheel: { view.zoomTimeline(wheel.x, Math.exp(-wheel.angleDelta.y / (wheel.modifiers & Qt.ControlModifier ? 24000.0 : wheel.modifiers & Qt.ShiftModifier ? 240.0 : 2400.0))); }
			onPressed: {
				if (mouse.button == Qt.RightButton)
				{
					mouse.accepted = true;
					posDrag = mouse.x;
					posOffset = view.offset
				}
				if (mouse.button == Qt.LeftButton)
				{
					mouse.accepted = true;
					audio.cursor = Time.madd(mouse.x, timelines.pitch, timelines.offset);
				}
			}
			onReleased: { posDrag = -1; }
			property int posDrag: -1;
			property var posOffset;
			onPositionChanged: {
				if (posDrag > -1)
					view.offset = Time.madd(posDrag - mouse.x, view.pitch, posOffset);
				if (mouse.buttons & Qt.LeftButton)
					audio.cursor = Time.madd(mouse.x, timelines.pitch, timelines.offset);
			}
		}

		DropArea {
			id: newGraphDrop
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			height: 80
			onDropped: {
				var i = graphListView.append()
				i.timelineGraphs.append({ 'url': drop.source.url, 'name': drop.source.name })
				i.visible = true
				drop.accept();
			}
			Rectangle {
				id: footer
				anchors.fill: parent
				color: newGraphDrop.containsDrag ? Qt.rgba(0.9, 0.9, 1, 0.9) : Qt.rgba(1, 1, 1, 0.9);
				visible: graphSpecDrag
				property bool graphSpecDrag: false
				Text {
					scale: 2
					anchors.fill: parent
					color: Qt.rgba(0.5, 0.5, 0.5, 1)
					text: "Drag here to create"
					verticalAlignment: Text.AlignVCenter
					horizontalAlignment: Text.AlignHCenter
				}
			}
		}
	}

	Rectangle {
		id: gutterHandle
		x: 200
		anchors.top: overview.bottom
		anchors.topMargin: 5

		width: 30; height: 30
		color: gutterHandleMouseArea.pressed ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(0.9, 0.9, 0.9, 1)
		border.color: 'white'
		border.width: 5
		MouseArea {
			id: gutterHandleMouseArea
			anchors.fill: parent
			drag.target: gutterHandle
			drag.axis: Drag.XAxis
			drag.minimumX: graphListView.maxHandleX
		}
	}

	Rectangle {
		id: graphSpecs
		color: Qt.rgba(0.98, 0.98, 0.98, 0.9)
		border { width: 1; color: Qt.rgba(0.9, 0.9, 0.9, 1) }
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		height: 40
		ListView {
			model: graphs
			orientation: ListView.Horizontal
			delegate: GraphSpec { url: model.url; name: model.name }

			anchors.fill: parent
			anchors.margins: 4
			spacing: 20
		}
	}
}
