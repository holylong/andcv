TEMPLATE = app
TARGET = tst_snippets

QT += quick quickcontrols2 quickcontrols2-private testlib
CONFIG += testcase
macos:CONFIG -= app_bundle

DEFINES += QQC2_SNIPPETS_PATH=\\\"$$QQC2_SOURCE_TREE/src/imports/controls/doc/snippets\\\"

SOURCES += \
    $$PWD/tst_snippets.cpp

OTHER_FILES += \
    $$PWD/data/*.qml

RESOURCES += \
    qtquickcontrols2.conf

TESTDATA += \
    $$PWD/data/*
