/*
 * fram.h
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#ifndef INC_FRAM_MAIN_H_
#define INC_FRAM_MAIN_H_


#include "stdbool.h"
#include "stint.h"


#define FRAM_TPU                      (1)    /* Must wait 1ms after power up to access */
#define FRAM_DEFAULT_TIMEOUT          (1000) /* Timeout in ms for events */

#define FRAM_OPCODE_WRSR              (0x01)
#define FRAM_OPCODE_WRITE             (0x02)
#define FRAM_OPCODE_READ              (0x03)
#define FRAM_OPCODE_WRDI              (0x04)
#define FRAM_OPCODE_RDSR              (0x05)
#define FRAM_OPCODE_WREN              (0x06)

#define FRAM_SR_WEN                   (1 << 1)
#define FRAM_SR_BP0                   (1 << 2)
#define FRAM_SR_BP1                   (1 << 3)
#define FRAM_SR_WPEN                  (1 << 7)

#define FRAM_START_ADDR               (0x00)
#define FRAM_SIZE                     (1 << 13)                           /* Size in bytes */
#define FRAM_END_ADDR                 ((FRAM_START_ADDR + FRAM_SIZE) - 1) /* 0x1fff */
#define FRAM_HALF_SIZE                (1 << 12)
#define FRAM_UPPER_HALF_START_ADDR    (0x1000)
#define FRAM_QUARTER_SIZE             (1 << 11)
#define FRAM_UPPER_QUARTER_START_ADDR (0x1800)


typedef enum {
    FRAM_OK      = HAL_OK,
    FRAM_ERROR   = HAL_ERROR,
    FRAM_BUSY    = HAL_BUSY,
    FRAM_TIMEOUT = HAL_TIMEOUT,
    FRAM_NOT_IMPLEMENTED_ERROR,
    FRAM_PARAMETER_ERROR,
    FRAM_IO_ERROR, /* Written data != read data */
} fram_status_t;

typedef enum {
    FRAM_VARIANT_FM25CL64B,
} fram_variant_t;

typedef enum {
    FRAM_PROTECT_ALL,
    FRAM_PROTECT_NONE,
    FRAM_PROTECT_UPPER_HALF,
    FRAM_PROTECT_UPPER_QUARTER,
} fram_block_protect_t;

typedef enum {
    FRAM_PIN_RESET,
    FRAM_PIN_SET
} fram_pinstate_t;

typedef fram_status_t (*fram_callback_spi_transmit_t)(const uint32_t *data, uint16_t size, void *context);
typedef fram_status_t (*fram_callback_spi_receive_t)(uint32_t *data, uint16_t size, void *context);
typedef void (*fram_callback_write_pin_t)(fram_pinstate_t state);

typedef struct {
    fram_callback_spi_transmit_t callback_spi_transmit;   /* Write data via SPI */
    fram_callback_spi_receive_t  callback_spi_receive;    /* Read data via SPI */
    fram_callback_write_pin_t    callback_write_cs_pin;   /* Write to the cs GPIO pin */
    fram_callback_write_pin_t    callback_write_wp_pin;   /* Write to the write-protect GPIO pin */
    fram_callback_write_pin_t    callback_write_hold_pin; /* Write to the hold GPIO pin */
} fram_callbacks_t;

typedef struct {
    const fram_callbacks_t *callbacks;
    void *                  callback_context;
    bool                    configured;
    fram_variant_t          variant;
    fram_block_protect_t    block_protect;
} fram_handle_t;


fram_status_t fram_init(fram_handle_t *dev,fram_variant_t variant, fram_callbacks_t callbacks, void *callback_context);

fram_status_t fram_write(fram_handle_t *dev, uint16_t addr, const uint8_t *data, uint16_t size);
fram_status_t fram_read(fram_handle_t *dev, uint16_t addr, uint8_t *data, uint16_t size);
fram_status_t fram_test(fram_handle_t *dev, uint16_t addr, uint8_t data);

fram_status_t fram_enable_wp(fram_handle_t *dev);
fram_status_t fram_disable_wp(fram_handle_t *dev);
fram_status_t fram_set_block_protection(fram_handle_t *dev, fram_block_protect_t protect);


#endif /* INC_FRAM_MAIN_H_ */
