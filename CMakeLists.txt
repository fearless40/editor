cmake_minimum_required(VERSION 3.5...4.1)


### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

project(CppEditor
     VERSION 0.1
     DESCRIPTION "Cpp based text editor, No RUST."
     LANGUAGES CXX)

set(CMAKE_CXX_STANDARD  26)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    CHECK_COMPILERER_FLAG("/std:c++23" _cpp_20_flag_supported)
    if (_cpp_20_flag_supported)
        add_compile_options("/std:c++23")
    endif()
    add_compile_options("/WX" "/MP")
else()
    add_compile_options("--std=c++26" "-fconcepts-diagnostics-depth=10")

endif()

# configure_file(
#    cmakegen/version.hpp.in
#    include/version.hpp
# )
#
set( ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
set( ROOT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")


# Ensure compile_comamnds.json copied to root
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy compile_commands.json to the source root after generation/build
add_custom_target(export_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json"
)



# add_subdirectory(external)
add_subdirectory( source )
# add_subdirectory(extern)
# add_subdirectory(tests)
