#include "pci.h"

#define PCI_CONFIG_ADDRESS 0x0cf8U
#define PCI_CONFIG_DATA 0x0cfcU
#define PCI_ABSENT 0xffffffffU

static int read_device(edos_pci_config_reader_t reader, void *reader_context,
                       edos_pci_u8 bus, edos_pci_u8 slot,
                       edos_pci_u8 function,
                       edos_pci_device_callback_t callback,
                       void *callback_context) {
    edos_pci_u32 identity = reader(bus, slot, function, 0x00U, reader_context);
    edos_pci_u16 vendor = (edos_pci_u16)(identity & 0xffffU);
    if (identity == PCI_ABSENT || vendor == 0xffffU) return 0;

    edos_pci_u32 class_info = reader(bus, slot, function, 0x08U, reader_context);
    edos_pci_u32 header_info = reader(bus, slot, function, 0x0cU, reader_context);
    edos_pci_device_t device;
    device.vendor_id = vendor;
    device.device_id = (edos_pci_u16)(identity >> 16);
    device.class_code = (edos_pci_u8)(class_info >> 24);
    device.subclass = (edos_pci_u8)(class_info >> 16);
    device.programming_interface = (edos_pci_u8)(class_info >> 8);
    device.revision = (edos_pci_u8)class_info;
    device.header_type = (edos_pci_u8)(header_info >> 16);
    device.bus = bus;
    device.slot = slot;
    device.function = function;
    if (callback) callback(&device, callback_context);
    return 1;
}

edos_pci_u32 edos_pci_enumerate_with_reader(
    edos_pci_config_reader_t reader, void *reader_context,
    edos_pci_device_callback_t callback, void *callback_context) {
    if (!reader) return 0;
    edos_pci_u32 found = 0;
    for (edos_pci_u32 bus = 0; bus < 256U; ++bus) {
        for (edos_pci_u32 slot = 0; slot < 32U; ++slot) {
            if (!read_device(reader, reader_context, (edos_pci_u8)bus,
                             (edos_pci_u8)slot, 0U, callback, callback_context))
                continue;
            ++found;
            edos_pci_u32 header = reader((edos_pci_u8)bus, (edos_pci_u8)slot,
                                         0U, 0x0cU, reader_context);
            if (!((header >> 16) & 0x80U)) continue;
            for (edos_pci_u32 function = 1; function < 8U; ++function) {
                if (read_device(reader, reader_context, (edos_pci_u8)bus,
                                (edos_pci_u8)slot, (edos_pci_u8)function,
                                callback, callback_context))
                    ++found;
            }
        }
    }
    return found;
}

static void pci_outl(edos_pci_u16 port, edos_pci_u32 value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static edos_pci_u32 pci_inl(edos_pci_u16 port) {
    edos_pci_u32 value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static edos_pci_u32 hardware_reader(edos_pci_u8 bus, edos_pci_u8 slot,
                                    edos_pci_u8 function, edos_pci_u8 offset,
                                    void *context) {
    (void)context;
    edos_pci_u32 address = 0x80000000U |
        ((edos_pci_u32)bus << 16) |
        ((edos_pci_u32)slot << 11) |
        ((edos_pci_u32)function << 8) |
        ((edos_pci_u32)offset & 0xfcU);
    pci_outl(PCI_CONFIG_ADDRESS, address);
    return pci_inl(PCI_CONFIG_DATA);
}

edos_pci_u32 edos_pci_enumerate(edos_pci_device_callback_t callback,
                                void *callback_context) {
    return edos_pci_enumerate_with_reader(hardware_reader, (void *)0,
                                          callback, callback_context);
}

int edos_pci_read_bar_with_reader(const edos_pci_device_t *device,
                                  edos_pci_u8 index,
                                  edos_pci_config_reader_t reader,
                                  void *reader_context, edos_pci_bar_t *bar) {
    if (!device || !reader || !bar) return 0;
    edos_pci_u8 header = (edos_pci_u8)(device->header_type & 0x7fU);
    edos_pci_u8 bar_count = header == 0U ? 6U : header == 1U ? 2U : 0U;
    if (!bar_count || index >= bar_count) return 0;
    for (edos_pci_u8 previous = 0; previous < index; ++previous) {
        edos_pci_u32 prior = reader(device->bus, device->slot, device->function,
                                    (edos_pci_u8)(0x10U + previous * 4U), reader_context);
        if (prior != 0U && prior != PCI_ABSENT && !(prior & 1U) &&
            (((prior >> 1) & 3U) == 2U)) {
            if ((edos_pci_u8)(previous + 1U) == index) return 0;
            ++previous;
        }
    }
    edos_pci_u8 offset = (edos_pci_u8)(0x10U + (edos_pci_u8)(index * 4U));
    edos_pci_u32 low = reader(device->bus, device->slot, device->function,
                              offset, reader_context);
    if (low == 0U || low == PCI_ABSENT) return 0;
    bar->index = index;
    bar->is_io = 0;
    bar->is_64bit = 0;
    bar->prefetchable = 0;
    if (low & 1U) {
        bar->base = (unsigned long long)(low & ~3U);
        bar->is_io = 1;
        return 1;
    }
    edos_pci_u32 memory_type = (low >> 1) & 3U;
    if (memory_type == 3U) return 0;
    bar->prefetchable = (edos_pci_u8)((low >> 3) & 1U);
    bar->base = (unsigned long long)(low & ~0x0fU);
    if (memory_type == 2U) {
        if (index + 1U >= bar_count) return 0;
        edos_pci_u32 high = reader(device->bus, device->slot, device->function,
                                   (edos_pci_u8)(offset + 4U), reader_context);
        if (high == PCI_ABSENT) return 0;
        bar->base |= ((unsigned long long)high << 32);
        bar->is_64bit = 1;
    }
    return 1;
}

int edos_pci_read_bar(const edos_pci_device_t *device, edos_pci_u8 index,
                      edos_pci_bar_t *bar) {
    return edos_pci_read_bar_with_reader(device, index, hardware_reader,
                                         (void *)0, bar);
}
