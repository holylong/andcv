# Generated from qttranslations.pro.

cmake_minimum_required(VERSION 3.15.0)

include(.cmake.conf)
project(QtTranslations # special case
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Translations" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
set(QT_REPO_NOT_WARNINGS_CLEAN TRUE)
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core) # special case
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Linguist) # special case

include(QtSetup) # for qt_install

if(TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate AND TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::lconvert)
    add_subdirectory(translations)
else()
    message(STATUS "Skipping adding translation custom targets due to missing lupdate or lconvert.")
endif()
# special case end
