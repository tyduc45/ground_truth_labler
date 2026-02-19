set(SOURCES
    main.cpp
    core/AnnotationData.cpp
    core/VideoManager.cpp
    core/TrackingEngine.cpp
    core/MotExporter.cpp
    ui/MainWindow.cpp
    ui/VideoWidget.cpp
    ui/LabelPanel.cpp
    ui/ResultPanel.cpp
    ui/ControlBar.cpp
    util/FrameConverter.cpp
)

set(HEADERS
    core/AnnotationData.h
    core/VideoManager.h
    core/TrackingEngine.h
    core/MotExporter.h
    ui/MainWindow.h
    ui/VideoWidget.h
    ui/LabelPanel.h
    ui/ResultPanel.h
    ui/ControlBar.h
    util/FrameConverter.h
)

add_executable(GT_labler ${SOURCES} ${HEADERS})

target_include_directories(GT_labler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(GT_labler PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    opencv_core
    opencv_imgproc
    opencv_videoio
    opencv_tracking
)

# 编译后自动将 Qt 平台插件复制到可执行文件同级的 platforms/ 目录
if(WIN32)
    get_filename_component(_vcpkg_installed "${Qt6_DIR}/../.." ABSOLUTE)
    set(_qt_plugin_release "${_vcpkg_installed}/Qt6/plugins/platforms/qwindows.dll")
    set(_qt_plugin_debug   "${_vcpkg_installed}/debug/Qt6/plugins/platforms/qwindowsd.dll")

    add_custom_command(TARGET GT_labler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:GT_labler>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${_qt_plugin_debug},${_qt_plugin_release}>"
            "$<TARGET_FILE_DIR:GT_labler>/platforms/"
        COMMENT "Deploying Qt platform plugin (windows)"
    )
endif()


