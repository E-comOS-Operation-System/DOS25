#   <one line to give the program's name and a brief idea of what it does.>
#   Copyright (C) <year>  <name of author>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

# DOS25 Makefile
# Targets:
#   - make              → Compile bootloader components (bootsect.bin, loader.bin)
#   - make myself-release → Generate installable release .bin file
#   - make run          → Test with QEMU (requires E-comOS kernel)
#   - make clean        → Clean build artifacts


# -------------------------- Configuration --------------------------
# Assembler and flags (output raw binary for bootloader)
AS      := nasm
ASFLAGS := -f bin
# Output directory for intermediate files
BUILD_DIR := build
# Release file name (matches README's .bin output)
RELEASE_BIN := dos25-release.bin
# Source files (boot sector and optional loader)
BOOTSECT_SRC := src/bootsect.s
LOADER_SRC   := src/loader.s
# Compiled intermediate files
BOOTSECT_BIN := $(BUILD_DIR)/bootsect.bin
LOADER_BIN   := $(BUILD_DIR)/loader.bin


# -------------------------- Default Target --------------------------
# Compile all components when running 'make'
all: $(BOOTSECT_BIN) $(LOADER_BIN)
	@echo "✅ DOS25 components compiled: $(BOOTSECT_BIN), $(LOADER_BIN)"


# -------------------------- Build Components --------------------------
# Compile boot sector (1st stage, must be 512 bytes)
$(BOOTSECT_BIN): $(BOOTSECT_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "🔧 Assembling boot sector: $<"
	$(AS) $(ASFLAGS) $< -o $@
	@# Critical check: ensure boot sector is exactly 512 bytes with 0x55AA signature
	@if [ $(shell wc -c < $@) -ne 512 ]; then \
		echo "❌ Error: Boot sector must be 512 bytes"; \
		rm $@; \
		exit 1; \
	fi
	@# Check for 0x55AA at the end (last 2 bytes)
	@if ! tail -c 2 $@ | cmp -s - <(echo -en "\x55\xAA"); then \
		echo "❌ Error: Boot sector missing 0x55AA signature"; \
		rm $@; \
		exit 1; \
	fi

# Compile loader (2nd stage, optional but recommended for larger kernels)
$(LOADER_BIN): $(LOADER_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "🔧 Assembling loader: $<"
	$(AS) $(ASFLAGS) $< -o $@


# -------------------------- Generate Release --------------------------
# Create installable .bin file (combines boot sector + loader)
myself-release: $(BOOTSECT_BIN) $(LOADER_BIN)
	@echo "📦 Generating release file: $(RELEASE_BIN)"
	# Combine boot sector and loader into a single installable binary
	cat $(BOOTSECT_BIN) $(LOADER_BIN) > $(RELEASE_BIN)
	@echo "✅ Release ready: $(RELEASE_BIN) (install to your OS with this file)"


# -------------------------- Test with QEMU --------------------------
# Test bootloader with a dummy kernel (replace KERNEL_PATH with actual E-comOS kernel)
run: $(RELEASE_BIN)
	@echo "🚀 Testing with QEMU (ensure E-comOS kernel is available)..."
	# Create a test disk image: release.bin + dummy kernel (replace with real kernel path)
	dd if=/dev/zero of=dos25-test.img bs=1M count=10 status=none
	dd if=$(RELEASE_BIN) of=dos25-test.img conv=notrunc status=none
	# Optional: Add E-comOS kernel to test loading (uncomment and set KERNEL_PATH)
	# KERNEL_PATH := ../e-comos-kernel/build/e-comos-kernel
	# dd if=$(KERNEL_PATH) of=dos25-test.img seek=10 conv=notrunc status=none
	qemu-system-x86_64 -drive format=raw,file=dos25-test.img -m 4G


# -------------------------- Clean Artifacts --------------------------
clean:
	@echo "🧹 Cleaning build files..."
	rm -rf $(BUILD_DIR) $(RELEASE_BIN) dos25-test.img


# -------------------------- Phony Targets --------------------------
.PHONY: all myself-release run clean
