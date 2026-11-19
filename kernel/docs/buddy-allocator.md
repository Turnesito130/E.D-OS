# Buddy allocator for E.D-OS

Status: implemented as a freestanding, build-integrated component; **not yet connected to the Limine physical-memory map or used for live kernel allocations**.

## Why this component exists

E.D-OS keeps its own x86-64 kernel and does not boot or depend on the Linux kernel. This allocator implements the binary Buddy strategy used by mature kernels, following the algorithmic design documented by Linux. It is an E.D-OS implementation, not copied Linux source.

Linux reference material:
- `mm/page_alloc.c` and `include/linux/mmzone.h`
- [Linux physical memory documentation](https://docs.kernel.org/mm/physical_memory.html)
- [Linux memory allocation guide](https://docs.kernel.org/core-api/memory-allocation.html)

## Interface and constraints

- Metadata is supplied by the caller; the allocator never allocates its own metadata.
- The managed arena is a contiguous array of page frames. Discontiguous physical ranges must be represented as separate arenas or mapped into a carefully validated frame-index space.
- Order N represents 2^N pages.
- Free blocks are split on allocation and coalesced with their XOR buddy on release.
- Exact allocation order and block-head state are checked to reject interior pointers, wrong-order frees and straightforward double frees.
- Synchronization is deliberately not included. Do not use concurrently or from interrupt contexts until E.D-OS adds and tests the appropriate locks/interrupt discipline.
- Callers must only add RAM ranges that have been validated as usable and not reserved for the kernel, bootloader, modules, firmware tables or MMIO.

## Current limitations

This first implementation is a foundation, not Linux's complete page allocator. It does not yet implement NUMA, zones, per-CPU page lists, watermarks, reclaim, compaction, page migration, hotplug, or GFP-style policies. It must pass dedicated unit tests and be integrated with the physical-frame ownership metadata before replacing the current PMM.
