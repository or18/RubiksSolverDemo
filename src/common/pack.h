#pragma once

inline unsigned char get_prune(const unsigned char* table, int index) {
    return (table[index >> 2] >> ((index & 3) << 1)) & 0x03;
}

inline void set_prune(unsigned char* table, int index, unsigned char val) {
    int byte_idx = index >> 2;
    int shift = (index & 3) << 1;
    table[byte_idx] = (table[byte_idx] & ~(0x03 << shift)) | (val << shift);
}