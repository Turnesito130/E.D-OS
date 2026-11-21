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
