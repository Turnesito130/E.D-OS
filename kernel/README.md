# E.D-OS kernel workspace

This directory is the integration point for the Linux kernel used as a starting
point for E.D-OS. The upstream kernel source is intentionally **not copied into
this repository yet**: first we pin an upstream revision, record its license
and provenance, and make builds reproducible.

## Initial target

- First bring-up target: x86_64 under QEMU with UEFI/OVMF.
- First physical target: a standard x86_64 UEFI PC, selected after hardware
  compatibility checks.
- ARM64/mobile support is a later target; it is not implied by the first build.

## Source and modification policy

1. Select a maintained upstream stable/LTS kernel revision and record the
   exact tag and commit before importing or building it.
2. Keep upstream source distinguishable from E.D-OS changes.
3. Store E.D-OS kernel changes as small, reviewable patches under
   `kernel/patches/`, with rationale and test evidence.
4. Keep configuration fragments under `kernel/config/`; do not silently
   replace upstream defaults.
5. Preserve upstream copyright notices, SPDX identifiers, and all applicable
   license texts. Linux kernel contributions are generally GPL-2.0-only;
   review the exact licensing of every imported component.
6. Do not claim a kernel boots until a reproducible build and QEMU boot test
   have both succeeded.

## Planned layout

- `kernel/config/`: E.D-OS configuration fragments per target.
- `kernel/patches/`: E.D-OS patches against the pinned upstream revision.
- `kernel/manifest.json`: exact upstream version, commit, source URL and
  checksums once selected.
- `kernel/build/`: generated build output only; never commit build products.

## First milestones

- [ ] Choose and pin a maintained kernel revision.
- [ ] Define a reproducible x86_64 toolchain and build environment.
- [ ] Produce an unmodified reference kernel build.
- [ ] Boot the reference kernel in QEMU.
- [ ] Add one narrowly scoped E.D-OS patch only after the reference build works.
- [ ] Rebuild and boot-test after each patch.

## Important boundary

The current E.D-OS web desktop and ecosystem API are not a bootable operating
system. This directory documents and prepares the native-kernel work; it does
not yet contain a kernel image, bootloader, root filesystem, or tested patch.
