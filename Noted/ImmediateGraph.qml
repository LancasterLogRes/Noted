import QtQuick 2.0
import com.llr 1.0

Item {
	id: graphSet
	property ListModel timelineGraphs: ListModel { }

	property int overflow: 10
	property var highlighted: h
	property var immGraphHarness: graphHarness

	Item {
		anchors.fill: parent
		anchors.margins: -graphSet.overflow
		clip: true

		Item {
			anchors.fill: parent
			anchors.margins: graphSet.overflow

			Item {
				id: graphHarness
				anchors.left: ylabels.right
				anchors.top: parent.top
				anchors.bottom: xlabels.top
				anchors.right: parent.right
				anchors.margins: graphSet.overflow

				property var highlightedGraph: parent.visible && timelineGraphs.count ? children[panel.currentIndex] : 0

				Repeater {
					id: graphRepeater
					model: timelineGraphs
					CursorGraph {
						id: graphItem
						anchors.fill: parent
						xScale: xscale.xScale
						yScale: yscale.yScale
						xMode: yMode
						yMode: 0
						color: Qt.hsla(0, 0, 0.5, 1)
						url: model.url
						highlight: (index == panel.currentIndex)
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
				XScale {
					id: xscale
					anchors.fill: parent
					xScale: graphHarness.highlightedGraph.xScaleHint
					z: -1
				}
				YScale {
					id: yscale
					anchors.fill: parent
					yScale: graphHarness.highlightedGraph.yScaleHint
					z: -1
				}

			}
			Item {
				id: ylabels
				anchors.left: parent.left
				anchors.top: graphHarness.top
				anchors.bottom: graphHarness.bottom
				anchors.topMargin: -graphSet.overflow
				anchors.bottomMargin: -graphSet.overflow

				width: graphSet.width > 240 ? 80 : graphSet.width > 160 ? graphSet.width - 160 : 0
				YLabels {
					anchors.fill: parent
					anchors.rightMargin: 5
					yScale: yscale.yScale
					overflow: graphSet.overflow
				}
			}
			XLabels {
				id: xlabels
				anchors.left: graphHarness.left
				anchors.right: graphHarness.right
				anchors.bottom: parent.bottom
				anchors.leftMargin: -graphSet.overflow
				anchors.rightMargin: -graphSet.overflow
				xScale: xscale.xScale
				height: graphSet.height > 120 ? 20 : graphSet.height > 100 ? graphSet.height - 100 : 0
				overflow: graphSet.overflow
			}
		}
	}
}
