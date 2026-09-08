#include <csr/csr.hpp>
#include <iostream>

int main() {
    // path graph 0-1-2-3, each undirected edge given once
    csr::Csr m = csr::build(4, {{0, 1}, {1, 2}, {2, 3}});

    for (int i = 0; i < 4; ++i) {
        std::cout << i << ":";
        for (int k = m.row_ptr[i]; k < m.row_ptr[i + 1]; ++k)
            std::cout << " " << m.col_idx[k];
        std::cout << "\n";
    }
}
