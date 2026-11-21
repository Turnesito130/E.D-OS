#include <assert.h>
#include <stdio.h>
#include "pci.h"

typedef struct { unsigned int callbacks; edos_pci_device_t devices[4]; } test_state;

static edos_pci_u32 fake_read(edos_pci_u8 bus, edos_pci_u8 slot,
                              edos_pci_u8 function, edos_pci_u8 offset,
                              void *context) {
    (void)context;
    if (bus != 0U) return 0xffffffffU;
    if (slot == 0U && function == 0U) {
        if (offset == 0x00U) return 0x11111234U;
        if (offset == 0x08U) return 0x06000001U;
        if (offset == 0x0cU) return 0x00800000U;
    }
    if (slot == 0U && function == 1U) {
        if (offset == 0x00U) return 0x22221234U;
        if (offset == 0x08U) return 0x03000002U;
        if (offset == 0x0cU) return 0x00000000U;
    }
    if (slot == 1U && function == 0U) {
        if (offset == 0x00U) return 0x33338086U;
        if (offset == 0x08U) return 0x01060103U;
        if (offset == 0x0cU) return 0x00000000U;
        if (offset == 0x10U) return 0x0000c001U;
        if (offset == 0x14U) return 0xfebf0008U;
        if (offset == 0x18U) return 0x00000004U;
        if (offset == 0x1cU) return 0x00000001U;
    }
    /* A non-multifunction device must not expose function 1. */
    if (slot == 1U && function == 1U && offset == 0x00U) return 0x44448086U;
    return 0xffffffffU;
}

static void capture(const edos_pci_device_t *device, void *context) {
    test_state *state = (test_state *)context;
    assert(state->callbacks < 4U);
    state->devices[state->callbacks++] = *device;
}

int main(void) {
    test_state state = {0};
    assert(edos_pci_enumerate_with_reader(0, 0, capture, &state) == 0U);
    assert(edos_pci_enumerate_with_reader(fake_read, 0, capture, &state) == 3U);
    assert(state.callbacks == 3U);
    assert(state.devices[0].vendor_id == 0x1234U);
    assert(state.devices[0].device_id == 0x1111U);
    assert(state.devices[0].header_type == 0x80U);
    assert(state.devices[1].function == 1U);
    assert(state.devices[1].class_code == 0x03U);
    assert(state.devices[2].vendor_id == 0x8086U);
    assert(state.devices[2].subclass == 0x06U);
    assert(state.devices[2].function == 0U);
    edos_pci_bar_t bar;
    assert(edos_pci_read_bar_with_reader(&state.devices[2], 0, fake_read, 0, &bar));
    assert(bar.is_io && !bar.is_64bit && bar.base == 0xc000ULL);
    assert(edos_pci_read_bar_with_reader(&state.devices[2], 1, fake_read, 0, &bar));
    assert(!bar.is_io && !bar.is_64bit && bar.prefetchable && bar.base == 0xfebf0000ULL);
    assert(edos_pci_read_bar_with_reader(&state.devices[2], 2, fake_read, 0, &bar));
    assert(!bar.is_io && bar.is_64bit && bar.base == 0x100000000ULL);
    assert(!edos_pci_read_bar_with_reader(&state.devices[2], 3, fake_read, 0, &bar));
    assert(!edos_pci_read_bar_with_reader(&state.devices[2], 6, fake_read, 0, &bar));
    puts("PCI_ENUMERATION_TESTS_OK");
    return 0;
}
