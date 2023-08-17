#include <stdint.h>
uint64_t pointers[14] = {0xabcdef1234, 0xabcdef1334, 0xabcdef1434, 0xabcdef1534, 0xabcdef1634, 0xabcdef1734, 0xabcdef1834, 0xabcdef1934, 0xabcdef1a34, 0xabcdef1b34, 0xabcdef1c34, 0xabcdef1d34, 0xabcdef1e34, 0xabcdef1f34};
int main() {
    int i = 0;
    while (1) {
        uint64_t* p;
        p = (uint64_t*)pointers[i];
        *p = 1;
        i = (i + 1) % 14;
    }
}