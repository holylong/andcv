# Generated from qtattributionsscanner.pro.

#####################################################################
## qtattributionsscanner Tool:
#####################################################################

qt_get_tool_target_name(target_name qtattributionsscanner)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Qt Source Code Attribution Scanner"
    TOOLS_TARGET Tools # special case
    SOURCES
        jsongenerator.cpp jsongenerator.h
        logging.h
        main.cpp
        package.h
        packagefilter.cpp packagefilter.h
        qdocgenerator.cpp qdocgenerator.h
        scanner.cpp scanner.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:qtattributionsscanner.pro:<TRUE>:
# CMAKE_BIN_DIR = "$$cmakeRelativePath($$[QT_HOST_BINS], $$[QT_INSTALL_PREFIX])"
# CMAKE_INSTALL_LIBS_DIR = "$$cmakeTargetPath($$[QT_INSTALL_LIBS])"
# CMAKE_LIB_DIR = "$$cmakeRelativePath($$[QT_INSTALL_LIBS], $$[QT_INSTALL_PREFIX])"
# CMAKE_PACKAGE_VERSION = "$$MODULE_VERSION"
# INSTALLS = "cmake_qattributionsscanner_tools_files"
# QMAKE_SUBSTITUTES = "cmake_qattributionsscanner_config_file" "cmake_qattributionsscanner_config_version_file"
# QMAKE_TARGET_DESCRIPTION = "Qt Source Code Attribution Scanner"
# QT_FOR_CONFIG = "tools-private"
# _OPTION = "host_build"
# _REQUIREMENTS = "qtConfig(qtattributionsscanner)"
# cmake_qattributionsscanner_config_file.input = "$$PWD/Qt5AttributionsScannerTools.cmake.in"
# cmake_qattributionsscanner_config_file.output = "$$MODULE_BASE_OUTDIR/lib/cmake/Qt5AttributionsScannerTools/Qt5AttributionsScannerToolsConfig.cmake"
# cmake_qattributionsscanner_config_version_file.input = "$$[QT_HOST_DATA/src]/mkspecs/features/data/cmake/Qt5ConfigVersion.cmake.in"
# cmake_qattributionsscanner_config_version_file.output = "$$MODULE_BASE_OUTDIR/lib/cmake/Qt5AttributionsScannerTools/Qt5AttributionsScannerToolsConfigVersion.cmake"
# cmake_qattributionsscanner_tools_files.CONFIG = "no_check_exist"
# cmake_qattributionsscanner_tools_files.files = "$$cmake_qattributionsscanner_config_file.output" "$$cmake_qattributionsscanner_config_version_file.output"
# cmake_qattributionsscanner_tools_files.path = "$$[QT_INSTALL_LIBS]/cmake/Qt5AttributionsScannerTools"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qtattributionsscanner.pro:CMAKE_INSTALL_LIBS_DIR___contains___^(/usr?/lib(64)?._x_):
# CMAKE_USR_MOVE_WORKAROUND = "$$CMAKE_INSTALL_LIBS_DIR"

#### Keys ignored in scope 3:.:.:qtattributionsscanner.pro:NOT CMAKE_LIB_DIR___contains___^\\.\\./._x_:
# CMAKE_RELATIVE_INSTALL_DIR = "$${CMAKE_RELATIVE_INSTALL_LIBS_DIR}../../"
# CMAKE_RELATIVE_INSTALL_LIBS_DIR = "$$cmakeRelativePath($$[QT_INSTALL_PREFIX], $$[QT_INSTALL_LIBS])"

#### Keys ignored in scope 4:.:.:qtattributionsscanner.pro:else:
# CMAKE_LIB_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 5:.:.:qtattributionsscanner.pro:CMAKE_BIN_DIR___contains___^\\.\\./._x_:
# CMAKE_BIN_DIR = "$$[QT_HOST_BINS]/"
# CMAKE_BIN_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 6:.:.:qtattributionsscanner.pro:QMAKE_HOST.os___equals___Windows:
# CMAKE_BIN_SUFFIX = ".exe"
