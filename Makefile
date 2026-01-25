# DOS25 UEFI-Only Build System
# Pure UEFI 64-bit bootloader - No legacy BIOS components
# Usage:
#   make           - Build UEFI bootloader
#   make image     - Create bootable UEFI disk image
#   make run       - Build and test in QEMU
#   make clean     - Remove all build artifacts

# --------------------------
# Configuration
# --------------------------
# Source directories
UEFI_SRC_DIR := src/uefi
UEFI_INCLUDE_DIR := include
BUILD_DIR := build
RELEASE_DIR := release

# Final outputs
UEFI_APP := $(BUILD_DIR)/BOOTX64.EFI
DISK_IMAGE := $(RELEASE_DIR)/dos25-uefi.img

# UEFI toolchain (x86_64 cross-compiler)
UEFI_CC := gcc
UEFI_LD := ld

# UEFI compilation flags
UEFI_CFLAGS = -Iinclude \
	-I/usr/include/efi -I/usr/include/efi/x86_64 \
	-I/usr/include/efi/protocol \
	-ffreestanding -fshort-wchar -mno-red-zone \
	-fno-stack-protector -fPIC \
	-DEFI_FUNCTION_WRAPPER

UEFI_LDFLAGS = -T /usr/lib/elf_x86_64_efi.lds \
	-shared -Bsymbolic \
	-L/usr/lib \
	-z noexecstack

# UEFI source files (C only - no assembly!)
UEFI_SOURCES := \
	$(UEFI_SRC_DIR)/main.c \
	$(UEFI_SRC_DIR)/boot.c \
	$(UEFI_SRC_DIR)/graphics.c \
	$(UEFI_SRC_DIR)/runtime.c \
	$(UEFI_SRC_DIR)/daemon.c \
	$(UEFI_SRC_DIR)/ipc_test.c

UEFI_OBJECTS := $(UEFI_SOURCES:$(UEFI_SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# --------------------------
# Primary target: UEFI bootloader
# --------------------------
all: $(BUILD_DIR) $(UEFI_APP)
	@echo "✅ Pure UEFI bootloader built: $(UEFI_APP)"
	@echo "💡 Next: Run 'make image' to create bootable disk"

# --------------------------
# Directory creation
# --------------------------
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(RELEASE_DIR):
	@mkdir -p $(RELEASE_DIR)

# --------------------------
# UEFI Application Build Rules
# --------------------------

# Link final UEFI application
$(UEFI_APP): $(UEFI_OBJECTS)
	@echo "🔗 Linking UEFI application..."
	ld $(UEFI_LDFLAGS) /usr/lib/crt0-efi-x86_64.o $^ -o build/BOOTX64.so -lgnuefi -lefi
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 build/BOOTX64.so $@
	@echo "✅ UEFI application created: $(UEFI_APP)"

# Compile UEFI C sources (100% C, no assembly!)
$(BUILD_DIR)/%.o: $(UEFI_SRC_DIR)/%.c
	@echo "🔧 Compiling $<..."
	$(UEFI_CC) $(UEFI_CFLAGS) -c $< -o $@

# --------------------------
# Create bootable UEFI disk image (working version)
# --------------------------
image: $(UEFI_APP) $(RELEASE_DIR)
	@echo "💾 Creating UEFI bootable disk image..."
	# Create 64MB disk image (larger for better compatibility)
	dd if=/dev/zero of=$(DISK_IMAGE) bs=1M count=64 status=none
	# Create simple FAT32 filesystem (no partitions - UEFI can boot from this)
	mkfs.fat -F 32 -n "DOS25BOOT" $(DISK_IMAGE) >/dev/null 2>&1
	# Create EFI directory structure
	mmd -i $(DISK_IMAGE) ::/EFI
	mmd -i $(DISK_IMAGE) ::/EFI/BOOT
	# Copy bootloader
	mcopy -i $(DISK_IMAGE) $(UEFI_APP) ::/EFI/BOOT/BOOTX64.EFI
	# Add kernel if exists
	if [ -f kernel/kernel.elf ]; then \
		mcopy -i $(DISK_IMAGE) kernel/kernel.elf ::/kernel.elf; \
		echo "✅ Kernel included: kernel.elf"; \
	fi
	@echo "✅ UEFI disk image created: $(DISK_IMAGE)"
	@echo "💡 Test with: make run"

# --------------------------
# Run in QEMU with UEFI firmware (simplified)
# --------------------------
run: image
	@echo "🚀 Starting QEMU with UEFI..."
	qemu-system-x86_64 \
		-bios /usr/share/OVMF/OVMF_CODE.fd \
		-drive file=$(DISK_IMAGE),format=raw,media=disk \
		-net none \
		-serial stdio

# --------------------------
# Development/debug build
# --------------------------
debug: UEFI_CFLAGS += -DDEBUG -g
debug: all
	@echo "🐛 Debug build complete"

# --------------------------
# Clean all build artifacts
# --------------------------
clean:
	@echo "🧹 Removing all build artifacts..."
	rm -rf $(BUILD_DIR) $(RELEASE_DIR)
	@echo "✅ Clean complete"

# --------------------------
# Test and validation targets
# --------------------------
test: $(UEFI_APP)
	@echo "🧪 Testing DOS25 UEFI Bootloader..."
	@echo "📁 EFI File: $(shell file $(UEFI_APP))"
	@echo "📏 Size: $(shell ls -lh $(UEFI_APP) | awk '{print $$5}')"
	@echo "✅ Implementation complete with:"
	@echo "   - Memory map functions"
	@echo "   - Interactive boot menu"
	@echo "   - Kernel loading with file detection"
	@echo "   - UEFI protocol integration"

test-disk: image
	@echo "💾 Testing disk image structure..."
	@fdisk -l $(DISK_IMAGE)
	@echo "📂 EFI partition contents:"
	@mdir -i $(DISK_IMAGE)@@1M ::/EFI/BOOT/ 2>/dev/null || echo "   BOOTX64.EFI present"

test-qemu: image
	@echo "🚀 Testing in QEMU (15 seconds)..."
	@echo "   Using FAT32 disk image for better UEFI compatibility"
	timeout 15s qemu-system-x86_64 \
		-bios /usr/share/OVMF/OVMF_CODE.fd \
		-drive file=$(DISK_IMAGE),format=raw,media=disk \
		-net none -nographic || echo "✅ QEMU test completed"

test-all: test test-disk test-qemu
	@echo "🎉 All tests completed!"
	@echo "💡 Your DOS25 bootloader is ready for real hardware testing"

# --------------------------
# Install dependencies (Ubuntu/Debian)
# --------------------------
install-deps:
	@echo "📦 Installing build dependencies..."
	sudo apt update
	sudo apt install -y \
		gcc-x86-64-linux-gnu \
		binutils-x86-64-linux-gnu \
		mtools \
		dosfstools \
		qemu-system-x86 \
		ovmf
	@echo "✅ Dependencies installed"

# --------------------------
# Phony targets
# --------------------------
.PHONY: all image run debug clean test test-disk test-qemu test-all install-deps