# Generated from qt5compat.pro.

cmake_minimum_required(VERSION 3.15.0)

include(.cmake.conf)
project(Qt5Compat # special case
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt 5 Compatibility Libraries" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
# Make sure we only use latest private CMake API, aka no compatibility wrappers.
set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Network Xml)
# special case end

qt_build_repo()
