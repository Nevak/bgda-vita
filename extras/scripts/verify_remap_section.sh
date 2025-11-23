#!/bin/bash
# verify_remap_section.sh
# Verifies that the .sce_remap_source_files section has the correct format

if [ $# -eq 0 ]; then
    echo "Usage: $0 <path_to_elf_file>"
    exit 1
fi

ELF_FILE="$1"

echo "Checking $ELF_FILE for .sce_remap_source_files section..."
echo ""

# Check if section exists
if ! arm-vita-eabi-readelf -S "$ELF_FILE" | grep -q "sce_remap_source_files"; then
    echo "❌ ERROR: .sce_remap_source_files section not found!"
    exit 1
fi

echo "✓ Section exists"

# Get section details
echo ""
echo "Section details:"
arm-vita-eabi-readelf -S "$ELF_FILE" | grep -A 1 "sce_remap_source_files"

# Extract section data
echo ""
echo "Section contents (hex dump):"
arm-vita-eabi-objcopy --dump-section .sce_remap_source_files=/tmp/remap_section.bin "$ELF_FILE" 2>/dev/null

if [ -f /tmp/remap_section.bin ]; then
    SIZE=$(stat -f%z /tmp/remap_section.bin 2>/dev/null || stat -c%s /tmp/remap_section.bin 2>/dev/null)
    echo "Section size: $SIZE bytes"
    echo ""
    
    if [ $SIZE -eq 1 ]; then
        echo "⚠️  WARNING: Section is only 1 byte (empty section)"
        echo "   This might not be enough for the tool."
        echo "   Try using remap_section_with_data.c instead."
    elif [ $SIZE -gt 1 ]; then
        echo "✓ Section has data ($SIZE bytes)"
        echo ""
        echo "First 200 bytes (strings):"
        head -c 200 /tmp/remap_section.bin | strings | head -10
        echo ""
        echo "String pairs found:"
        strings /tmp/remap_section.bin | wc -l
    fi
    
    rm /tmp/remap_section.bin
else
    echo "⚠️  Could not extract section data"
fi

echo ""
echo "Verification complete!"