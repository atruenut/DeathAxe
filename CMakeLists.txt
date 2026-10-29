# DeathAxe - CMake build using the Steinberg VST3 SDK (Windows / macOS / Linux).
#   cmake -S . -B build -DVST3_SDK_ROOT=/path/to/vst3sdk -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
# The .vst3 bundle appears in build/VST3/Release/ (and is linked into the user VST3 folder on macOS/Linux).
cmake_minimum_required(VERSION 3.25)
project(DeathAxe VERSION 1.2.0 LANGUAGES CXX)

set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vst3sdk" CACHE PATH "Path to the VST3 SDK checkout")
if(NOT EXISTS "${VST3_SDK_ROOT}/pluginterfaces")
  message(FATAL_ERROR "VST3 SDK not found at ${VST3_SDK_ROOT}. git clone --recurse-submodules https://github.com/steinbergmedia/vst3sdk")
endif()

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
add_subdirectory(${VST3_SDK_ROOT} ${CMAKE_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(DeathAxe src/vst3/deathaxe_vst3.cpp)
set_target_properties(DeathAxe PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(DeathAxe PRIVATE sdk)
if(SMTG_MAC)
  smtg_target_set_bundle(DeathAxe BUNDLE_IDENTIFIER com.deathaxe.vst3 COMPANY_NAME "DeathAxe")
elseif(SMTG_WIN)
  target_sources(DeathAxe PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp)
endif()

# portable offline tools (no SDK needed)
add_executable(da_render tools/render.cpp)
add_executable(da_stress tools/stress.cpp)
set_target_properties(da_render da_stress PROPERTIES CXX_STANDARD 17)
