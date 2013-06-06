import QtQuick 2.0
import QtGraphicalEffects 1.0
import com.llr 1.0

Item {
	Timelines {
		id: overview
		offset: view.globalOffset
		pitch: view.globalPitch
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		height: 90
		XLabels {
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

	Timelines {
		id: timelines
		objectName: 'timelines'
		offset: view.offset
		pitch: view.pitch
		anchors.top: overview.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom

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

			GraphTimeline { id: gt }

			Rectangle {
				color: Qt.rgba(0.98, 0.98, 0.98, 0.9)
				border { width: 1; color: Qt.rgba(0.9, 0.9, 0.9, 1) }
				anchors.left: parent.left
				anchors.right: parent.right
				height: 200
				Flow {
					Repeater {
						model: graphs
						GraphSpec { url: model.url }
					}
					anchors.fill: parent
					anchors.margins: 4
					spacing: 2
				}
			}
			spacing: 20
		}
	}
}
