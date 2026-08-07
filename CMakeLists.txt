cmake_minimum_required (VERSION 3.30)
include(FetchContent)

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)

            #case sensitive match
            # string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            #
            #case insenstitive match
            string( TOLOWER "${ARGV0}" ARGV0_lower )
            string( TOLOWER "${_variableName}" _variableName_lower )
            string(REGEX MATCH ${ARGV0_lower} MATCHED ${_variableName_lower})

            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("FMTools")

set_property(GLOBAL PROPERTY CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(FALSE)
FetchContent_Declare( ctre 
    GIT_REPOSITORY https://github.com/hanickadot/compile-time-regular-expressions
    GIT_TAG origin/main
    )
FetchContent_MakeAvailable(ctre)
endif()

if(FALSE)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    message(STATUS "Fetching hello_imgui")
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
       set(HELLOIMGUI_USE_GLFW3 ON)
        set(HELLOIMGUI_HAS_OPENGL3 ON)
        FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master)
        set(HELLOIMGUI_WIN32_AUTO_WINMAIN ON)
        set(HELLOIMGUI_FETCH_FORBIDDEN OFF)
        set(HELLOIMGUI_USE_GLFW3 ON)
        FetchContent_MakeAvailable(hello_imgui)
    else()
        set(HELLOIMGUI_USE_GLFW3 ON)
        set(HELLOIMGUI_HAS_OPENGL3 ON)
        FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master)
        set(HELLOIMGUI_FETCH_FORBIDDEN OFF)
        set(HELLOIMGUI_USE_GLFW3 ON)
        set(HELLOIMGUI_HAS_OPENGL3 ON)
        FetchContent_MakeAvailable(hello_imgui)
    endif()
    FetchContent_Declare(implot GIT_REPOSITORY https://github.com/epezent/implot GIT_TAG master)
    FetchContent_MakeAvailable(implot)
    include_directories(${implot_SOURCE_DIR})
    dump_cmake_variables(implot)
endif()
endif()

find_package(Boost 1.89 REQUIRED COMPONENTS system math)
find_package(fmt CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/common)

add_subdirectory("fftlib")
include_directories(${PROJECT_SOURCE_DIR}/fftlib)

add_subdirectory("tools")
add_subdirectory("fmdemod")
