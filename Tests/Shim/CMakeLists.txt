cmake_minimum_required(VERSION 3.20)
project(vrm_sdk_shim LANGUAGES CXX)

set(VRM_SDK_DIR "${CMAKE_SOURCE_DIR}/../../Plugins/VrmToolchain/Source/ThirdParty/VrmSdk")
set(INC "${VRM_SDK_DIR}/include")
set(LIB_REL "${VRM_SDK_DIR}/lib/Win64/Release")

if(NOT EXISTS "${INC}")
  message(FATAL_ERROR "Missing include dir: ${INC}. Run Scripts/FetchVrmSdk.ps1 first.")
endif()

if(NOT EXISTS "${LIB_REL}")
  message(FATAL_ERROR "Missing Release libs dir: ${LIB_REL}. Run Scripts/FetchVrmSdk.ps1 first.")
endif()

set(REQUIRED_LIBS
  "${LIB_REL}/vrm_glb_parser.lib"
  "${LIB_REL}/vrm_normalizers.lib"
  "${LIB_REL}/vrm_validate.lib"
)

foreach(L IN LISTS REQUIRED_LIBS)
  if(NOT EXISTS "${L}")
    message(FATAL_ERROR "Missing required library: ${L}")
  endif()
endforeach()

add_executable(vrm_sdk_shim main.cpp)
target_compile_features(vrm_sdk_shim PRIVATE cxx_std_17)
target_include_directories(vrm_sdk_shim PRIVATE "${INC}")
target_link_libraries(vrm_sdk_shim PRIVATE ${REQUIRED_LIBS})

if(MSVC)
  # UE-aligned CRT
  set_property(TARGET vrm_sdk_shim PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()
