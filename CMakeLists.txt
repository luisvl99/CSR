cmake_minimum_required(VERSION 3.23)

project(csr
    VERSION      0.1.0
    DESCRIPTION  "Header-only C++ builder for CSR (compressed sparse row) adjacency structures"
    HOMEPAGE_URL "https://github.com/luisvl99/CSR"
    LANGUAGES    CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ---- Library ----

add_library(csr INTERFACE)
add_library(csr::csr ALIAS csr)

# C++17 minimum: build() uses structured bindings.
target_compile_features(csr INTERFACE cxx_std_17)

target_sources(csr
    INTERFACE
        FILE_SET  HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES     "${CMAKE_CURRENT_SOURCE_DIR}/include/csr/csr.hpp"
)

# ---- Options ----

# All default to OFF when csr is consumed via add_subdirectory or FetchContent.
option(CSR_BUILD_EXAMPLES "Build the csr examples"                ${PROJECT_IS_TOP_LEVEL})
option(CSR_BUILD_TESTS    "Build the csr tests"                   ${PROJECT_IS_TOP_LEVEL})
option(CSR_INSTALL        "Generate csr install and export rules" ${PROJECT_IS_TOP_LEVEL})

# ---- Examples ----

if(CSR_BUILD_EXAMPLES)
    add_executable(csr_example examples/main.cpp)
    target_link_libraries(csr_example PRIVATE csr::csr)
endif()

# ---- Tests ----

if(CSR_BUILD_TESTS)
    # enable_testing() has to be called from the top-level CMakeLists.txt.
    # Called from tests/ instead, `ctest` run at the build root finds nothing.
    #
    # Deliberately not include(CTest): that defines its own BUILD_TESTING
    # option, defaulting to ON, which would then fight CSR_BUILD_TESTS, and it
    # drags in CDash dashboard machinery this project has no use for.
    enable_testing()
    add_subdirectory(tests)
endif()

# ---- Install ----
#
#     find_package(csr 0.1 REQUIRED)
#     target_link_libraries(app PRIVATE csr::csr)

if(CSR_INSTALL)
    # Header-only, so nothing installed here is architecture-specific:
    # share/cmake/csr rather than lib/cmake/csr.
    set(CSR_CMAKE_DIR "${CMAKE_INSTALL_DATADIR}/cmake/csr")

    install(TARGETS csr
        EXPORT   csrTargets
        FILE_SET HEADERS
    )

    install(EXPORT csrTargets
        FILE        csrTargets.cmake
        NAMESPACE   csr::
        DESTINATION "${CSR_CMAKE_DIR}"
    )

    # ARCH_INDEPENDENT: no compiled artifacts, so the package is usable
    # whatever the consumer's pointer size.
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/csrConfigVersion.cmake"
        VERSION       ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/csrConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/csrConfig.cmake"
        INSTALL_DESTINATION "${CSR_CMAKE_DIR}"
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/csrConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/csrConfigVersion.cmake"
        DESTINATION "${CSR_CMAKE_DIR}"
    )
endif()
