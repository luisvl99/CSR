// doctest's entry point, and nothing else.
//
// This macro expands to the whole test-runner implementation plus main(), and
// it must appear in exactly one translation unit. Keeping it alone in its own
// file means the test cases live in files that compile fast, and adding a new
// test_*.cpp never raises the question of where main() went.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
