cmake_minimum_required(VERSION 3.20)
project(nctv-player VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Fix for 'moc' in emulated/cross environments
set(CMAKE_AUTOGEN_COMPILER_PREDEFINES OFF)
set(CMAKE_AUTOGEN_VERBOSE ON)

# Only use hardcoded Windows path if building on Windows and not cross-compiling
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/msvc2022_64")
endif()

# ──────────────────────────────────────────────
# Qt6 Discovery
# ──────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Multimedia
    Widgets
)

# ──────────────────────────────────────────────
# libVLC Discovery (cross-platform)
# ──────────────────────────────────────────────
if(WIN32)
    # Windows: Use bundled VLC SDK from external/ folder
    set(VLC_SDK_DIR "${CMAKE_SOURCE_DIR}/external/vlc-sdk-windows")

    if(NOT EXISTS "${VLC_SDK_DIR}/sdk/include/vlc/vlc.h")
        message(FATAL_ERROR
            "VLC SDK not found at ${VLC_SDK_DIR}.\n"
            "Download the VLC SDK (win64) from https://www.videolan.org/vlc/download-windows.html\n"
            "and extract it into external/vlc-sdk-windows/ so that\n"
            "external/vlc-sdk-windows/sdk/include/vlc/vlc.h exists."
        )
    endif()

    set(VLC_INCLUDE_DIRS "${VLC_SDK_DIR}/sdk/include")
    set(VLC_LIBRARY_DIRS "${VLC_SDK_DIR}/sdk/lib")
    set(VLC_BIN_DIR      "${VLC_SDK_DIR}")

    # Import libvlc and libvlccore as shared libraries
    add_library(libvlc SHARED IMPORTED)
    set_target_properties(libvlc PROPERTIES
        IMPORTED_IMPLIB "${VLC_LIBRARY_DIRS}/libvlc.lib"
        IMPORTED_LOCATION "${VLC_BIN_DIR}/libvlc.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${VLC_INCLUDE_DIRS}"
    )

    add_library(libvlccore SHARED IMPORTED)
    set_target_properties(libvlccore PROPERTIES
        IMPORTED_IMPLIB "${VLC_LIBRARY_DIRS}/libvlccore.lib"
        IMPORTED_LOCATION "${VLC_BIN_DIR}/libvlccore.dll"
    )

else()
    # Linux (Raspberry Pi / Debian): Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBVLC REQUIRED IMPORTED_TARGET libvlc)
    pkg_check_modules(LIBVLCCORE IMPORTED_TARGET libvlccore)
endif()

# ──────────────────────────────────────────────
# Source Files
# ──────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/core/Config.cpp
    src/services/CliService.cpp
    src/services/PidService.cpp
    src/services/PlaylistService.cpp
    src/services/WindowService.cpp
    src/player/ZonePlayer.cpp
    src/utils/VideoOptimizer.cpp
)

set(HEADERS
    include/core/Config.h
    include/core/Models.h
    include/services/CliService.h
    include/services/PidService.h
    include/services/PlaylistService.h
    include/services/WindowService.h
    include/player/ZonePlayer.h
    include/utils/VideoOptimizer.h
)

# ──────────────────────────────────────────────
# QML Resource via qt_add_qml_module
# ──────────────────────────────────────────────
qt_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

qt_add_qml_module(${PROJECT_NAME}
    URI NctvPlayer
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/shared/Styles.qml
        ui/features/splash/Splash.qml
        ui/features/player/PlayerLayout.qml
        ui/features/player/BackgroundZone.qml
        ui/features/player/HorizontalZone.qml
        ui/features/player/MainZone.qml
        ui/features/player/VerticalZone.qml
        ui/features/menu/Menu.qml
    NO_RESOURCE_TARGET_PATH
)

# ──────────────────────────────────────────────
# Include Directories
# ──────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ──────────────────────────────────────────────
# Link Libraries
# ──────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Widgets
)

# Fix for 'moc' parse errors on some GCC/Qt combinations (e.g. stl_relops.h)
# Tells moc to ignore some problematic headers
if(NOT WIN32)
    # Define architecture to avoid moc getting lost in system headers
    set(MOC_ARCH_DEFINES "")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(MOC_ARCH_DEFINES "-D__aarch64__")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        set(MOC_ARCH_DEFINES "-D__arm__")
    endif()

    # CRITICAL: Use APPEND to avoid overwriting flags set by qt_add_qml_module (like --output-json)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
        AUTOMOC_MOC_OPTIONS "--no-notes" ${MOC_ARCH_DEFINES}
    )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE libvlc libvlccore)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::LIBVLC)
    if(LIBVLCCORE_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::LIBVLCCORE)
    endif()
endif()

# ──────────────────────────────────────────────
# Platform-specific settings
# ──────────────────────────────────────────────
# Detection handles armhf (arm), arm64 (aarch64), and various linux architectures
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l" OR NCTV_PLATFORM_PI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NCTV_PLATFORM_PI=1)
    message(STATUS "Building for Raspberry Pi (ARM / ${CMAKE_SYSTEM_PROCESSOR})")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE NCTV_PLATFORM_DESKTOP=1)
    message(STATUS "Building for Desktop")
endif()

# Install target for Debian packaging
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION usr/bin
)

# ──────────────────────────────────────────────
# Windows: Post-Build DLL Copy
# ──────────────────────────────────────────────
if(WIN32)
    # Copy VLC plugins to build directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VLC_SDK_DIR}/plugins"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins"
        COMMENT "Copying VLC plugins..."
    )

    # Deploy local playlist folder for debugging
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/playlist"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../playlist"
        COMMENT "Deploying local playlist for debugging..."
    )

    # Copy all VLC DLLs next to the .exe
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VLC_BIN_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/vlc"
        COMMENT "Copying VLC runtime DLLs to build directory..."
    )

    # Copy the top-level DLLs (libvlc.dll, libvlccore.dll)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VLC_BIN_DIR}/libvlc.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/libvlc.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VLC_BIN_DIR}/libvlccore.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/libvlccore.dll"
        COMMENT "Copying libvlc.dll and libvlccore.dll..."
    )

    # Use windeployqt to gather Qt DLLs
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND Qt6::windeployqt --qmldir "${CMAKE_SOURCE_DIR}/ui"
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt to deploy Qt dependencies..."
    )
endif()
