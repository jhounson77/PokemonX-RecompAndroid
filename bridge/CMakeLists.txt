get_filename_component(PX_HEXLITH "${CMAKE_CURRENT_LIST_DIR}/../hexlith" ABSOLUTE)
file(GLOB_RECURSE PX_ARM_SOURCES "${PX_HEXLITH}/src/domain/arm/*.cpp")
add_library(px_native_cpu STATIC ${PX_ARM_SOURCES}
  "${PX_HEXLITH}/src/domain/memory/process_memory.cpp"
  "${PX_HEXLITH}/src/domain/memory/paged_memory.cpp"
  "${PX_HEXLITH}/src/domain/lift/engine.cpp"
  "${PX_HEXLITH}/src/domain/ctr/cro.cpp"
  "${PX_HEXLITH}/src/core/byte_reader.cpp"
  "${PX_HEXLITH}/src/core/text.cpp"
  "${PX_HEXLITH}/src/adapters/lifted/lifted_library.cpp"
  "${PX_HEXLITH}/src/adapters/lifted/modules_library.cpp"
  sha256_cryptopp.cpp)
target_include_directories(px_native_cpu PUBLIC "${PX_HEXLITH}/src")
target_compile_features(px_native_cpu PUBLIC cxx_std_20)
target_compile_options(px_native_cpu PRIVATE -O2 -ffp-contract=off -fvisibility=default)
target_link_libraries(px_native_cpu PUBLIC cryptopp ${CMAKE_DL_LIBS})
target_sources(citra_core PRIVATE "${CMAKE_CURRENT_LIST_DIR}/arm_native.cpp" "${CMAKE_CURRENT_LIST_DIR}/program.cpp")
target_include_directories(citra_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(citra_core PUBLIC px_native_cpu)
if(NOT ANDROID)
 add_executable(px-azahar-hle-smoke tests/hle_smoke.cpp)
 target_compile_definitions(px-azahar-hle-smoke PRIVATE ENABLE_VULKAN ENABLE_SOFTWARE_RENDERER)
 target_include_directories(px-azahar-hle-smoke PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/src" "${PROJECT_SOURCE_DIR}/externals/boost" "${PROJECT_SOURCE_DIR}/externals/fmt/include")
 target_link_libraries(px-azahar-hle-smoke PRIVATE citra_core)
 set_target_properties(px-azahar-hle-smoke PROPERTIES ENABLE_EXPORTS ON)
endif()
if(ANDROID)
 add_library(pxhost SHARED android_host.cpp battle_data.cpp)
 target_include_directories(pxhost PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/src" "${PROJECT_SOURCE_DIR}/externals/boost" "${PROJECT_SOURCE_DIR}/externals/fmt/include")
 target_compile_definitions(pxhost PRIVATE ENABLE_VULKAN)
 target_link_libraries(pxhost PRIVATE citra_core -Wl,--whole-archive px_native_cpu -Wl,--no-whole-archive android log)
 target_link_options(pxhost PRIVATE -Wl,-z,max-page-size=16384 -Wl,--no-undefined)
endif()
if(NOT ANDROID)
 add_executable(px-azahar-inspect tests/inspect_game.cpp)
 target_include_directories(px-azahar-inspect PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_BINARY_DIR}/src" "${PROJECT_SOURCE_DIR}/externals/boost" "${PROJECT_SOURCE_DIR}/externals/fmt/include")
 target_compile_definitions(px-azahar-inspect PRIVATE ENABLE_VULKAN ENABLE_SOFTWARE_RENDERER)
 target_link_libraries(px-azahar-inspect PRIVATE citra_core)
endif()

if(ANDROID)
 foreach(feature ZLIB BZip2 LIBXML2 EXPAT OPENSSL LIBB2 LZMA ZSTD LZ4 ACL XATTR ICONV)
  set(ENABLE_${feature} OFF CACHE BOOL "" FORCE)
 endforeach()
 set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
 set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
 set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
 set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
 set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
 set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
 add_subdirectory("${PROJECT_SOURCE_DIR}/externals/libarchive" "${PROJECT_BINARY_DIR}/px-libarchive" EXCLUDE_FROM_ALL)
 target_sources(pxhost PRIVATE update_archive.cpp)
 target_link_libraries(pxhost PRIVATE archive_static)
endif()
