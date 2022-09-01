set(PROJECT_SOURCES
        main.cpp
        ShadersGUI.cpp
        ShadersGUI.h
        ShadersGUI.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(kwin-effect-shaders_gui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET kwin-effect-shaders_gui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(kwin-effect-shaders_gui SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(kwin-effect-shaders_gui
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(kwin-effect-shaders_gui PUBLIC Qt${QT_VERSION_MAJOR}::Network PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(kwin-effect-shaders_gui PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(kwin-effect-shaders_gui)
endif()

install(
    TARGETS
        kwin-effect-shaders_gui
    DESTINATION
        /usr/local/bin/
)
