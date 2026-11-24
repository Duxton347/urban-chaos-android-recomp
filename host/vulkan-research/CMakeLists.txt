cmake_minimum_required(VERSION 3.31.6)
project(UrbanVulkanRendererContract LANGUAGES CXX)
get_filename_component(REPO "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
foreach(INPUT URBAN_VULKAN_SOURCE URBAN_OVERLAY URBAN_VULKAN_HEADERS)
 if(NOT EXISTS "${${INPUT}}")
  message(FATAL_ERROR "Missing fixed renderer contract input ${INPUT}")
 endif()
endforeach()
add_library(urban_vulkan_renderer_contract STATIC "${URBAN_VULKAN_SOURCE}" "${REPO}/native/gpu/vulkan_visibility.cpp")
target_compile_features(urban_vulkan_renderer_contract PRIVATE cxx_std_20)
target_compile_options(urban_vulkan_renderer_contract PRIVATE -O1 -g0)
target_include_directories(urban_vulkan_renderer_contract PRIVATE
 "${CMAKE_CURRENT_SOURCE_DIR}" "${URBAN_OVERLAY}/runtime" "${REPO}/vendor/runtime/runtime"
 "${REPO}/vendor/runtime/harness" "${REPO}/vendor/runtime/harness/compat" "${REPO}/native" "${REPO}/vendor/black-shader/native" "${URBAN_VULKAN_HEADERS}")
if(URBAN_ANDROID_WINDOW)
 if(NOT ANDROID)
  message(FATAL_ERROR "Android SDL window contract requires Android")
 endif()
 target_include_directories(urban_vulkan_renderer_contract BEFORE PRIVATE "${REPO}/native/android" "${REPO}/vendor/SDL/include")
 set(PROFILE "${CMAKE_CURRENT_BINARY_DIR}/authored-profile")
 file(MAKE_DIRECTORY "${PROFILE}")
 file(WRITE "${PROFILE}/image_symbols.h" "#pragma once\n#define IMAGE_RESUMABLE_GUEST 1\n#define IMAGE_XBE \"AuthoredNotAGame.xbe\"\n")
 file(WRITE "${PROFILE}/build_source_profile.h" "#pragma once\n#define URBAN_BUILD_ID \"authored-contract\"\n#define URBAN_BUILD_XBE_SHA256 \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"\n#define URBAN_BUILD_TITLE_ID 1u\n")
 add_library(urban_android_host_contract STATIC
  "${REPO}/native/android/main.cpp" "${REPO}/native/android/input.cpp" "${REPO}/native/android/bridge.cpp"
  "${REPO}/native/audio/sdl_output.cpp" "${REPO}/native/diagnostics/android_bridge.cpp"
  "${URBAN_OVERLAY}/runtime/gpu/render_thread.cpp" "${URBAN_OVERLAY}/runtime/cpu/dispatch.cpp")
 target_compile_features(urban_android_host_contract PRIVATE cxx_std_20)
 target_compile_definitions(urban_android_host_contract PRIVATE XBE_RESUMABLE_GUEST=1 XBE_RAW_VERTICES=1 XBE_NATIVE_COMPONENT_ALIGNMENT=1 XBE_RENDER_THREAD=0)
 target_compile_options(urban_android_host_contract PRIVATE -O1 -g0 -fsigned-char -fno-strict-aliasing -include "${REPO}/native/android/platform_compat.h")
 target_include_directories(urban_android_host_contract PRIVATE "${PROFILE}" "${REPO}/native/android"
  "${URBAN_OVERLAY}/runtime" "${REPO}/vendor/runtime/runtime" "${REPO}/vendor/runtime/harness/compat" "${REPO}/native" "${REPO}/vendor/SDL/include")
endif()
if(NOT ANDROID)
 set(SDK "${REPO}/vendor/runtime")
 add_executable(vulkan_renderer_fixture fixture_main.cpp "${REPO}/host/runtime-probes/visibility.cpp" "${REPO}/host/runtime-probes/fog.cpp" "${REPO}/host/runtime-probes/vertex_inputs.cpp"
  "${SDK}/runtime/gpu/shaders.cpp" "${SDK}/runtime/gpu/texture.cpp"
  "${URBAN_OVERLAY}/runtime/gpu/vertex.cpp" "${URBAN_OVERLAY}/runtime/gpu/render_thread.cpp"
  "${SDK}/runtime/kernel/memory.cpp" "${SDK}/runtime/core/log.cpp" "${SDK}/harness/app_headless.cpp")
 target_compile_features(vulkan_renderer_fixture PRIVATE cxx_std_20)
 target_compile_options(vulkan_renderer_fixture PRIVATE -O1 -UNDEBUG -march=x86-64-v2 -fsigned-char -fno-strict-aliasing)
 target_compile_definitions(vulkan_renderer_fixture PRIVATE XBE_RAW_VERTICES=1 XBE_NATIVE_COMPONENT_ALIGNMENT=1)
 target_include_directories(vulkan_renderer_fixture PRIVATE "${URBAN_OVERLAY}/runtime" "${SDK}/runtime" "${SDK}/harness" "${SDK}/harness/compat" "${REPO}/native")
 find_library(URBAN_VULKAN_LOADER NAMES vulkan libvulkan.so.1 REQUIRED)
 target_link_libraries(vulkan_renderer_fixture PRIVATE urban_vulkan_renderer_contract
  "${REPO}/build/vulkan-shaders/liburban_black_shader.a"
  "${REPO}/build/vulkan-shaders/glslang/glslang/libglslang.a"
  "${REPO}/build/vulkan-shaders/glslang/glslang/libglslang-default-resource-limits.a"
  "${URBAN_VULKAN_LOADER}" pthread)
endif()
