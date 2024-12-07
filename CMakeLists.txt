project(VarjoExamples)
cmake_minimum_required(VERSION 3.12)
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
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/VarjoLib.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/VarjoLib.lib"
)

# Add library: OpenVR
add_library(OpenVR SHARED IMPORTED GLOBAL)
set_target_properties(OpenVR PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/include"
    IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/bin/${_arch}/Debug/openvr_api.dll"
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/bin/${_arch}/Release/openvr_api.dll"
    IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/bin/${_arch}/RelWithDebInfo/openvr_api.dll"
    IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/lib/${_arch}/Debug/openvr_api.lib"
    IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/lib/${_arch}/Release/openvr_api.lib"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/vendor/OpenVR/lib/${_arch}/RelWithDebInfo/openvr_api.lib"
)
add_library(OpenVR::OpenVR ALIAS OpenVR)

# Add library: GLEW
add_library(GLEW STATIC  IMPORTED GLOBAL)
set_target_properties(GLEW PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Glew/include"
    IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Glew/lib/${_arch}/Debug/libglew32d.lib"
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Glew/lib/${_arch}/Release/libglew32.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/vendor/Glew/lib/${_arch}/RelWithDebInfo/libglew32.lib"
    INTERFACE_COMPILE_DEFINITIONS "GLEW_STATIC"
)
add_library(Glew::Glew ALIAS GLEW)

# Add library: ImGui
add_library(ImGui SHARED IMPORTED GLOBAL)
set_target_properties(ImGui PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/include"
    IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/lib/${_arch}/Debug/ImGui.lib"
    IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/lib/${_arch}/Release/ImGui.lib"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/lib/${_arch}/RelWithDebInfo/ImGui.lib"
)
add_library(ImGui::ImGui ALIAS ImGui)

# Add library: ImGuiBackends
add_library(ImGuiBackends SHARED IMPORTED GLOBAL)
set_target_properties(ImGuiBackends PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/include"
    IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/lib/${_arch}/Debug/ImGuiBackends.lib"
    IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/lib/${_arch}/Release/ImGuiBackends.lib"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGui/lib/${_arch}/RelWithDebInfo/ImGuiBackends.lib"
)
add_library(ImGui::ImGuiBackends ALIAS ImGuiBackends)

# Add library: FreeType
add_library(FreeType SHARED IMPORTED GLOBAL)
set_target_properties(FreeType PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FreeType/include"
    IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FreeType/lib/${_arch}/Debug/freetyped.lib"
    IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FreeType/lib/${_arch}/Release/freetype.lib"
    IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FreeType/lib/${_arch}/RelWithDebInfo/freetype.lib"
)
add_library(FreeType::FreeType ALIAS FreeType)

# Add library: GLM
add_library(GLM INTERFACE)
target_include_directories(GLM INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/GLM/include>
)
add_library(GLM::GLM ALIAS GLM)

# Add library: JSON
add_library(JSON INTERFACE)
target_include_directories(JSON INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/json/include>
)
add_library(JSON::JSON ALIAS JSON)

# Add library: CxxOpts
add_library(CxxOpts INTERFACE)
target_include_directories(CxxOpts INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/cxxopts/include>
)
add_library(CxxOpts::CxxOpts ALIAS CxxOpts)

# Add library: D3DX12
add_library(D3DX12 INTERFACE)
target_include_directories(D3DX12 INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/D3DX12/include>
)
add_library(D3DX12::D3DX12 ALIAS D3DX12)


set(_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(_src_common_dir ${CMAKE_CURRENT_SOURCE_DIR}/Common)

file(
    GLOB _source_list
    LIST_DIRECTORIES false
    ${_src_dir}/main.cpp
)

file(
    GLOB _source_list_common
    LIST_DIRECTORIES false
    ${_src_common_dir}/CameraManager.hpp
    ${_src_common_dir}/CameraManager.cpp
    ${_src_common_dir}/D3D11MultiLayerView.hpp
    ${_src_common_dir}/D3D11MultiLayerView.cpp
    ${_src_common_dir}/D3D11Renderer.hpp
    ${_src_common_dir}/D3D11Renderer.cpp
    ${_src_common_dir}/D3D11Shaders.hpp
    ${_src_common_dir}/D3D11Shaders.cpp
    ${_src_common_dir}/DataStreamer.hpp
    ${_src_common_dir}/DataStreamer.cpp
    ${_src_common_dir}/ExampleShaders.hpp
    ${_src_common_dir}/GfxContext.hpp
    ${_src_common_dir}/GfxContext.cpp
    ${_src_common_dir}/Globals.hpp
    ${_src_common_dir}/Globals.cpp
    ${_src_common_dir}/MultiLayerView.hpp
    ${_src_common_dir}/MultiLayerView.cpp
    ${_src_common_dir}/Renderer.hpp
    ${_src_common_dir}/Renderer.cpp
    ${_src_common_dir}/Scene.hpp
    ${_src_common_dir}/Scene.cpp
    ${_src_common_dir}/SyncView.hpp
    ${_src_common_dir}/SyncView.cpp
    ${_src_common_dir}/UI.hpp
    ${_src_common_dir}/UI.cpp
    ${_src_common_dir}/Undistorter.hpp
    ${_src_common_dir}/Undistorter.cpp
)

source_group("Common" FILES ${_source_list_common})

set(_target HelloFrame)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(ZMQ REQUIRED libzmq)

# Create the executable
add_executable(${_target} ${_source_list} ${_source_list_common})

# Set include directories
target_include_directories(${_target} PRIVATE ${_src_common_dir})

# Set target properties
set_property(TARGET ${_target} PROPERTY FOLDER "Examples")
set_target_properties(${_target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Define compile definitions
target_compile_definitions(${_target} PUBLIC -D_UNICODE -DUNICODE -DNOMINMAX)

set(CPPZMQ_BUILD_TESTS OFF)
set(BUILD_TESTS OFF)

add_subdirectory(vendor/libzmq)
add_subdirectory(cppzmq)

# Link libraries
target_link_libraries(${_target}
    PRIVATE ImGui::ImGui
    PRIVATE ImGui::ImGuiBackends
    PRIVATE FreeType::FreeType
    PRIVATE GLM
    PRIVATE CxxOpts
    PRIVATE VarjoLib
    PRIVATE d3d11
    PRIVATE dxgi
    PRIVATE d3dcompiler
    PRIVATE windowscodecs
    PRIVATE cppzmq
)

add_custom_command(
    TARGET ${_target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/bin/VarjoLib.dll ${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/vendor/OpenVR/bin/${_arch}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>/openvr_api.dll  ${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/vendor/libzmq/bin/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>/libzmq-v143-mt-gd-4_3_6.dll  ${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>/
)
