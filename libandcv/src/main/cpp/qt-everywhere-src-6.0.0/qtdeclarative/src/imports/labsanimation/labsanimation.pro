CXX_MODULE = qml
TARGET  = labsanimationplugin
TARGETPATH = Qt/labs/animation
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    qquickboundaryrule.cpp \
    plugin.cpp

HEADERS += \
    qquickboundaryrule_p.h

QT = qml-private quick-private

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
