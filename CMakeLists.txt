cmake_minimum_required(VERSION 3.28...3.30)

# ArVisual can be built in two ways:
# 1) Inside the official obs-plugintemplate scaffold, where cmake/common exists.
# 2) Directly against an installed libobs CMake package.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)
  project(${_name} VERSION ${_version} LANGUAGES C CXX)

  option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
  option(ENABLE_QT "Use Qt functionality" OFF)

  # Required by the official OBS plugin template.
  # - compilerconfig/defaults/helpers set up MSVC flags, install layout, resources,
  #   and the buildspec-driven OBS dependency bootstrap.
  # - defaults includes the platform buildspec module which downloads obs-deps,
  #   OBS sources, builds libobs Development components, and populates
  #   CMAKE_PREFIX_PATH before find_package(libobs REQUIRED).
  include(compilerconfig)
  include(defaults)
  include(helpers)
else()
  project(arvisual VERSION 0.5.8 LANGUAGES C CXX)
  option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
  option(ENABLE_QT "Use Qt functionality" OFF)
endif()

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/arvisual-filter.cpp
)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(COMMAND set_target_properties_plugin)
  set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
else()
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "" OUTPUT_NAME ${CMAKE_PROJECT_NAME})
  install(TARGETS ${CMAKE_PROJECT_NAME}
    RUNTIME DESTINATION "obs-plugins/64bit"
    LIBRARY DESTINATION "obs-plugins/64bit")
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/" DESTINATION "data/obs-plugins/${CMAKE_PROJECT_NAME}")
endif()
