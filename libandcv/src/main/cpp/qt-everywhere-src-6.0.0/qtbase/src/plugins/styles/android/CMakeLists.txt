# Generated from android.pro.

#####################################################################
## QAndroidStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QAndroidStylePlugin
    OUTPUT_NAME qandroidstyle
    TYPE styles
    SOURCES
        main.cpp
        qandroidstyle.cpp qandroidstyle_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:android.pro:<TRUE>:
# DISTFILES = "androidstyle.json"
