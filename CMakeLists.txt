cmake_minimum_required(VERSION 3.8)

project(offscreen_vulkan)

include(cmake/check-compiler.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -D_RELEASE")

set(
	SOURCE_FILES
        main.cpp
        source/common.cpp
        source/object.cpp
        source/shader.cpp
        source/renderer.cpp
        source/fileio/file_codec.cpp
        source/fileio/file_encoder.cpp
        source/fileio/video_writer.cpp
)

include_directories("include")
include(cmake/add-libraries-linux.cmake)

add_executable(offscreen_vulkan ${SOURCE_FILES})

include(cmake/target-link-libraries-linux.cmake)

target_include_directories(offscreen_vulkan PUBLIC ${CMAKE_BINARY_DIR})

configure_file(include/project_constants.h.in ${PROJECT_BINARY_DIR}/project_constants.h @ONLY)

#==============================================================================
# COMPILE SHADERS
#==============================================================================
find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
file(GLOB SHADERS
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
)

add_custom_command(
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_SOURCE_DIR}
  OUTPUT ${SHADER_SOURCE_DIR}
)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable} -o ${SHADER_SOURCE_DIR}/${FILENAME}.spv ${source}
    OUTPUT ${SHADER_SOURCE_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_SOURCE_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_SOURCE_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})