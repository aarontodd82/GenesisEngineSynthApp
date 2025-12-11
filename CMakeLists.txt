cmake_minimum_required(VERSION 3.16)
project(GenesisEngineSynthApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort)

# Platform-specific MIDI libraries
if(WIN32)
    # Windows: Use RtMidi for MIDI port enumeration
    # Virtual MIDI requires loopMIDI to be installed by user
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(RTMIDI rtmidi)
    endif()

    if(NOT RTMIDI_FOUND)
        # Try to find RtMidi manually or use bundled version
        find_path(RTMIDI_INCLUDE_DIRS rtmidi/RtMidi.h)
        find_library(RTMIDI_LIBRARIES rtmidi)
    endif()

    if(RTMIDI_FOUND OR RTMIDI_LIBRARIES)
        set(MIDI_LIBRARIES ${RTMIDI_LIBRARIES})
        set(MIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIRS})
        add_compile_definitions(USE_RTMIDI)
    else()
        # Fallback: Windows Multimedia API (basic MIDI only, no virtual ports)
        set(MIDI_LIBRARIES winmm)
        add_compile_definitions(USE_WINMM)
    endif()

elseif(APPLE)
    # macOS: CoreMIDI (supports native virtual ports)
    find_library(COREMIDI_LIBRARY CoreMIDI REQUIRED)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    set(MIDI_LIBRARIES ${COREMIDI_LIBRARY} ${COREFOUNDATION_LIBRARY})
    add_compile_definitions(USE_COREMIDI)

elseif(UNIX)
    # Linux: ALSA (supports native virtual ports)
    find_package(ALSA REQUIRED)
    set(MIDI_LIBRARIES ALSA::ALSA)
    add_compile_definitions(USE_ALSA)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/SerialManager.cpp
    src/MIDIManager.cpp
    src/PatchBank.cpp
    src/FileFormats.cpp
    src/FMPatchEditor.cpp
    src/PSGEnvelopeEditor.cpp
    src/AlgorithmWidget.cpp
    src/OperatorWidget.cpp
    src/EnvelopeWidget.cpp
)

set(HEADERS
    src/MainWindow.h
    src/SerialManager.h
    src/MIDIManager.h
    src/PatchBank.h
    src/FileFormats.h
    src/FMPatchEditor.h
    src/PSGEnvelopeEditor.h
    src/AlgorithmWidget.h
    src/OperatorWidget.h
    src/EnvelopeWidget.h
    src/Types.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MIDI_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    ${MIDI_LIBRARIES}
)

# Platform-specific settings
if(WIN32)
    # Windows: Set subsystem to Windows (no console)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    # macOS: Create app bundle
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Genesis Engine Synth"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.fm90s.genesisenginesynth"
    )
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
