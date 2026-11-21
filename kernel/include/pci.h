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

typedef struct {
    unsigned long long base;
    edos_pci_u8 index;
    edos_pci_u8 is_io;
    edos_pci_u8 is_64bit;
    edos_pci_u8 prefetchable;
} edos_pci_bar_t;

typedef edos_pci_u32 (*edos_pci_config_reader_t)(
    edos_pci_u8 bus, edos_pci_u8 slot, edos_pci_u8 function,
    edos_pci_u8 offset, void *context);
typedef void (*edos_pci_device_callback_t)(
    const edos_pci_device_t *device, void *context);

edos_pci_u32 edos_pci_enumerate_with_reader(
    edos_pci_config_reader_t reader, void *reader_context,
    edos_pci_device_callback_t callback, void *callback_context);
edos_pci_u32 edos_pci_enumerate(
    edos_pci_device_callback_t callback, void *callback_context);
int edos_pci_read_bar_with_reader(const edos_pci_device_t *device,
                                  edos_pci_u8 index,
                                  edos_pci_config_reader_t reader,
                                  void *reader_context, edos_pci_bar_t *bar);
int edos_pci_read_bar(const edos_pci_device_t *device, edos_pci_u8 index,
                      edos_pci_bar_t *bar);

#endif
