import QtQuick 2.0
import QtGraphicalEffects 1.0
import com.llr 1.0

Item {
	id: graphSet
	property int index
	property alias handleY: panel.handleY
	property ListModel graphs: ListModel { }

	function swap(x) {
		var d;
		d = graphs; graphs = x.graphs; x.graphs = d;
		d = handleY; handleY = x.handleY; x.handleY = d;
		d = visible; visible = x.visible; x.visible = d;
	}
	function kill() { visible = false; handleY = 200; graphs.clear() }

	anchors.left: parent ? parent.left : undefined
	anchors.right: parent ? parent.right : undefined
	height: panel.height
	visible: false

	ControlPanel {
		id: panel
		anchors.left: parent.left
		width: timelines.gutterWidth - yScale.width - graphHarness.anchors.leftMargin
		index: parent.index
		handleY: 200
	}

	Item {
		id: graphHarness
		height: parent.height
		anchors.right: parent.right
		anchors.left: yScale.right
		anchors.leftMargin: 10
		property var highlightedGraph: parent.visible && graphs.count ? children[panel.currentIndex] : 0
		Repeater {
			id: graphRepeater
			model: graphs
			Graph {
				url: graphUrl
				offset: timelines.offset
				pitch: timelines.pitch
				anchors.fill: parent
				highlight: (index == panel.currentIndex)
				yMode: 0		// 0 -> respect graphtimeline's y-scale, 1-> ignore y-scale and do best-fit
				yFrom: panel.yFrom
				yDelta: panel.yDelta
			}
		}
		YScale {
			z: -1
			anchors.fill: parent
			yFrom: panel.yFrom
			yDelta: panel.yDelta
			anchors.leftMargin: -10
		}
		DropArea {
			id: graphDrop
			anchors.fill: parent
			onDropped: {
				graphs.append({'graphUrl': drop.source.url})
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
		anchors.left: panel.right
		height: parent.height + overflow * 2
		yFrom: panel.yFrom
		yDelta: panel.yDelta
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
