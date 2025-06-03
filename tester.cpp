#include "gc.hpp"
#include <iostream>
#include <vector>

int main() {
    GC_init();

    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        void* mem = GC_malloc(64);
        if (mem)
            std::cout << "Allocated: " << mem << std::endl;
        else
            std::cerr << "Allocation failed at index " << i << std::endl;
        ptrs.push_back(mem);
    }

    std::cout << "Running garbage collector..." << std::endl;
    GC_collect();
    std::cout << "GC done." << std::endl;

    return 0;
}
