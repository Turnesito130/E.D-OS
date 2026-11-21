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
    puts("PCI_ENUMERATION_TESTS_OK");
    return 0;
}
