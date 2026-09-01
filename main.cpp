#include "csr.hpp"
#include <iostream>

int main() {
    Csr m = build(4, {{0, 1}, {1, 2}, {2, 3}});   // path 0-1-2-3
    for (int r = 0; r < 4; ++r) {
        std::cout << r << ":";
        for (int k = m.row_ptr[r]; k < m.row_ptr[r + 1]; ++k)
            std::cout << " " << m.col_idx[k];
        std::cout << "\n";
    }
}