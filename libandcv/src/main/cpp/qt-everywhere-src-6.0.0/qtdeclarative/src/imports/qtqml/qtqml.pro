TARGETPATH = QtQml
CXX_MODULE = qml
TARGET  = qmlplugin
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

# In Qt6 we won't need qmlmodels-private here
QT = qml-private qmlmodels-private

DYNAMIC_QMLDIR = \
    "module QtQml" \
    "optional plugin qmlplugin" \
    "classname QtQmlPlugin" \
    "typeinfo plugins.qmltypes" \
    "designersupported" \
    "import QtQml.Models auto"

qtConfig(qml-worker-script): DYNAMIC_QMLDIR += \
    "import QtQml.WorkerScript auto"

load(qml_plugin)
