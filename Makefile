# DOS25 Build System
# Generate release binaries and manage builds
# Usage:
#   make            - Default: build all components
#   make myself-release - Generate final bootable .bin release
#   make clean      - Remove build artifacts

# --------------------------
# Configuration (Modify these as needed)
# --------------------------
# Source directory (matches your src structure)
SRC_DIR := src
# Build output directory (for intermediate binaries)
BUILD_DIR := build
# Final release binary name
RELEASE_BIN := $(BUILD_DIR)/dos25-release.bin
# List of core components (order matches disk layout in docs)
COMPONENTS := \
    $(BUILD_DIR)/boot_menu.bin \
    $(BUILD_DIR)/bootsect.bin \
    $(BUILD_DIR)/kernel.bin \
    $(BUILD_DIR)/rescue_boot.bin \
    $(BUILD_DIR)/rescue_kernel.bin

# --------------------------
# Tools (Assembler, etc.)
# --------------------------
NASM := nasm
NASM_FLAGS := -f bin -Wall  # Binary format, show warnings
BUILD_SCRIPT := tools/build.sh  # Path to your existing build script

# --------------------------
# Default target: build all components
# --------------------------
all: $(BUILD_DIR) $(COMPONENTS)
	@echo "✅ All components built successfully in $(BUILD_DIR)"

# --------------------------
# Create build directory if missing
# --------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --------------------------
# Compile individual components (assembler sources)
# --------------------------
$(BUILD_DIR)/boot_menu.bin: $(SRC_DIR)/boot_menu.s
	@echo "🔧 Assembling $<..."
	$(NASM) $(NASM_FLAGS) $< -o $@

$(BUILD_DIR)/bootsect.bin: $(SRC_DIR)/bootsect.s
	@echo "🔧 Assembling $<..."
	$(NASM) $(NASM_FLAGS) $< -o $@

$(BUILD_DIR)/kernel.bin: $(SRC_DIR)/kernel.s  # Adjust if kernel has multiple sources
	@echo "🔧 Assembling $<..."
	$(NASM) $(NASM_FLAGS) $< -o $@

$(BUILD_DIR)/rescue_boot.bin: $(SRC_DIR)/rescue_boot.s
	@echo "🔧 Assembling $<..."
	$(NASM) $(NASM_FLAGS) $< -o $@

$(BUILD_DIR)/rescue_kernel.bin: $(SRC_DIR)/rescue_kernel.s
	@echo "🔧 Assembling $<..."
	$(NASM) $(NASM_FLAGS) $< -o $@

# --------------------------
# Generate final release: concatenate components in disk order
# --------------------------
myself-release: all
	@echo "📦 Generating release binary..."
	# Combine components (matches disk layout: Sector 1 → 2 → 3+ → 50+ → 80+)
	cat $(COMPONENTS) > $(RELEASE_BIN)
	# Verify release size (optional: ensure it fits your disk constraints)
	@echo "📏 Release size: $$(du -h $(RELEASE_BIN) | cut -f1)"
	@echo "✅ Release ready: $(RELEASE_BIN)"

# --------------------------
# Clean build artifacts
# --------------------------
clean:
	@echo "🧹 Cleaning build files..."
	rm -rf $(BUILD_DIR)/*.bin
	@echo "✅ Clean complete"

# --------------------------
# Phony targets (avoid filename conflicts)
# --------------------------
.PHONY: all myself-release clean