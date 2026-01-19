#ifndef DHT_H
#define DHT_H

#include <stdint.h>
#include "stm32f3xx.h"

#define DHT_DATA_LEN_BYTES 5
#define DHT_PRESCALER 63              /* Prescaler for 1MHz timer at 64MHz clock */
#define DHT_THRESHOLD_TICKS 2000      /* Bit discrimination threshold */
#define DHT_DEFAULT_TIMEOUT 100000    /* Default timeout cycles */

/**
 * @brief Supported GPIO ports
 */
typedef enum {
    DHT_GPIO_PORT_A,
    DHT_GPIO_PORT_B,
    DHT_GPIO_PORT_F
} DHT_GPIO_Port_t;

/**
 * @brief Supported timers
 */
typedef enum {
    DHT_TIMER_3,
    DHT_TIMER_7,
    DHT_TIMER_15,
    DHT_TIMER_16,
    DHT_TIMER_17
} DHT_Timer_t;

/**
 * @brief DHT sensor error codes
 */
typedef enum {
    DHT_OK = 0,
    DHT_ERROR_INVALID_CONFIG = -1,
    DHT_ERROR_TIMEOUT = -2,
    DHT_ERROR_CHECKSUM = -3,
    DHT_ERROR_INIT_FAILED = -4
} DHT_Status_t;

/**
 * @brief DHT sensor configuration structure
 */
typedef struct {
    DHT_GPIO_Port_t gpio_port;      /**< GPIO port enum (A, B, F) */
    uint16_t pin;                   /**< Pin number on the port (0-15) */
    DHT_Timer_t timer;              /**< Timer enum (3, 7, 15, 16, 17) */
    uint32_t timeout_cycles;        /**< Timeout for handshake loops (e.g., 100000) */
} DHT_Config_t;

/**
 * @brief Initialize DHT sensor with configuration
 * Configures the GPIO pin (open-drain output) and timer
 * @param config Pointer to DHT_Config_t structure
 * @return DHT_Status_t status code
 */
DHT_Status_t DHT_Init(DHT_Config_t *config);

/**
 * @brief Read temperature and humidity data from DHT sensor
 * @param config Pointer to DHT_Config_t structure (must be initialized first)
 * @param data Pointer to a 5-byte array to store DHT data
 * @return DHT_Status_t status code
 */
DHT_Status_t GetDHTData(DHT_Config_t *config, uint8_t *data);

#endif /* DHT_H */
