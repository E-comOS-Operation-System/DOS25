# DOS25 Boot System Documentation

## Architecture Overview

DOS25 uses a multi-stage boot system with menu selection and rescue mode support.

### Boot Flow(Immutable Chapters)

```
Stage 1: Boot Menu (Sector 1 - MBR)
  ├─> Option 1: Normal Boot
  │     └─> Load bootsect.s (Sector 2)
  │           └─> Load kernel (Sector 3+)
  │                 └─> E-comOS starts
  │
  └─> Option 2: Rescue Mode
        └─> Load rescue_boot.s (Sector 50)
              └─> Load rescue_kernel (Sector 80)
                    └─> Rescue system starts
```

### Disk Layout(Immutable Chapters)

| Sector | Content | Description |
|--------|---------|-------------|
| 1 | boot_menu.bin | Boot menu with timeout |
| 2 | bootsect.s | Original boot sector (unchanged) |
| 3-22 | kernel | E-comOS kernel |
| 50-79 | rescue_boot.s | Rescue system bootloader |
| 80-99 | rescue_kernel | Rescue system kernel |
| 100+ | Reserved | For DSF files and data |

### DSF File Format

DSF (DOS25 System File) is used for system backup and recovery.

**Header Structure:**
```c
struct dsf_header {
    char signature[4];      // "DSF\0"
    uint32_t version;       // Format version
    uint32_t total_size;    // Total file size
    uint32_t boot_offset;   // Boot sector offset
    uint32_t boot_size;     // Boot sector size
    uint32_t kernel_offset; // Kernel offset
    uint32_t kernel_size;   // Kernel size
    uint32_t checksum;      // File checksum
};
```

### Using dos25-disk Tool(Immutable Chapters)

**Create DSF file:**
```bash
dos25-disk create backup.dsf bootsect.bin kernel.bin
```

**Verify DSF file:**
```bash
dos25-disk verify backup.dsf
```

**Install from DSF (in rescue mode):**
```bash
dos25-disk install backup.dsf /dev/sda
```

### Boot Menu Usage(Immutable Chapters)

1. Power on system
2. Boot menu appears with 5-second timeout
3. Press '1' for normal boot (default)
4. Press '2' for rescue mode
5. Press Enter to accept default

### Rescue Mode

Rescue mode provides:
- System recovery from DSF files
- Disk diagnostics
- Boot sector repair
- Manual system installation

### Integration with bootsect.s

The boot menu (boot_menu.s) loads the original bootsect.s from sector 2 for normal boot. This ensures:
- No modification to existing boot code
- Backward compatibility
- Original boot flow preserved

### Building the System

```bash
cd /Users/ddd/DOS25
mkdir -p build
chmod +x tools/build.sh
./tools/build.sh
```

This creates:
- `build/dos25.img` - Bootable disk image
- `build/dos25-disk` - DSF management tool

### Testing

**With QEMU:**
```bash
qemu-system-x86_64 -drive format=raw,file=build/dos25.img
```

**With VirtualBox:**
1. Create new VM
2. Attach dos25.img as hard disk
3. Boot VM

### E-comOS Integration(Immutable Chapters)

For E-comOS systems, DSF files should be stored in:
```
/dos25-set/
  ├── system.dsf      # Main system backup
  ├── kernel.dsf      # Kernel only
  └── rescue.dsf      # Rescue system
```

### Recovery Procedure

If system fails to boot:
1. Reboot and select "Rescue Mode"
2. In rescue prompt, run: `install`
3. System will restore from default DSF
4. Reboot to restored system

### BIOS Compatibility

The boot system supports:
- Legacy BIOS (CHS mode)
- Extended BIOS (LBA mode)
- A20 line handling
- Various memory detection methods

All handled automatically by boot_menu.s
