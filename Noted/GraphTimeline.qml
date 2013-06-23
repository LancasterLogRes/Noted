import QtQuick 2.0
import QtGraphicalEffects 1.0
import com.llr 1.0

Item {
	id: graphSet
	property int index
	property alias handleX: panel.handleX
	property alias handleY: panel.handleY
	property ListModel timelineGraphs: ListModel { }
	property alias immGraphHarness: immGraph.immGraphHarness
	property alias panel: panel

	function swap(x) {
		panel.swap(x.panel)
		var d;
		d = timelineGraphs; timelineGraphs = x.timelineGraphs; x.timelineGraphs = d;
		d = visible; visible = x.visible; x.visible = d;
	}
	function kill() {
		visible = false
		timelineGraphs.clear()
		panel.reset()
	}

	anchors.left: parent ? parent.left : undefined
	anchors.right: parent ? parent.right : undefined
	height: panel.height
	visible: false

	ControlPanel {
		id: panel
		anchors.left: parent.left
		width: handleX
		index: parent.index
		maxHandleX: timelines.gutterWidth - yScale.width - graphHarness.anchors.leftMargin
		handleX: 200
		handleY: 200
	}

	ImmediateGraph {
		id: immGraph
		anchors.left: panel.right
		width: panel.maxHandleX - panel.width
		height: panel.height
		timelineGraphs: parent.timelineGraphs
	}

	Item {
		id: graphHarness
		height: parent.height
		anchors.right: parent.right
		anchors.left: yScale.right
		anchors.leftMargin: 10
		property var highlightedGraph: parent.visible && timelineGraphs.count ? children[panel.currentIndex] : 0
		Repeater {
			id: graphRepeater
			model: timelineGraphs
			Graph {
				id: graphItem
				url: model.url
				offset: timelines.offset
				pitch: timelines.pitch
				anchors.fill: parent
				highlight: (index == panel.currentIndex)
				yMode: 0		// 0 -> respect graphtimeline's y-scale, 1-> ignore y-scale and do best-fit
				yScale: panel.yScale
				z: highlight ? 1 : 0
				Text {
					scale: 2
					text: "Data not available for " + model.name
					anchors.fill: parent
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					visible: !graphItem.dataAvailable && graphItem.graphAvailable
				}
				Text {
					scale: 2
					text: "Graph not available for " + model.url
					anchors.fill: parent
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					visible: !graphItem.graphAvailable
				}
			}
		}
		YScale {
			z: -1
			anchors.fill: parent
			yScale: panel.yScale
			anchors.leftMargin: -10
		}
		DropArea {
			id: graphDrop
			anchors.fill: parent
			onDropped: {
				timelineGraphs.append({'url': drop.source.url, 'name': drop.source.name})
				drop.accept();
			}
			Rectangle {
				id: dragHighlight
				color: Qt.rgba(0, 0, 1, 0.25)
				visible: graphDrop.containsDrag
				anchors.fill: parent
			}
		}
	}
	YLabels {
		id: yScale
		overflow: 10
		y: -overflow
		width: 60
		anchors.left: immGraph.right
		height: parent.height + overflow * 2
		yScale: panel.yScale
		MouseArea {
			anchors.fill: parent
			anchors.topMargin: parent.overflow
			anchors.bottomMargin: parent.overflow
			onWheel: {
				panel.zoomScale(wheel.y, Math.exp(-wheel.angleDelta.y / (wheel.modifiers & Qt.ControlModifier ? 24000.0 : wheel.modifiers & Qt.ShiftModifier ? 240.0 : 2400.0)));
			}
			acceptedButtons: Qt.LeftButton
			preventStealing: true;
			onPressed: {
				if (mouse.button == Qt.LeftButton && panel.yMode == 0)
				{
					mouse.accepted = true;
					posDrag = mouse.y;
					posOffset = panel.yScaleUser.x

				}
			}
			onReleased: { posDrag = -1; }
			property int posDrag: -1;
			property var posOffset;
			onPositionChanged: {
				if (posDrag > -1)
					panel.yScaleUser.x = (mouse.y - posDrag) * panel.yScaleUser.y / height + posOffset;
			}
		}
	}
}
