#ifndef MACROS_H
#define MACROS_H
extern so_module so_mod;

#define LOC(x) (int *)(so_mod.text_base + x - 0x00010000)
#define CONCAT22(high16, low16) ( \
    ( ((uint32_t)(high16) & 0xFFFF) << 16 ) | \
      ((uint32_t)(low16)  & 0xFFFF)          \
)
#define CONCAT44(high32, low32) ( \
    ( ((uint64_t)(high32) & 0xFFFFFFFFULL) << 32 ) | \
      (  (uint64_t)(low32)  & 0xFFFFFFFFULL )        \
)

#endif