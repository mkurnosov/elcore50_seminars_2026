#include <stdio.h>
#include <stdint.h>

void calculate_crc(uint8_t *buf, size_t size, uint32_t *ret_crc)
{
    uint32_t crc, mask;
    crc = 0xFFFFFFFF;
    for (int i = 0; i < size; ++i) {
        crc = crc ^ buf[i];
        for (int j = 7; j >= 0; --j) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    *ret_crc = ~crc;
}
