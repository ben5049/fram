# FRAM Driver

This is a lightweight, platform agnostic driver written in C for interfacing with ferroelectric random access memory (FRAM) chips. It supports the following devices:
- FM25CL64B (8KB)

## Initialisation

Below is an example of how to initialise and use the driver.

```C
#include "fram.h"

fram_handle_t hfram;

const fram_callbacks_t fram_callbacks = {
    .callback_spi_transmit   = &fram_spi_transmit,
    .callback_spi_receive    = &fram_spi_receive,
    .callback_write_cs_pin   = &fram_write_cs_pin,
    .callback_write_wp_pin   = &fram_write_wp_pin,
    .callback_write_hold_pin = &fram_write_hold_pin,
    .callback_log_info       = &fram_log_info,
    .callback_log_error      = &fram_log_error,
};

uint32_t fram_callback_context = 42;

int main(){

    fram_init(&hfram, FRAM_VARIANT_FM25CL64B, &fram_callbacks, &fram_callback_context);

    ...

    uint16_t addr = 0x100;
    uint8_t data = 100;
    fram_set_block_protection(&hfram, FRAM_PROTECT_NONE); /* Default protection is FRAM_PROTECT_ALL so must be disabled before writing */
    fram_write(&hfram, addr, &data, sizeof(data));
    fram_set_block_protection(&hfram, FRAM_PROTECT_ALL);

    ...
}
```
