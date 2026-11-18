cmake_minimum_required(VERSION 3.31.6)
project(UrbanChaosCpuContract LANGUAGES CXX)
if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
 message(FATAL_ERROR "Portability probe requires Android arm64-v8a")
endif()
add_library(urban_cpu_contract STATIC cpu_contract.cpp)
target_compile_features(urban_cpu_contract PRIVATE cxx_std_20)
target_compile_options(urban_cpu_contract PRIVATE -fsigned-char -fno-strict-aliasing)
target_include_directories(urban_cpu_contract PRIVATE ../../vendor/runtime/runtime)

# Compile authored recorded-packet fixture/decoder for the target ISA; execution remains a host test.
add_library(urban_recorded_packet_contract STATIC ../../host/runtime-probes/recorded_push.cpp)
target_compile_features(urban_recorded_packet_contract PRIVATE cxx_std_20)
target_include_directories(urban_recorded_packet_contract PRIVATE ..)
target_compile_options(urban_recorded_packet_contract PRIVATE -UNDEBUG)

add_library(urban_audio_cone_contract STATIC ../../host/runtime-probes/audio_cone.cpp)
target_compile_features(urban_audio_cone_contract PRIVATE cxx_std_20)
target_include_directories(urban_audio_cone_contract PRIVATE ..)
target_compile_options(urban_audio_cone_contract PRIVATE -UNDEBUG)

add_library(urban_audio_spatial_contract STATIC ../../host/runtime-probes/audio_spatial.cpp)
target_compile_features(urban_audio_spatial_contract PRIVATE cxx_std_20)
target_include_directories(urban_audio_spatial_contract PRIVATE ..)
target_compile_options(urban_audio_spatial_contract PRIVATE -UNDEBUG)

# Shared ISO/XBE recognition, compiled for Android; no game content is linked.
if(NOT EXISTS "${URBAN_SOURCE_OVERLAY}/runtime/kernel/disc.cpp")
 message(FATAL_ERROR "A fresh bounded reader overlay is required")
endif()
add_library(urban_game_source_contract STATIC
 ../game/source.cpp "${URBAN_SOURCE_OVERLAY}/runtime/kernel/disc.cpp"
 ../../vendor/runtime/runtime/kernel/crypto.cpp)
target_compile_features(urban_game_source_contract PRIVATE cxx_std_20)
target_include_directories(urban_game_source_contract PRIVATE
 "${URBAN_SOURCE_OVERLAY}/runtime" ../../vendor/runtime/runtime ..)
target_compile_options(urban_game_source_contract PRIVATE -fsigned-char -fno-strict-aliasing)

add_library(urban_diagnostics_contract STATIC ../diagnostics/contract.cpp)
target_compile_features(urban_diagnostics_contract PRIVATE cxx_std_20)
target_include_directories(urban_diagnostics_contract PRIVATE ..)

add_library(urban_android_report_bridge STATIC ../diagnostics/android_bridge.cpp)
target_compile_features(urban_android_report_bridge PRIVATE cxx_std_20)
target_include_directories(urban_android_report_bridge PRIVATE .. "${URBAN_SOURCE_OVERLAY}/runtime" ../../vendor/runtime/runtime)

add_library(urban_sdl_output_contract STATIC ../audio/sdl_output.cpp)
target_compile_features(urban_sdl_output_contract PRIVATE cxx_std_20)
target_include_directories(urban_sdl_output_contract PRIVATE .. ../../vendor/runtime/runtime ../../vendor/SDL/include)
