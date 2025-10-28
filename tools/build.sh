#!/bin/bash
# DOS25 Build Script

set -e

echo "Building DOS25..."

# Build boot menu
echo "Building boot menu..."
nasm -f bin src/bootloader/boot_menu.s -o build/boot_menu.bin

# Build original bootsect (keep existing)
echo "Building bootsect..."
nasm -f bin src/boot/bootsect.s -o build/bootsect.bin

# Build rescue system
echo "Building rescue system..."
nasm -f bin src/rescue_system/rescue_boot.s -o build/rescue_boot.bin
nasm -f bin src/rescue_system/rescue_kernel.s -o build/rescue_kernel.bin

# Build dos25-disk tool
echo "Building dos25-disk..."
gcc src/dsf_tools/dos25-disk.c -o build/dos25-disk

# Create disk image
echo "Creating disk image..."
dd if=/dev/zero of=build/dos25.img bs=512 count=2048

# Write boot menu to sector 1 (MBR)
dd if=build/boot_menu.bin of=build/dos25.img bs=512 count=1 conv=notrunc

# Write original bootsect to sector 2
dd if=build/bootsect.bin of=build/dos25.img bs=512 seek=1 count=1 conv=notrunc

# Write rescue boot to sector 50
dd if=build/rescue_boot.bin of=build/dos25.img bs=512 seek=49 count=1 conv=notrunc

# Write rescue kernel to sector 80
dd if=build/rescue_kernel.bin of=build/dos25.img bs=512 seek=79 conv=notrunc

echo "Build complete!"
echo "Disk image: build/dos25.img"
echo "Tool: build/dos25-disk"
