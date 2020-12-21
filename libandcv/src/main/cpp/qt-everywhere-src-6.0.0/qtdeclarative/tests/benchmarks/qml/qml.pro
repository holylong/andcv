TEMPLATE = subdirs

SUBDIRS += \
           binding \
           compilation \
           javascript \
           holistic \
           qqmlchangeset \
           qqmlcomponent \
           qqmlmetaproperty \
           librarymetrics_performance \
           script \
           js \
           creation

qtHaveModule(opengl) {
    SUBDIRS += qquickwindow
    qtHaveModule(openglwidgets): SUBDIRS += painting
}
