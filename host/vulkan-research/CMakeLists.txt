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
if(NOT ANDROID)
 set(SDK "${REPO}/vendor/runtime")
 add_executable(vulkan_renderer_fixture fixture_main.cpp "${REPO}/host/runtime-probes/visibility.cpp"
  "${SDK}/runtime/gpu/shaders.cpp" "${SDK}/runtime/gpu/texture.cpp"
  "${URBAN_OVERLAY}/runtime/gpu/vertex.cpp" "${URBAN_OVERLAY}/runtime/gpu/render_thread.cpp"
  "${SDK}/runtime/kernel/memory.cpp" "${SDK}/runtime/core/log.cpp" "${SDK}/harness/app_headless.cpp")
 target_compile_features(vulkan_renderer_fixture PRIVATE cxx_std_20)
 target_compile_options(vulkan_renderer_fixture PRIVATE -O1 -UNDEBUG -march=x86-64-v2 -fsigned-char -fno-strict-aliasing)
 target_compile_definitions(vulkan_renderer_fixture PRIVATE XBE_RAW_VERTICES=0)
 target_include_directories(vulkan_renderer_fixture PRIVATE "${URBAN_OVERLAY}/runtime" "${SDK}/runtime" "${SDK}/harness" "${SDK}/harness/compat")
 find_library(URBAN_VULKAN_LOADER NAMES vulkan libvulkan.so.1 REQUIRED)
 target_link_libraries(vulkan_renderer_fixture PRIVATE urban_vulkan_renderer_contract
  "${REPO}/build/vulkan-shaders/liburban_black_shader.a"
  "${REPO}/build/vulkan-shaders/glslang/glslang/libglslang.a"
  "${REPO}/build/vulkan-shaders/glslang/glslang/libglslang-default-resource-limits.a"
  "${URBAN_VULKAN_LOADER}" pthread)
endif()
