/*
 * NEON-optimized memory copy for WriteCommand_Optimized
 * Uses ARM NEON SIMD instructions for faster bulk data transfer
 */

#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

// Fast NEON-based memcpy for aligned data
static inline void neon_memcpy_aligned(void* __restrict dst, const void* __restrict src, size_t size) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    
    // Handle 64-byte chunks with NEON
    while (size >= 64) {
        uint8x16x4_t data = vld1q_u8_x4(s);
        vst1q_u8_x4(d, data);
        s += 64;
        d += 64;
        size -= 64;
    }
    
    // Handle 16-byte chunks
    while (size >= 16) {
        uint8x16_t data = vld1q_u8(s);
        vst1q_u8(d, data);
        s += 16;
        d += 16;
        size -= 16;
    }
    
    // Handle remaining bytes
    if (size > 0) {
        memcpy(d, s, size);
    }
}

// Optimized memcpy wrapper that chooses best method based on size and alignment
void writecommand_memcpy_optimized(void* dst, const void* src, size_t size) {
    // Use NEON for larger transfers and when both pointers are 16-byte aligned
    if (size >= 32 && 
        ((uintptr_t)dst & 15) == 0 && 
        ((uintptr_t)src & 15) == 0) {
        neon_memcpy_aligned(dst, src, size);
    } else {
        // Fall back to standard memcpy for small or unaligned data
        memcpy(dst, src, size);
    }
}