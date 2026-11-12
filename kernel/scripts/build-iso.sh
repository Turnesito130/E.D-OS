#!/usr/bin/env bash
set -euo pipefail

KERNEL_ELF="${1:-build/ed-os-kernel.elf}"
OUTPUT_ISO="${2:-build/ed-os.iso}"
: "${LIMINE_DIR:?Set LIMINE_DIR to the extracted Limine binary release directory}"

for required in limine limine-bios.sys limine-bios-cd.bin limine-uefi-cd.bin BOOTX64.EFI; do
    if [[ ! -f "$LIMINE_DIR/$required" ]]; then
        echo "Missing Limine release file: $LIMINE_DIR/$required" >&2
        exit 1
    fi
done

if [[ ! -f "$KERNEL_ELF" ]]; then
    echo "Kernel ELF not found: $KERNEL_ELF" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$(dirname "$OUTPUT_ISO")"
ISO_ROOT="$BUILD_DIR/iso-root"
mkdir -p "$BUILD_DIR" "$ISO_ROOT/boot/limine" "$ISO_ROOT/EFI/BOOT"

cp "$KERNEL_ELF" "$ISO_ROOT/boot/ed-os-kernel.elf"
cp "$ROOT/kernel/limine.conf" "$ISO_ROOT/boot/limine/limine.conf"
cp "$LIMINE_DIR/limine-bios.sys" "$ISO_ROOT/boot/limine/"
cp "$LIMINE_DIR/limine-bios-cd.bin" "$ISO_ROOT/boot/"
cp "$LIMINE_DIR/limine-uefi-cd.bin" "$ISO_ROOT/boot/"
cp "$LIMINE_DIR/BOOTX64.EFI" "$ISO_ROOT/EFI/BOOT/"

xorriso -as mkisofs -R -r -J -b boot/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 \
    --efi-boot boot/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
    --protective-msdos-label "$ISO_ROOT" -o "$OUTPUT_ISO"

"$LIMINE_DIR/limine" bios-install "$OUTPUT_ISO"
echo "Created hybrid BIOS/UEFI image: $OUTPUT_ISO"
