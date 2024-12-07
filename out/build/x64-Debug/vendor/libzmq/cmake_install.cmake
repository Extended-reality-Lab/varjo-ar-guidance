# Install script for directory: C:/Users/Students/Downloads/HelloZmq/vendor/libzmq

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/Students/Downloads/HelloZmq/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/libzmq.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/lib/libzmq-mt-gd-4_3_6.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/bin/libzmq-mt-gd-4_3_6.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "SDK" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/Students/Downloads/HelloZmq/vendor/libzmq/include/zmq.h"
    "C:/Users/Students/Downloads/HelloZmq/vendor/libzmq/include/zmq_utils.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/lib/libzmq-mt-sgd-4_3_6.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "SDK" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/Students/Downloads/HelloZmq/vendor/libzmq/include/zmq.h"
    "C:/Users/Students/Downloads/HelloZmq/vendor/libzmq/include/zmq_utils.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "SDK" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE FILE OPTIONAL FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/bin/libzmq-mt-gd-4_3_6.pdb")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/bin/libzmq-mt-gd-4_3_6.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "C:/Users/Students/Downloads/HelloZmq/vendor/libzmq/include/zmq.h"
    "C:/Users/Students/Downloads/HelloZmq/vendor/libzmq/include/zmq_utils.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/zmq" TYPE FILE FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/AUTHORS.txt")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/zmq" TYPE FILE FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/LICENSE.txt")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/zmq" TYPE FILE FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/NEWS.txt")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/CMake/ZeroMQTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/CMake/ZeroMQTargets.cmake"
         "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/CMakeFiles/Export/df49adab93b9e0c10c64f72458b31971/ZeroMQTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/CMake/ZeroMQTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/CMake/ZeroMQTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/CMake" TYPE FILE FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/CMakeFiles/Export/df49adab93b9e0c10c64f72458b31971/ZeroMQTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/CMake" TYPE FILE FILES "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/CMakeFiles/Export/df49adab93b9e0c10c64f72458b31971/ZeroMQTargets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/CMake" TYPE FILE FILES
    "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/ZeroMQConfig.cmake"
    "C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/ZeroMQConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/msvcp140d.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/msvcp140_1d.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/msvcp140_2d.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/msvcp140d_atomic_wait.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/msvcp140d_codecvt_ids.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/vcruntime140_1d.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/vcruntime140d.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.42.34226/Debug_NonRedist/x64/Microsoft.VC143.DebugCRT/concrt140d.dll"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/ucrt/ucrtbased.dll"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE DIRECTORY FILES "")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/tests/cmake_install.cmake")
  include("C:/Users/Students/Downloads/HelloZmq/out/build/x64-Debug/vendor/libzmq/unittests/cmake_install.cmake")

endif()

