cmake_minimum_required(VERSION 3.21)
project (Val3dity VERSION 0.3.0)

if(PROJECT_IS_TOP_LEVEL)
  find_package(geoflow REQUIRED)
  include_directories(${geoflow_INCLUDE_DIRS})
endif()

# get val3dity
set(GF_GIT_SUBMODULES val3dity)
if(EXISTS "${PROJECT_SOURCE_DIR}/.gitmodules")
  execute_process(
    COMMAND             git submodule update --init ${GF_GIT_SUBMODULES}
    WORKING_DIRECTORY   ${PROJECT_SOURCE_DIR}/thirdparty
    )
endif()

add_subdirectory(thirdparty/val3dity)

set(GF_PLUGIN_NAME ${PROJECT_NAME})
set(GF_PLUGIN_TARGET_NAME "gfp_val3dity")
set(GF_PLUGIN_REGISTER ${PROJECT_SOURCE_DIR}/register.hpp)
geoflow_create_plugin(
  nodes.cpp
)

# target_include_directories(gfp_val3dity PRIVATE 
#   thirdparty/val3dity/src
#   thirdparty/val3dity/thirdparty
#   )
set_property(TARGET gfp_val3dity PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(gfp_val3dity PRIVATE geoflow-core val3dity)

if (MSVC)
  # collect dll's required for runtime
  INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Release/ DESTINATION bin
    FILES_MATCHING 
    PATTERN "*.dll"
    PATTERN "gfp*" EXCLUDE)
endif()