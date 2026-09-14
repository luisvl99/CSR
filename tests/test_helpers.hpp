#pragma once

// Shared test utilities: the invariant checker every case calls, the sorted
// row accessor every case compares through, and the dumb reference builder
// the randomised case compares against.

#include <csr/csr.hpp>
#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace csr_test {

using Edges = std::vector<std::pair<int, int>>;

// ---------------------------------------------------------------------------
// Reading a row
// ---------------------------------------------------------------------------

// Row i's neighbours, sorted.
//
// Every case compares rows through this rather than touching col_idx
// directly. The order *within* a row is an artefact of the build -- today it
// is edge-list order -- and is not part of the contract. Sorting here means
// these tests survive the day build() starts emitting sorted rows, while
// still failing if an entry is genuinely wrong or missing. Pinning the raw
// col_idx vector instead would couple every case to an implementation detail
// the roadmap intends to change.
inline std::vector<int> neighbors_of(const csr::Csr& m, int i) {
    std::vector<int> row(m.col_idx.begin() + m.row_ptr[i],
                         m.col_idx.begin() + m.row_ptr[i + 1]);
    std::sort(row.begin(), row.end());
    return row;
}

// ---------------------------------------------------------------------------
// Invariants
// ---------------------------------------------------------------------------

// Properties that must hold for *any* input. This is the real specification
// of the structure, so every case calls it: a case that only checks its own
// expected numbers will not notice a rewrite that quietly breaks symmetry or
// lets an index escape the matrix.
inline void check_invariants(const csr::Csr& m, int n, const Edges& edges) {
    // REQUIRE, not CHECK: everything below indexes row_ptr, so continuing
    // past a wrong size would be undefined behaviour rather than a second
    // failure message. CHECK records and carries on; REQUIRE abandons the
    // case. Use REQUIRE whenever continuing is unsafe or meaningless.
    REQUIRE(m.row_ptr.size() == static_cast<std::size_t>(n) + 1);

    CHECK(m.row_ptr.front() == 0);

    // Offsets never go backwards -- equal means an empty row.
    for (int i = 0; i < n; ++i) {
        INFO("row ", i);
        CHECK(m.row_ptr[i] <= m.row_ptr[i + 1]);
    }

    // The final offset is the entry count, and each undirected edge is
    // stored in both directions.
    CHECK(m.row_ptr[n] == static_cast<int>(m.col_idx.size()));
    CHECK(m.col_idx.size() == edges.size() * 2);

    // No column index escapes the matrix. REQUIRE again: the symmetry check
    // below indexes with these values.
    for (int k : m.col_idx) {
        INFO("column index ", k);
        REQUIRE(k >= 0);
        REQUIRE(k < n);
    }

    // Symmetry. j must appear in row i exactly as often as i appears in row
    // j -- "as often as", not "at least once", so that a duplicated edge is
    // still required to be duplicated on both sides.
    std::vector<std::vector<int>> count(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i)
        for (int k = m.row_ptr[i]; k < m.row_ptr[i + 1]; ++k)
            ++count[i][m.col_idx[k]];

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < i; ++j) {
            INFO("entry (", i, ", ", j, ")");
            CHECK(count[i][j] == count[j][i]);
        }
}

// ---------------------------------------------------------------------------
// A reference implementation to disagree with
// ---------------------------------------------------------------------------

// A dense n x n matrix of edge multiplicities, filled straight from the edge
// list. Hopelessly wasteful and obviously correct -- which is the whole point
// of an oracle. It has nowhere to hide a bug, so any disagreement with
// csr::build is a bug in build().
//
// Note what this encodes: a repeated edge becomes a count of 2, i.e. parallel
// edges are kept. That matches build() today. The moment you decide
// duplicates should be collapsed, this function is what has to change first.
inline std::vector<std::vector<int>> dense_adjacency(int n, const Edges& edges) {
    std::vector<std::vector<int>> a(n, std::vector<int>(n, 0));
    for (auto [i, j] : edges) {
        ++a[i][j];
        ++a[j][i];
    }
    return a;
}

// Row i of the dense matrix expanded into the same shape neighbors_of
// returns: each column repeated as many times as its multiplicity. Sorted by
// construction, since j only increases.
inline std::vector<int> dense_row(const std::vector<std::vector<int>>& a, int i) {
    std::vector<int> row;
    for (std::size_t j = 0; j < a.size(); ++j)
        row.insert(row.end(), static_cast<std::size_t>(a[i][j]), static_cast<int>(j));
    return row;
}

// ---------------------------------------------------------------------------
// Random input
// ---------------------------------------------------------------------------

// count random edges over n nodes.
//
// The seed is an argument and the engine is std::mt19937, so a failure is
// exactly reproducible: note the seed from the output and you can replay it
// forever. Seeding from the clock instead would hand you bug reports you
// cannot re-run, which is the classic way randomised tests become useless.
//
// Self-loops are rejected because the contract for them is still undecided
// (csr.hpp says "assumes no self-loops"); duplicates are kept, because
// dense_adjacency above models them and build() produces them.
inline Edges random_edges(int n, int count, std::uint32_t seed) {
    Edges edges;
    if (n < 2) return edges;   // no non-loop edge exists; avoids an infinite loop

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> node(0, n - 1);

    edges.reserve(static_cast<std::size_t>(count));
    while (static_cast<int>(edges.size()) < count) {
        const int i = node(rng);
        const int j = node(rng);
        if (i == j) continue;
        edges.emplace_back(i, j);
    }
    return edges;
}

}   // namespace csr_test

// ---------------------------------------------------------------------------
// Failure messages
// ---------------------------------------------------------------------------

// Out of the box doctest prints an unknown type as "{?}", which turns a
// failed vector comparison into a message that tells you nothing. Teaching it
// to print a vector<int> costs a few lines and makes every future failure
// readable. (Specialising StringMaker is doctest's hook for this; defining
// operator<< for std::vector in namespace std would be ill-formed.)
namespace doctest {
template <>
struct StringMaker<std::vector<int>> {
    static String convert(const std::vector<int>& v) {
        std::ostringstream os;
        os << '{';
        for (std::size_t k = 0; k < v.size(); ++k)
            os << (k ? ", " : "") << v[k];
        os << '}';
        return os.str().c_str();
    }
};
}   // namespace doctest
