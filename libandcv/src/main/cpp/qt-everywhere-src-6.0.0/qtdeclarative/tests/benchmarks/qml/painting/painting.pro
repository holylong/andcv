requires(qtHaveModule(opengl))
requires(qtHaveModule(openglwidgets))
requires(qtHaveModule(widgets))

QT += opengl widgets openglwidgets
CONFIG += console
macx:CONFIG -= app_bundle

SOURCES += paintbenchmark.cpp
