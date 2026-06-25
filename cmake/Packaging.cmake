# Copyright (C) 2026 Honu Robotics
#
# Debian (.deb) packaging via CPack, component-based so we ship the standard
# library split:
#   libencinowaves1      runtime: libEncinoWaves.so.1, libEncinoWaves.so.1.0.0
#   libencinowaves-dev   headers, libEncinoWaves.so symlink, CMake package config
#
# Build the packages with (umask 022 keeps directory perms at 0755):
#   umask 022
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
#   cmake --build build -j"$(nproc)"
#   ( cd build && cpack -G DEB )
#
# CMAKE_INSTALL_PREFIX=/usr is required, not optional. GNUInstallDirs only
# expands CMAKE_INSTALL_LIBDIR to the Debian multiarch path 

set(CPACK_PACKAGE_NAME      "encinowaves")
set(CPACK_PACKAGE_VENDOR    "Honu Robotics")
set(CPACK_PACKAGE_CONTACT   "Honu Robotics <info@honurobotics.com>")
set(CPACK_PACKAGE_VERSION   "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Headless spectral ocean-wave synthesis C++ library")

set(CPACK_GENERATOR "DEB")
# Debian packages live under /usr (the DEB generator's default, pinned here for
# clarity). Pair with -DCMAKE_INSTALL_PREFIX=/usr at configure time so the
# multiarch libdir matches; see the header comment above.
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_COMPONENTS_ALL runtime dev)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)            # libencinowaves1_1.0.0-1_amd64.deb
set(CPACK_DEBIAN_PACKAGE_RELEASE 1)
set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/HonuRobotics/encinowaves")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)            # auto runtime deps via dpkg-shlibdeps
set(CPACK_STRIP_FILES TRUE)                       # strip the shared object
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)   # 0755 maintainer scripts

# Provide a versioned shlibs file so downstream CPack/dpkg-shlibdeps consumers can
# resolve a dependency on this library.
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")

# ---- runtime package: the shared object ----
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME    "libencinowaves1")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "libs")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION
    "Spectral ocean-wave synthesis library (Tessendorf-style FFT), headless\nwith no OpenGL viewer, for embedding in simulators and tools. This\npackage contains the shared runtime library.")
# Route ldconfig through a modern dpkg trigger. The postinst/postrm are supplied
# (rather than letting CPack auto-generate ones that call ldconfig directly) so the
# trigger is the single ldconfig mechanism; they intentionally do nothing else.
set(CPACK_DEBIAN_RUNTIME_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/triggers;${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/postrm")

# ---- dev package: headers, .so symlink, CMake config ----
set(CPACK_DEBIAN_DEV_PACKAGE_NAME    "libencinowaves-dev")
set(CPACK_DEBIAN_DEV_PACKAGE_SECTION "libdevel")
set(CPACK_COMPONENT_DEV_DEPENDS runtime)          # -> libencinowaves1 (= exact version)
# The PUBLIC link deps surface in the installed headers and in
# EncinoWavesConfig.cmake's find_dependency() calls, so downstream builds need
# their -dev packages. Floors reuse EW_*_MIN from the top-level CMakeLists.txt
# so the packaging metadata can never drift from the build-time requirements.
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS
    "libeigen3-dev (>= ${EW_EIGEN3_MIN}), libtbb-dev (>= ${EW_TBB_MIN}), libimath-dev (>= ${EW_IMATH_MIN})")
set(CPACK_COMPONENT_DEV_DESCRIPTION
    "Spectral ocean-wave synthesis library (Tessendorf-style FFT), headless\nwith no OpenGL viewer, for embedding in simulators and tools. This\npackage contains the development headers and the CMake package config.")

# ---- Debian changelog (lintian requires a compressed changelog.Debian) ----
find_program(GZIP_TOOL gzip REQUIRED)
set(_changelog_gz "${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian.gz")

# The changelog is hand-maintained; fail loudly if its top entry drifts from
# the version we are actually packaging (PROJECT_VERSION-PACKAGE_RELEASE).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/changelog.Debian"
  _changelog_first LIMIT_COUNT 1)
if(NOT _changelog_first MATCHES "^encinowaves \\(([^)]+)\\)")
  message(FATAL_ERROR
    "Cannot parse version from changelog.Debian first line: '${_changelog_first}'")
endif()
set(_expected_changelog_version "${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}")
if(NOT CMAKE_MATCH_1 STREQUAL _expected_changelog_version)
  message(FATAL_ERROR
    "changelog.Debian version (${CMAKE_MATCH_1}) does not match the package "
    "version (${_expected_changelog_version}); update cmake/deb/changelog.Debian.")
endif()
execute_process(
  COMMAND ${GZIP_TOOL} -9nc "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/changelog.Debian"
  OUTPUT_FILE "${_changelog_gz}")
install(FILES "${_changelog_gz}"
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/libencinowaves1    COMPONENT runtime)
install(FILES "${_changelog_gz}"
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/libencinowaves-dev COMPONENT dev)

# ---- lintian overrides for documented, internal-only acceptable tags ----
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/lintian-overrides-runtime"
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/lintian/overrides
  RENAME libencinowaves1 COMPONENT runtime)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb/lintian-overrides-dev"
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/lintian/overrides
  RENAME libencinowaves-dev COMPONENT dev)

include(CPack)   # MUST be last: consumes the CPACK_* variables set above
