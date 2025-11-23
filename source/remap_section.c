// remap_section.c
// CRITICAL: Use char array WITHOUT implicit null terminator

__attribute__((section(".sce_remap_source_files")))
__attribute__((used))
const char sce_remap_data[] = {
    '\0',  // Prefix
    
    // Pair 1
    '/','t','e','s','t','/','p','a','t','h','1','.','c','\0',
    'C',':','/','t','e','s','t','/','p','a','t','h','1','.','c','\0',
    
    // Pair 2
    '/','t','e','s','t','/','p','a','t','h','2','.','c','\0',
    'C',':','/','t','e','s','t','/','p','a','t','h','2','.','c','\0'
    // NO comma after last element - no automatic null terminator
};