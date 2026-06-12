# Copyright (C) 2026 Honu Robotics
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)
project(EncinoWaves VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Upstream code: partial template instantiation leaves some statics unused;
# the EWAV_* macro redefinitions are fixed at source with #undefs in
# EncinoWaves/Exception.h.
add_compile_options(-Wno-unused-function)

find_package(Eigen3 REQUIRED NO_MODULE)   # header-only; FFT via unsupported/Eigen/FFT
find_package(TBB    REQUIRED)             # parallel sweeps in Propagation.h
find_package(Imath  REQUIRED)             # BSD-3 math types (OpenEXR project)

add_library(${PROJECT_NAME} SHARED
  src/Foundation.cpp
  src/Exception.cpp
  src/util/Exception.cpp)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION     ${PROJECT_VERSION}
  SOVERSION   ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen TBB::tbb Imath::Imath)

# ---- install: headers, target, and the CMake package config ----
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${PROJECT_NAME}
  EXPORT   ${PROJECT_NAME}Targets
  LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(_cfgdir ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${_cfgdir})

configure_package_config_file(
  cmake/${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${_cfgdir})
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${_cfgdir})

# ---- headless smoke tests (drivers exit non-zero on failure) ----
if(BUILD_TESTING)
  foreach(t test_FftwWrapper test_headers test_Propagation)
    add_executable(${t} test/${t}.cc)
    target_link_libraries(${t} PRIVATE ${PROJECT_NAME})
    add_test(NAME ${t} COMMAND ${t})
  endforeach()
endif()

# ---- optional FFT microbenchmark (off by default; needs Google Benchmark) ----
option(ENCINOWAVES_BUILD_BENCHMARKS
       "Build the FFT microbenchmark (requires Google Benchmark)" OFF)
if(ENCINOWAVES_BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)
  add_executable(bench_FftwWrapper benchmark/bench_FftwWrapper.cc)
  target_link_libraries(bench_FftwWrapper
    PRIVATE ${PROJECT_NAME} benchmark::benchmark)
endif()
