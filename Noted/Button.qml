import QtQuick 2.0

MouseArea {
	property alias text: t.text
	property bool selected: false
	anchors.top: parent.top
	anchors.bottom: parent.bottom
	width: parent.height
	Rectangle {
		anchors.fill: parent
		border.width: parent.selected ? 2 : 1; border.color: Qt.rgba(0, 0, 0, parent.pressed ? 1 : 0.3)
		Text {
			id: t
			color: parent.border.color
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.verticalCenter: parent.verticalCenter
		}
	}
}
