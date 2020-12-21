CONFIG += testcase
SOURCES  += ../tst_qclipboard.cpp
TARGET = ../tst_qclipboard
QT += testlib gui-private

osx: LIBS += -framework AppKit

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qclipboard
} else {
    TARGET = ../../release/tst_qclipboard
  }
}

!android: TEST_HELPER_INSTALLS = \
    ../copier/copier \
    ../paster/paster