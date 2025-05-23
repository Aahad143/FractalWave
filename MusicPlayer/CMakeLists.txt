cmake_minimum_required(VERSION 3.16)

project(MusicPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)


# Add JUCE as a dependency but prevent re-downloading every time
set(FETCHCONTENT_QUIET OFF) # Show progress
set(FETCHCONTENT_UPDATES_DISCONNECTED ON) # Don't check for updates

set(JUCE_AUDIO_FORMATS_FFMPEG ON CACHE BOOL "Enable FFmpeg audio format" FORCE)

message(STATUS "Top level build dir: ${CMAKE_BINARY_DIR}")
message(STATUS "This CMakeLists build dir: ${CMAKE_CURRENT_BINARY_DIR}")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.5
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE # Store in a local directory
)

# Only necessary JUCE modules
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Don't build JUCE examples")
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Don't build JUCE extras")
set(JUCE_BUILD_TOOLS OFF CACHE BOOL "Don't build JUCE tools")

FetchContent_MakeAvailable(JUCE)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MusicPlayer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
        homepage.h homepage.cpp homepage.ui
        styles/homepage.qss
        CenteredItemDelegate.h
        helper/qsshelper.h helper/sidebarhelper.h
        mediacontroller.h mediacontroller.cpp
        currenttracklistmanager.h currenttracklistmanager.cpp
        audioplayback.h audioplayback.cpp
        CircularBuffer.h
        unitypage.h unitypage.cpp unitypage.ui
        unityembedder.h unityembedder.cpp
        Worker.h
        player.h player.cpp player.ui
        Worker.h
        styles/player.qss styles/sidebar.qss
        settingspage.h settingspage.cpp settingspage.ui
        track.h
        librarymanager.h librarymanager.cpp
        playlist.h playlist.cpp
        addcontentform.h addcontentform.cpp addcontentform.ui
        helper/directoryhelper.h
        TrackItemDelegate.h TrackItemDelegate.cpp
        helper/delegatehelper.h
        displayplaylists.h displayplaylists.cpp displayplaylists.ui
        playlistitemdelegate.h
        ClickableWIdget.h
        playlistlistdelegate.h playlistlistdelegate.cpp
        centeredicondelegate.h
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET MusicPlayer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(MusicPlayer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(MusicPlayer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(
    MusicPlayer PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    Qt${QT_VERSION_MAJOR}::Widgets
)

set(FFMPEG_ROOT "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")
message(STATUS "This ffmpeg build dir: ${FFMPEG_ROOT}")
find_library(AVFORMAT    avformat    PATHS "${FFMPEG_ROOT}/lib")
find_library(AVCODEC     avcodec     PATHS "${FFMPEG_ROOT}/lib")
find_library(AVUTIL      avutil      PATHS "${FFMPEG_ROOT}/lib")
find_library(SWRESAMPLE  swresample  PATHS "${FFMPEG_ROOT}/lib")

target_include_directories(MusicPlayer PRIVATE
  ${FFMPEG_ROOT}/include
)

target_link_libraries(MusicPlayer PRIVATE
    ${AVFORMAT}
    ${AVCODEC}
    ${AVUTIL}
    ${SWRESAMPLE}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MusicPlayer)
endif()
set_target_properties(MusicPlayer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS MusicPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_include_directories(MusicPlayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MusicPlayer)
endif()
