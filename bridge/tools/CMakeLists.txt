cmake_minimum_required(VERSION 3.25)
project(PXUpdateLifter LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
get_filename_component(H "${CMAKE_CURRENT_SOURCE_DIR}/../../hexlith/src" ABSOLUTE)
find_package(OpenSSL REQUIRED)
file(GLOB_RECURSE ARM "${H}/domain/arm/*.cpp")
file(GLOB ANALYSIS "${H}/domain/analysis/*.cpp")
file(GLOB LIFT "${H}/domain/lift/*.cpp")
file(GLOB LINKER "${H}/domain/linker/*.cpp")
add_executable(px-lift-update lift_update.cpp ${ARM} ${ANALYSIS} ${LIFT} ${LINKER}
 "${H}/application/analyze.cpp" "${H}/application/analyze_modules.cpp" "${H}/application/analysis_tables.cpp" "${H}/application/lift_emit.cpp"
 "${H}/core/byte_reader.cpp" "${H}/core/text.cpp" "${H}/core/sha256.cpp"
 "${H}/domain/ctr/cro.cpp" "${H}/domain/ctr/cro_link_tables.cpp" "${H}/domain/profile/pokemon_x.cpp"
 "${H}/adapters/filesystem/directory_blob_sink.cpp" "${H}/domain/memory/process_memory.cpp" "${H}/domain/memory/paged_memory.cpp")
target_include_directories(px-lift-update PRIVATE "${H}")
target_link_libraries(px-lift-update PRIVATE OpenSSL::Crypto)
target_compile_options(px-lift-update PRIVATE -O2 -ffunction-sections -fdata-sections)
target_link_options(px-lift-update PRIVATE -Wl,--gc-sections)
