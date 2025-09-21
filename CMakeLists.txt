cmake_minimum_required(VERSION 3.31)
project(ImageLabeler)

set(CMAKE_CXX_STANDARD 20)

# Enable automatic MOC, UIC and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    set(CMAKE_PREFIX_PATH C:/Qt5.1/Qt5.14.2/5.14.2/mingw73_64)
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -mwindows")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -mwindows ")
    endif ()
elseif (APPLE)
    set(CMAKE_PREFIX_PATH /Users/user/Qt6.12.4/5.12.4/clang_64)
elseif (UNIX AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    set(CMAKE_PREFIX_PATH /home/user/Downloads/qt-everywhere-src-5.12.4/Qt6.12.4_static)
else ()
    message(STATUS "Unknown platform")
endif ()

# Option for static linking
option(STATIC_QT "Link Qt statically" OFF)

# Find Qt Linguist Tools
find_package(Qt5 COMPONENTS Core Gui Widgets Network Sql LinguistTools REQUIRED)

# 处理翻译文件
set(TS_FILES
    ${CMAKE_SOURCE_DIR}/translations/label_en.ts
    ${CMAKE_SOURCE_DIR}/translations/label_zh.ts
)

# 生成QM文件
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

if (STATIC_QT)
    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
else()
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()

if (WIN32)
    set(CMAKE_PREFIX_PATH C:/Qt5.1/Qt5.14.2/5.14.2/mingw73_64)
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -mwindows")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -mwindows ")
    endif ()
elseif (APPLE)
    set(CMAKE_PREFIX_PATH /Users/user/Qt6.12.4/5.12.4/clang_64)
elseif (UNIX AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    set(CMAKE_PREFIX_PATH /home/user/Downloads/qt-everywhere-src-5.12.4/Qt6.12.4_static)
else ()
    message(STATUS "Unknown platform")
endif ()
find_package(Qt5 COMPONENTS Core Gui Widgets Network Sql REQUIRED)

# For static linking, we need to explicitly link Qt plugins
if (STATIC_QT)
    # Find Qt plugins path
    get_target_property(QT_CORE_PATH Qt5::Core LOCATION)
    get_filename_component(QT_PLUGINS_DIR "${QT_CORE_PATH}/../../../plugins" ABSOLUTE)

    # Link image formats plugins
    find_library(QT_QJPEG_PLUGIN qjpeg PATHS "${QT_PLUGINS_DIR}/imageformats")
    find_library(QT_QPNG_PLUGIN qpng PATHS "${QT_PLUGINS_DIR}/imageformats")
endif()

set(SOURCES
        main.cpp
        annotationgraphicsview.cpp
        classmanagerdialog.cpp
        mainwindow.cpp
)

set(HEADERS
        annotationgraphicsview.h
        classmanagerdialog.h
        mainwindow.h
)

# 创建资源文件
set(RESOURCES
    translations/label_en.qm
    translations/label_zh.qm
)

# 添加资源文件到项目
qt5_add_resources(RESOURCE_ADDED ${CMAKE_SOURCE_DIR}/translations.qrc)

add_executable(ImageLabeler ${SOURCES} ${HEADERS} ${RESOURCE_ADDED})

target_link_libraries(
        ${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Sql
        pthread
)

# For static linking, link image format plugins
if (STATIC_QT AND QT_QJPEG_PLUGIN AND QT_QPNG_PLUGIN)
    target_link_libraries(${PROJECT_NAME} ${QT_QJPEG_PLUGIN} ${QT_QPNG_PLUGIN})
endif()

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Sql Network)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt5${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
if (APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    # 生成LANShare.app
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.fgsqw.lanshare"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/build/macos/Info.plist"
            MACOSX_BUNDLE_ICON_FILE "build/macos/AppIcon.icns"
    )
    # 复制 AppIcon.icns 到 .app 包中
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/build/macos/AppIcon.icns ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/AppIcon.icns
            COMMENT "Copying AppIcon.icns to the bundle"
    )
    # 打包项目为dmg
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_PREFIX_PATH}/bin/macdeployqt
            ARGS ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROJECT_NAME}.app -dmg
            COMMENT "Running macdeployqt"
    )
endif ()