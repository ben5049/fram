/*
 * fram.c
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stddef.h"

#include "fram.h"


#define FRAM_SPI_TRANSMIT(h, d, s) ((h)->callbacks->callback_spi_transmit(d, s, (h)->callback_context))
#define FRAM_SPI_RECEIVE(h, d, s)  ((h)->callbacks->callback_spi_receive(d, s, (h)->callback_context))
#define FRAM_CS_PIN_WRITE(h, s)    ((h)->callbacks->callback_write_cs_pin((s), (h)->callback_context))
#define FRAM_WP_PIN_WRITE(h, s)    ((h)->callbacks->callback_write_wp_pin((s), (h)->callback_context))
#define FRAM_HOLD_PIN_WRITE(h, s)  ((h)->callbacks->callback_write_hold_pin((s), (h)->callback_context))
#define FRAM_LOG_INFO(h, f, ...)   ((h)->callbacks->callback_log_info((h)->callback_context, (f), ##__VA_ARGS__))
#define FRAM_LOG_ERROR(h, f, ...)  ((h)->callbacks->callback_log_error((h)->callback_context, (f), ##__VA_ARGS__))


/* Private function prototypes */
static fram_status_t fram_write_enable(fram_handle_t *dev);
static fram_status_t fram_write_disable(fram_handle_t *dev);
static fram_status_t fram_read_sr(fram_handle_t *dev, uint8_t *sr);
static fram_status_t fram_write_sr(fram_handle_t *dev, uint8_t sr);


fram_status_t fram_init(fram_handle_t *dev, fram_variant_t variant, const fram_callbacks_t *callbacks, void *callback_context) {

    fram_status_t status = FRAM_OK;

    /* Check the callbacks */
    if (callbacks->callback_spi_transmit == NULL) status = FRAM_PARAMETER_ERROR;
    if (callbacks->callback_spi_receive == NULL) status = FRAM_PARAMETER_ERROR;
    if (callbacks->callback_write_cs_pin == NULL) status = FRAM_PARAMETER_ERROR;
    if (callbacks->callback_write_wp_pin == NULL) status = FRAM_PARAMETER_ERROR;
    if (callbacks->callback_write_hold_pin == NULL) status = FRAM_PARAMETER_ERROR;
    if (callbacks->callback_log_info == NULL) status = FRAM_PARAMETER_ERROR;
    if (callbacks->callback_log_error == NULL) status = FRAM_PARAMETER_ERROR;
    if (status != FRAM_OK) return status;

    dev->configured       = false;
    dev->variant          = variant;
    dev->block_protect    = FRAM_PROTECT_ALL; /* Default is protect all blocks */
    dev->callbacks        = callbacks;
    dev->callback_context = callback_context;

    /* Set pins to default state */
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
    FRAM_HOLD_PIN_WRITE(dev, FRAM_PIN_SET);
    FRAM_WP_PIN_WRITE(dev, FRAM_PIN_RESET);

    /* Enable WP pin functionality */
    status = fram_enable_wp(dev);
    if (status != FRAM_OK) return status;

    /* Set the default protection status (all) */
    status = fram_set_block_protection(dev, dev->block_protect);
    if (status != FRAM_OK) return status;

    /* Clear the write enable latch to be safe */
    status = fram_write_disable(dev);
    if (status != FRAM_OK) return status;

    /* Update the device struct */
    dev->configured = true;

    FRAM_LOG_INFO(dev, "FRAM Initialised");

    return status;
}


static fram_status_t fram_write_enable(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WREN;

    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_RESET);
    status = FRAM_SPI_TRANSMIT(dev, &opcode, sizeof(opcode));
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);

    return status;
}


static fram_status_t fram_write_disable(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WRDI;

    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_RESET);
    status = FRAM_SPI_TRANSMIT(dev, &opcode, sizeof(opcode));
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);

    return status;
}


static fram_status_t fram_read_sr(fram_handle_t *dev, uint8_t *sr) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_RDSR;

    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_RESET);

    status = FRAM_SPI_TRANSMIT(dev, &opcode, sizeof(opcode));
    if (status != FRAM_OK) goto end;

    status = FRAM_SPI_RECEIVE(dev, sr, sizeof(*sr));
    if (status != FRAM_OK) goto end;

end:
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
    return status;
}


static fram_status_t fram_write_sr(fram_handle_t *dev, uint8_t sr) {

    fram_status_t status   = FRAM_OK;
    uint8_t       opcode   = FRAM_OPCODE_WRSR;
    uint8_t       reg_data = 0;

    status = fram_write_enable(dev);
    if (status != FRAM_OK) return status;

    FRAM_WP_PIN_WRITE(dev, FRAM_PIN_SET);
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_RESET);

    status = FRAM_SPI_TRANSMIT(dev, &opcode, sizeof(opcode));
    if (status != FRAM_OK) {
        FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
        FRAM_WP_PIN_WRITE(dev, FRAM_PIN_RESET);
        return status;
    }

    status = FRAM_SPI_TRANSMIT(dev, &sr, sizeof(sr));
    if (status != FRAM_OK) {
        FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
        FRAM_WP_PIN_WRITE(dev, FRAM_PIN_RESET);
        return status;
    }

    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
    FRAM_WP_PIN_WRITE(dev, FRAM_PIN_RESET);

    /* Read the status register */
    status = fram_read_sr(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Check the write was successful */
    if (reg_data != sr) {
        status = FRAM_IO_ERROR;
        FRAM_LOG_ERROR(dev, "FRAM failed to write SR");
        return status;
    }

    return status;
}


fram_status_t fram_enable_wp(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Read the status register */
    status = fram_read_sr(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Check and update the status register */
    if (!(reg_data & FRAM_SR_WPEN)) {
        reg_data |= FRAM_SR_WPEN;

        /* Write to the status register */
        status = fram_write_sr(dev, reg_data);
        if (status != FRAM_OK) return status;
    }

    return status;
}


fram_status_t fram_disable_wp(fram_handle_t *dev) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Read the status register */
    status = fram_read_sr(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Check and update the status register */
    if (reg_data & FRAM_SR_WPEN) {
        reg_data &= ~FRAM_SR_WPEN;

        /* Write to the status register */
        status = fram_write_sr(dev, reg_data);
        if (status != FRAM_OK) return status;
    }

    return status;
}


fram_status_t fram_set_block_protection(fram_handle_t *dev, fram_block_protect_t protect) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Read the status register */
    status = fram_read_sr(dev, &reg_data);
    if (status != FRAM_OK) return status;

    /* Determine the new status register value */
    switch (protect) {
        case FRAM_PROTECT_NONE:
            reg_data &= ~FRAM_SR_BP0;
            reg_data &= ~FRAM_SR_BP1;
            break;

        case FRAM_PROTECT_UPPER_QUARTER:
            reg_data |= FRAM_SR_BP0;
            reg_data &= ~FRAM_SR_BP1;
            break;

        case FRAM_PROTECT_UPPER_HALF:
            reg_data &= ~FRAM_SR_BP0;
            reg_data |= FRAM_SR_BP1;
            break;

        case FRAM_PROTECT_ALL:
            reg_data |= FRAM_SR_BP0;
            reg_data |= FRAM_SR_BP1;
            break;

        default:
            status = FRAM_PARAMETER_ERROR;
            FRAM_LOG_ERROR(dev, "FRAM invalid block protection setting");
            break;
    }
    if (status != FRAM_OK) return status;

    /* Write the status register */
    status = fram_write_sr(dev, reg_data);
    if (status != FRAM_OK) return status;

    /* Update the device struct */
    dev->block_protect = protect;

    return status;
}


fram_status_t fram_write(fram_handle_t *dev, uint16_t addr, const uint8_t *data, uint16_t size) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_WRITE;
    uint8_t       addr_bytes[sizeof(addr)];

    status = fram_write_enable(dev);
    if (status != FRAM_OK) return status;

    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_RESET);

    /* Send the opcode */
    status = FRAM_SPI_TRANSMIT(dev, &opcode, sizeof(opcode));
    if (status != FRAM_OK) goto end;

    /* Send the address */
    addr_bytes[0] = (uint8_t)((addr >> 8) & 0x1f); /* 13-bit address */
    addr_bytes[1] = (uint8_t)(addr & 0xff);
    status  = FRAM_SPI_TRANSMIT(dev, addr_bytes, sizeof(addr_bytes));
    if (status != FRAM_OK) goto end;

    /* Send the data */
    status = FRAM_SPI_TRANSMIT(dev, data, size);

end:
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
    return status;
}


fram_status_t fram_read(fram_handle_t *dev, uint16_t addr, uint8_t *data, uint16_t size) {

    fram_status_t status = FRAM_OK;
    uint8_t       opcode = FRAM_OPCODE_READ;
    uint8_t       addr_bytes[sizeof(addr)];

    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_RESET);

    /* Send the opcode */
    status = FRAM_SPI_TRANSMIT(dev, &opcode, sizeof(opcode));
    if (status != FRAM_OK) goto end;

    /* Send the address */
    addr_bytes[0] = (uint8_t)((addr >> 8) & 0x1f); /* 13-bit address */
    addr_bytes[1] = (uint8_t)(addr & 0xff);
    status  = FRAM_SPI_TRANSMIT(dev, addr_bytes, sizeof(addr_bytes));
    if (status != FRAM_OK) goto end;

    /* Read the data */
    status = FRAM_SPI_RECEIVE(dev, data, size);

end:
    FRAM_CS_PIN_WRITE(dev, FRAM_PIN_SET);
    return status;
}


/* Test the FRAM is working by writing and then reading back a byte */
fram_status_t fram_test(fram_handle_t *dev, uint16_t addr, uint8_t data) {

    fram_status_t status = FRAM_OK;
    uint8_t       reg_data;

    /* Write the test data */
    status = fram_write(dev, addr, &data, sizeof(data));
    if (status != FRAM_OK) return status;

    /* Read the test data */
    status = fram_read(dev, addr, &reg_data, sizeof(data));
    if (status != FRAM_OK) return status;

    /* Check the test data */
    if (reg_data != data) {
        status = FRAM_IO_ERROR;
        FRAM_LOG_ERROR(dev, "fram_test failed");
        return status;
    }

    return status;
}
