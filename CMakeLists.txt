cmake_minimum_required(VERSION 3.20)

project(VarjoImg)
set (CMAKE_CXX_STANDARD 17)

# "Detect" platform to set correct imported locations
if(DEFINED ENV{Platform})
  if("$ENV{Platform}" STREQUAL "x86")
    set(_arch "x86")
  elseif("$ENV{Platform}" STREQUAL "x64")
    set(_arch "x86_64")
  endif()
else()
  set(_arch "x86_64")
endif()

# If running on cmake > 3.19, set cmp0111 to old to avoid spamming
# warnings/error to console when generating a project.
if (POLICY CMP0111)
  cmake_policy(SET CMP0111 OLD)
endif()

set(CMAKE_CXX_FLAGS_INIT "/WX (TreatWarningsAsLinkerErrors=false)")

# Add library: Varjo
add_library(VarjoLib SHARED IMPORTED GLOBAL)
set_target_properties(VarjoLib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Varjo/include"
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Varjo/bin/VarjoLib.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Varjo/lib/VarjoLib.lib"
)

# Add library: OpenCV
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenCV/")
add_library(OpenCV SHARED IMPORTED GLOBAL)
set_target_properties(OpenCV PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_core4100.dll"

    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_core4100.lib"
)

add_library(OpenCVDisplay SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVDisplay PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_highgui4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_highgui4100.lib"
)

add_library(OpenCVImgProc SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVImgProc PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_imgproc4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_imgproc4100.lib"
)

add_library(OpenCVCalib3D SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVCalib3D PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_calib3d4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_calib3d4100.lib"
)

add_library(OpenCVOmnidir SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVOmnidir PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_ccalib4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_ccalib4100.lib"
)

add_library(OpenCVVideoIO SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVVideoIO PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_videoio4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_videoio4100.lib"
)

add_library(OpenCVFlann SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVFlann PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_flann4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_flann4100.lib"
)

add_library(OpenCVFeatures2D SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVFeatures2D PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_features2d4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_features2d4100.lib"
)

add_library(OpenCVImgCodecs SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVImgCodecs PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_imgcodecs4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_imgcodecs4100.lib"
)

add_library(OpenCVFiltering SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVFiltering PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_ximgproc4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_ximgproc4100.lib"
)

add_library(OpenCVVideo SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVVideo PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_video4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_video4100.lib"
)

add_library(OpenCVDNN SHARED IMPORTED GLOBAL)
set_target_properties(OpenCVDNN PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_DIR}/include"
    IMPORTED_LOCATION_RELEASE "${OpenCV_DIR}/bin/opencv_dnn4100.dll"
    IMPORTED_IMPLIB_RELEASE "${OpenCV_DIR}/lib/opencv_dnn4100.lib"
)

# Add library: GLM
add_library(GLM INTERFACE)
target_include_directories(GLM INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/GLM/include>
)
add_library(GLM::GLM ALIAS GLM)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(_build_output_dir ${CMAKE_BINARY_DIR}/bin)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${_build_output_dir})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(_src_common_dir ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Varjo/Common)

set(_sources_app
    ${_src_dir}/main.cpp
    ${_src_dir}/AppLogic.hpp
    ${_src_dir}/AppLogic.cpp
    ${_src_dir}/DepthManager.hpp
    ${_src_dir}/DepthManager.cpp
)

set(_sources_common
    ${_src_common_dir}/DataStreamer.hpp
    ${_src_common_dir}/DataStreamer.cpp
    ${_src_common_dir}/Globals.hpp
    ${_src_common_dir}/Globals.cpp
)

source_group("Common" FILES ${_sources_common})

set(_target MRExample)
add_executable(${_target} ${_sources_app} ${_sources_common})

target_include_directories(${_target} PRIVATE ${_src_common_dir})

set_property(TARGET ${_target} PROPERTY FOLDER "Examples")
set_target_properties(${_target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_compile_definitions(${_target} PUBLIC -D_UNICODE -DUNICODE -DNOMINMAX ${VarjoLibDefinitions})

target_link_libraries(${_target} PRIVATE
    GLM
    VarjoLib
    windowscodecs
    OpenCV
    OpenCVDisplay
    OpenCVImgProc
    OpenCVCalib3D
    OpenCVOmnidir
    OpenCVVideoIO
    OpenCVFlann
    OpenCVFeatures2D
    OpenCVImgCodecs
    OpenCVFiltering
    OpenCVVideo
    OpenCVDNN
)


# If we are building to another directory, copy dll files from bin
if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message("Copying DLL files from ${CMAKE_SOURCE_DIR} to ${CMAKE_BINARY_DIR}")
    add_custom_target(copyLibs ALL)
    add_custom_command(
        TARGET copyLibs
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/vendor/Varjo/bin/VarjoLib.dll ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_core4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_highgui4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_imgproc4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_calib3d4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_ccalib4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_videoio4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_flann4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_features2d4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_imgcodecs4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_ximgproc4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_video4100.dll  ${CMAKE_BINARY_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${OpenCV_DIR}/bin/opencv_dnn4100.dll  ${CMAKE_BINARY_DIR}/bin/
    )

    # Add libs to public SDK examples
    add_dependencies(${_target} copyLibs)

endif()
