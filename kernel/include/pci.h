#ifndef EDOS_PCI_H
#define EDOS_PCI_H

typedef unsigned int edos_pci_u32;
typedef unsigned short edos_pci_u16;
typedef unsigned char edos_pci_u8;

typedef struct {
    edos_pci_u16 vendor_id;
    edos_pci_u16 device_id;
    edos_pci_u8 class_code;
    edos_pci_u8 subclass;
    edos_pci_u8 programming_interface;
    edos_pci_u8 revision;
    edos_pci_u8 header_type;
    edos_pci_u8 bus;
    edos_pci_u8 slot;
    edos_pci_u8 function;
} edos_pci_device_t;

typedef edos_pci_u32 (*edos_pci_config_reader_t)(
    edos_pci_u8 bus, edos_pci_u8 slot, edos_pci_u8 function,
    edos_pci_u8 offset, void *context);
typedef void (*edos_pci_device_callback_t)(
    const edos_pci_device_t *device, void *context);

/* Testable enumerator: the reader abstracts PCI configuration mechanism #1. */
edos_pci_u32 edos_pci_enumerate_with_reader(
    edos_pci_config_reader_t reader, void *reader_context,
    edos_pci_device_callback_t callback, void *callback_context);
/* Real x86 legacy PCI configuration-space enumerator. */
edos_pci_u32 edos_pci_enumerate(
    edos_pci_device_callback_t callback, void *callback_context);

#endif
