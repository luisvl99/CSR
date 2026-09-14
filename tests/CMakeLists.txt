# ---- doctest ----
#
# Prefer a doctest already on the machine; fall back to fetching a pinned tag.
# Both live inside this file, which the parent only adds when CSR_BUILD_TESTS
# is ON, so a consumer pulling csr in via FetchContent never downloads doctest
# and never builds any of this.
#
# CMake 3.24+ can express the same thing as a single FetchContent_Declare with
# FIND_PACKAGE_ARGS; this project's floor is 3.23, so it is spelled out.

find_package(doctest 2.4 QUIET)

if(NOT doctest_FOUND)
    include(FetchContent)

    # doctest ships install rules of its own. Without this they get folded
    # into csr's install step, and `cmake --install` would scatter doctest
    # headers into the consumer's prefix alongside ours.
    set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "")

    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.5.3    # a tag, never a branch: CI has to be reproducible
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(doctest)
endif()

# ---- Test executable ----
#
# main.cpp holds nothing but doctest's entry point, so adding a new
# test_*.cpp below never means touching it, and the one slow translation
# unit is compiled once and then cached by the build system.

add_executable(csr_tests
    main.cpp
    test_build.cpp
)

# csr::csr already propagates cxx_std_17; the tests need nothing beyond it.
target_link_libraries(csr_tests PRIVATE csr::csr doctest::doctest)

# One CTest entry running the whole binary. doctest also ships
# doctest_discover_tests(), which runs the binary at build time to enumerate
# TEST_CASEs and registers one CTest entry each -- better `ctest -j` and
# finer-grained reporting, at the cost of an extra moving part. Worth adding
# once the suite is big enough to care.
add_test(NAME csr_tests COMMAND csr_tests)
