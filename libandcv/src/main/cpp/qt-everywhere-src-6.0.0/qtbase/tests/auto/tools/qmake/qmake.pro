CONFIG += testcase
# Allow more time since examples are compiled, which may take longer on Windows.
win32:testcase.timeout=900
TARGET = tst_qmake
HEADERS += testcompiler.h
SOURCES += tst_qmake.cpp testcompiler.cpp
QT = core testlib
TESTDATA += testdata/*
