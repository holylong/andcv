TARGET = qtquickcontrols2imaginestyleplugin
TARGETPATH = QtQuick/Controls/Imagine

QML_IMPORT_NAME = QtQuick.Controls.Imagine
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private quickcontrols2impl-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(imagine.pri)

OTHER_FILES += \
    qmldir \
    $$QML_FILES

SOURCES += \
    $$PWD/qtquickcontrols2imaginestyleplugin.cpp

qtquickcontrols2imaginestyle.prefix = qt-project.org/imports/QtQuick/Controls/Imagine
qtquickcontrols2imaginestyle.files += \
    $$files($$PWD/images/*.png) \
    $$files($$PWD/images/*.webp)
RESOURCES += qtquickcontrols2imaginestyle

CONFIG += qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-imagine))
