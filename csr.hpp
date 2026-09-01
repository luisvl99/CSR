#pragma once
#include <vector>
#include <utility>

struct Csr {
    std::vector<int> row_ptr;   // size n+1
    std::vector<int> col_idx;   // size nnz
};

// Each undirected edge is given once; we store both directions.
// Assumes no self-loops for now.
inline Csr build(int n, const std::vector<std::pair<int, int>>& edges) {
    Csr m;
    m.row_ptr.assign(n + 1, 0);

    // Pass 1: count each node's degree, into slot i+1.
    for (auto [i, j] : edges) {
        ++m.row_ptr[i + 1];
        ++m.row_ptr[j + 1];
    }

    // Prefix sum. Because of the +1 above, this lands directly on row_ptr.
    for (int r = 1; r <= n; ++r)
        m.row_ptr[r] += m.row_ptr[r - 1];

    // Pass 2: write each neighbour into the next free slot of its row.
    m.col_idx.resize(m.row_ptr[n]);
    std::vector<int> cursor = m.row_ptr;
    for (auto [i, j] : edges) {
        m.col_idx[cursor[i]++] = j;
        m.col_idx[cursor[j]++] = i;
    }
    return m;
}