cmake_minimum_required(VERSION 3.25)
project(PokemonXAndroidModules LANGUAGES CXX)

if(NOT ANDROID OR NOT PX_MODULES_SOURCE_DIR OR NOT PX_HOST_LIBRARY)
  message(FATAL_ERROR "Pass the Android toolchain, PX_MODULES_SOURCE_DIR and PX_HOST_LIBRARY")
endif()
if(NOT HEXLITH_SOURCE_DIR)
  get_filename_component(HEXLITH_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

# Generated game sources stay outside this checkout. The JNI library is local
# in Android's loader namespace, so symbol presence alone is not sufficient:
# make it a direct DT_NEEDED dependency of the generated module library.
add_subdirectory("${PX_MODULES_SOURCE_DIR}" generated)
add_library(px_android_host SHARED IMPORTED)
set_target_properties(px_android_host PROPERTIES IMPORTED_LOCATION "${PX_HOST_LIBRARY}")
target_link_libraries(hexlith_modules PRIVATE px_android_host)
target_link_options(hexlith_modules PRIVATE -Wl,--no-undefined -Wl,-z,max-page-size=16384)
set_target_properties(hexlith_modules PROPERTIES OUTPUT_NAME pxmodules)
