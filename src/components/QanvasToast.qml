import QtQuick
import QtQuick.Controls

import Themed.Components

Item {
    id: root

    function show(text) {
        toast.showMessage(text)
    }

    ThemedToast {
        id: toast

        parent: root.Overlay.overlay ? root.Overlay.overlay : root
    }
}
