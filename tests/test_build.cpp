// Tests for csr::build.
//
// A note on a preprocessor trap you will hit the first time you edit these:
// braces do not protect commas from the preprocessor, only parentheses do.
// So CHECK(v == std::vector<int>{0, 1, 2}) does not compile -- the macro sees
// four arguments. Hence the named `expected` locals below, which read better
// anyway.

#include "test_helpers.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

using csr_test::check_invariants;
using csr_test::Edges;
using csr_test::neighbors_of;

TEST_CASE("path graph 0-1-2-3") {
    const Edges edges{{0, 1}, {1, 2}, {2, 3}};
    const csr::Csr m = csr::build(4, edges);

    check_invariants(m, 4, edges);

    // row_ptr *is* part of the contract -- the offsets are what a consumer
    // loops over -- so it gets pinned exactly. Degrees here are 1, 2, 2, 1.
    const std::vector<int> expected_row_ptr{0, 1, 3, 5, 6};
    CHECK(m.row_ptr == expected_row_ptr);

    // Row contents go through neighbors_of, i.e. compared as sorted sets.
    const std::vector<int> row0{1};
    const std::vector<int> row1{0, 2};
    const std::vector<int> row2{1, 3};
    const std::vector<int> row3{2};
    CHECK(neighbors_of(m, 0) == row0);
    CHECK(neighbors_of(m, 1) == row1);
    CHECK(neighbors_of(m, 2) == row2);
    CHECK(neighbors_of(m, 3) == row3);
}

TEST_CASE("empty graph") {
    // n == 0 is the degenerate case that a two-pass build tends to survive by
    // accident: row_ptr is assign(1, 0), the prefix-sum loop never runs, and
    // col_idx resizes to 0. Worth pinning that it stays that way.
    const Edges edges{};
    const csr::Csr m = csr::build(0, edges);

    check_invariants(m, 0, edges);

    const std::vector<int> expected_row_ptr{0};
    CHECK(m.row_ptr == expected_row_ptr);
    CHECK(m.col_idx.empty());
}

TEST_CASE("nodes but no edges") {
    const Edges edges{};
    const csr::Csr m = csr::build(5, edges);

    check_invariants(m, 5, edges);

    const std::vector<int> expected_row_ptr(6, 0);
    CHECK(m.row_ptr == expected_row_ptr);
    CHECK(m.col_idx.empty());

    for (int i = 0; i < 5; ++i) {
        INFO("row ", i);
        CHECK(neighbors_of(m, i).empty());
    }
}

TEST_CASE("a single edge is stored in both directions") {
    const Edges edges{{0, 1}};
    const csr::Csr m = csr::build(2, edges);

    check_invariants(m, 2, edges);

    const std::vector<int> expected_row_ptr{0, 1, 2};
    const std::vector<int> row0{1};
    const std::vector<int> row1{0};
    CHECK(m.row_ptr == expected_row_ptr);
    CHECK(neighbors_of(m, 0) == row0);
    CHECK(neighbors_of(m, 1) == row1);
}

TEST_CASE("star graph") {
    // One node of degree n-1 and n-1 nodes of degree 1. This is the case that
    // leans hardest on the cursor in pass 2: row 0 is written n-1 times while
    // every other row is written once, so an off-by-one in the cursor copy or
    // in the prefix sum shows up here rather than in a uniform graph.
    const int n = 6;
    Edges edges;
    for (int j = 1; j < n; ++j) edges.emplace_back(0, j);

    const csr::Csr m = csr::build(n, edges);

    check_invariants(m, n, edges);

    // Offsets: hub row is [0, 5), then one entry each.
    const std::vector<int> expected_row_ptr{0, 5, 6, 7, 8, 9, 10};
    CHECK(m.row_ptr == expected_row_ptr);

    const std::vector<int> hub{1, 2, 3, 4, 5};
    CHECK(neighbors_of(m, 0) == hub);

    for (int j = 1; j < n; ++j) {
        INFO("leaf ", j);
        const std::vector<int> leaf{0};
        CHECK(neighbors_of(m, j) == leaf);
    }
}

TEST_CASE("isolated nodes leave empty rows") {
    // Node 1 sits between two populated rows and node 4 sits at the end.
    // Interior empty rows catch a prefix sum that skips zero-degree nodes;
    // the trailing one catches a loop that stops at n-1 instead of n.
    const Edges edges{{0, 2}, {2, 3}};
    const csr::Csr m = csr::build(5, edges);

    check_invariants(m, 5, edges);

    const std::vector<int> expected_row_ptr{0, 1, 1, 3, 4, 4};
    CHECK(m.row_ptr == expected_row_ptr);
    CHECK(neighbors_of(m, 1).empty());
    CHECK(neighbors_of(m, 4).empty());
}

TEST_CASE("random graphs agree with a dense reference") {
    // Property-based testing without the framework: generate an input, build
    // it both ways, require agreement. Fifty seeds cover far more shapes than
    // hand-written cases ever will, and this case keeps working through every
    // rewrite on the roadmap -- it only knows about the contract.
    //
    // On failure, doctest prints the captured seed: re-run with that seed
    // alone to reproduce, and shrink the graph by hand from there.
    for (std::uint32_t seed = 0; seed < 50; ++seed) {
        CAPTURE(seed);

        const int n = 1 + static_cast<int>(seed % 17);
        const int edge_count = static_cast<int>(seed % 40);
        const Edges edges = csr_test::random_edges(n, edge_count, seed);

        const csr::Csr m = csr::build(n, edges);
        check_invariants(m, n, edges);

        const std::vector<std::vector<int>> dense = csr_test::dense_adjacency(n, edges);
        for (int i = 0; i < n; ++i) {
            CAPTURE(i);
            CHECK(neighbors_of(m, i) == csr_test::dense_row(dense, i));
        }
    }
}
