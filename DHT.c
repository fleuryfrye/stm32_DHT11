#include "DHT.h"

/* GPIO register bit manipulation macros */
#define DHT_GPIO_SET_BIT(port, pin)     ((port)->ODR |= (1U << (pin)))
#define DHT_GPIO_CLEAR_BIT(port, pin)   ((port)->ODR &= ~(1U << (pin)))
#define DHT_GPIO_READ_BIT(port, pin)    (((port)->IDR & (1U << (pin))) ? 1 : 0)

/* Timer control bit positions */
#define DHT_TIMER_CEN_BIT               0U   /* Counter enable bit position in CR1 */
#define DHT_TIMER_UIE_BIT               0U   /* Update interrupt flag bit position in SR */

/* Timer register manipulation macros */
#define DHT_TIMER_ENABLE(timer)         ((timer)->CR1 |= (1U << DHT_TIMER_CEN_BIT))
#define DHT_TIMER_DISABLE(timer)        ((timer)->CR1 &= ~(1U << DHT_TIMER_CEN_BIT))
#define DHT_TIMER_RESET_COUNT(timer)    ((timer)->CNT = 0)
#define DHT_TIMER_ENABLE_UPDATE_INT     (1 << DHT_TIMER_UIE_BIT)

/* GPIO mode configuration constants */
#define DHT_GPIO_MODER_INPUT            0x0U   /* GPIO mode: Input */
#define DHT_GPIO_MODER_OUTPUT           0x1U   /* GPIO mode: Output */
#define DHT_GPIO_MODER_BITS_PER_PIN     2U     /* 2 bits per pin in MODER register */
#define DHT_GPIO_OTYPER_OPEN_DRAIN      1U     /* OTYPER: Open-drain output */

/* GPIO register bit shift helpers */
#define DHT_GPIO_MODER_SHIFT(pin)       ((pin) * DHT_GPIO_MODER_BITS_PER_PIN)
#define DHT_GPIO_MODER_MASK(pin)        (0x3U << DHT_GPIO_MODER_SHIFT(pin))
#define DHT_GPIO_OTYPER_SHIFT(pin)      (pin)

/* DHT protocol timing */
#define DHT_START_SIGNAL_DELAY_MS       20U    /* 20ms low pulse */

/* Forward declarations for helper functions */
static GPIO_TypeDef* DHT_GetGPIOPort(DHT_GPIO_Port_t port);
static TIM_TypeDef* DHT_GetTimerInstance(DHT_Timer_t timer);
static void DHT_EnableGPIOClock(DHT_GPIO_Port_t port);
static void DHT_EnableTimerClock(DHT_Timer_t timer);
static DHT_Status_t DHT_ValidateConfig(DHT_Config_t *config);
static void DHT_SendStartSignal(GPIO_TypeDef *port, uint16_t pin);
static DHT_Status_t DHT_WaitForResponse(GPIO_TypeDef *port, uint16_t pin, uint32_t timeout_cycles);
static DHT_Status_t DHT_ReadBits(GPIO_TypeDef *port, TIM_TypeDef *timer, uint16_t pin, uint32_t timeout_cycles, uint8_t *data);
static uint8_t DHT_CalculateChecksum(uint8_t *data);

/**
 * @brief Get GPIO port register from enum
 */
static GPIO_TypeDef* DHT_GetGPIOPort(DHT_GPIO_Port_t port)
{
    switch (port) {
        case DHT_GPIO_PORT_A:
            return GPIOA;
        case DHT_GPIO_PORT_B:
            return GPIOB;
        case DHT_GPIO_PORT_F:
            return GPIOF;
        default:
            return NULL;
    }
}

/**
 * @brief Get timer instance register from enum
 */
static TIM_TypeDef* DHT_GetTimerInstance(DHT_Timer_t timer)
{
    switch (timer) {
        case DHT_TIMER_3:
            return TIM3;
        case DHT_TIMER_7:
            return TIM7;
        case DHT_TIMER_15:
            return TIM15;
        case DHT_TIMER_16:
            return TIM16;
        case DHT_TIMER_17:
            return TIM17;
        default:
            return NULL;
    }
}

/**
 * @brief Enable GPIO clock based on port enum
 */
static void DHT_EnableGPIOClock(DHT_GPIO_Port_t port)
{
    switch (port) {
        case DHT_GPIO_PORT_A:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case DHT_GPIO_PORT_B:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case DHT_GPIO_PORT_F:
            __HAL_RCC_GPIOF_CLK_ENABLE();
            break;
    }
}

/**
 * @brief Enable timer clock based on timer enum
 */
static void DHT_EnableTimerClock(DHT_Timer_t timer)
{
    switch (timer) {
        case DHT_TIMER_3:
            __HAL_RCC_TIM3_CLK_ENABLE();
            break;
        case DHT_TIMER_7:
            __HAL_RCC_TIM7_CLK_ENABLE();
            break;
        case DHT_TIMER_15:
            __HAL_RCC_TIM15_CLK_ENABLE();
            break;
        case DHT_TIMER_16:
            __HAL_RCC_TIM16_CLK_ENABLE();
            break;
        case DHT_TIMER_17:
            __HAL_RCC_TIM17_CLK_ENABLE();
            break;
    }
}

/**
 * @brief Validate DHT configuration
 */
static DHT_Status_t DHT_ValidateConfig(DHT_Config_t *config)
{
    if (config == NULL) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    if (config->pin > 15) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    if (config->timeout_cycles == 0) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    return DHT_OK;
}


/**
 * @brief Send the start signal to DHT sensor
 */
static void DHT_SendStartSignal(GPIO_TypeDef *port, uint16_t pin)
{
    DHT_GPIO_CLEAR_BIT(port, pin);
    osDelay(DHT_START_SIGNAL_DELAY_MS);
    DHT_GPIO_SET_BIT(port, pin);
}
DHT_Status_t DHT_Init(DHT_Config_t *config)
{
    /* Validate configuration */
    if (DHT_ValidateConfig(config) != DHT_OK) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    
    /* Get port and timer instances */
    GPIO_TypeDef *port = DHT_GetGPIOPort(config->gpio_port);
    TIM_TypeDef *timer = DHT_GetTimerInstance(config->timer);
    
    if (port == NULL || timer == NULL) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    
    /* Enable clocks */
    DHT_EnableGPIOClock(config->gpio_port);
    DHT_EnableTimerClock(config->timer);
    
    /* Configure GPIO pin */
    /* Set pin high initially */
    DHT_GPIO_SET_BIT(port, config->pin);
    
    /* Configure as open-drain output */
    port->OTYPER |= (DHT_GPIO_OTYPER_OPEN_DRAIN << DHT_GPIO_OTYPER_SHIFT(config->pin));
    
    /* Configure as output mode */
    port->MODER &= ~DHT_GPIO_MODER_MASK(config->pin);
    port->MODER |= (DHT_GPIO_MODER_OUTPUT << DHT_GPIO_MODER_SHIFT(config->pin));
    
    /* Initialize timer */
    timer->CR1 = 0x0000;           /* Disable, up counting */
    timer->PSC = DHT_PRESCALER;    /* Prescaler for 1MHz */
    timer->ARR = 0xFFFF;           /* Auto-reload register */
    timer->DIER |= DHT_TIMER_ENABLE_UPDATE_INT;      /* Enable update interrupt */
    
    return DHT_OK;
}

/**
 * @brief Wait for DHT response signal
 */
static DHT_Status_t DHT_WaitForResponse(GPIO_TypeDef *port, uint16_t pin, uint32_t timeout_cycles)
{
    /* Wait for DHT to pull line low */
    uint32_t timeout = timeout_cycles;
    while (DHT_GPIO_READ_BIT(port, pin) == 1 && timeout--);
    if (timeout == 0) return DHT_ERROR_TIMEOUT;
    
    /* Wait for DHT to release line high */
    timeout = timeout_cycles;
    while (DHT_GPIO_READ_BIT(port, pin) == 0 && timeout--);
    if (timeout == 0) return DHT_ERROR_TIMEOUT;
    
    /* Wait for DHT to pull line low again */
    timeout = timeout_cycles;
    while (DHT_GPIO_READ_BIT(port, pin) == 1 && timeout--);
    if (timeout == 0) return DHT_ERROR_TIMEOUT;
    
    return DHT_OK;
}

/**
 * @brief Read 40 bits from DHT sensor
 */
static DHT_Status_t DHT_ReadBits(GPIO_TypeDef *port, TIM_TypeDef *timer, uint16_t pin, uint32_t timeout_cycles, uint8_t *data)
{
    /* Initialize data array */
    for (int i = 0; i < DHT_DATA_LEN_BYTES; i++) {
        data[i] = 0;
    }

    /* Read 40 bits */
    for (int i = 0; i < 40; i++)
    {
            DHT_TIMER_RESET_COUNT(timer);
            
            /* Wait for line to go high */
            while (DHT_GPIO_READ_BIT(port, pin) == 0);
            
            /* Start timer */
            DHT_TIMER_ENABLE(timer);
            
            /* Wait for line to go low */
            while (DHT_GPIO_READ_BIT(port, pin) == 1);
            
            /* Stop timer and read count */
            DHT_TIMER_DISABLE(timer);
            uint16_t pulse_width = timer->CNT;
            
            /* If pulse width > threshold, bit is 1, otherwise bit is 0 */
            if (pulse_width > DHT_THRESHOLD_TICKS) {
                data[i / 8] |= (1 << (7 - (i % 8)));
            }

    }
    
    return DHT_OK;
}

/**
 * @brief Calculate checksum for DHT data
 */
static uint8_t DHT_CalculateChecksum(uint8_t *data)
{
    uint16_t sum = data[0] + data[1] + data[2] + data[3];
    return (uint8_t)(sum & 0xFF);
}

/**
 * @brief Read temperature and humidity data from DHT sensor
 */
DHT_Status_t GetDHTData(DHT_Config_t *config, uint8_t *data)
{
    if (config == NULL || data == NULL) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    
    GPIO_TypeDef *port = DHT_GetGPIOPort(config->gpio_port);
    TIM_TypeDef *timer = DHT_GetTimerInstance(config->timer);
    if (port == NULL || timer == NULL) {
        return DHT_ERROR_INVALID_CONFIG;
    }
    
    /* Send start signal */
    DHT_SendStartSignal(port, config->pin);
    
    /* Switch to input mode for response phase */
    port->MODER &= ~DHT_GPIO_MODER_MASK(config->pin);
    
    /* Wait for DHT response */
    DHT_Status_t status = DHT_WaitForResponse(port, config->pin, config->timeout_cycles);
    if (status != DHT_OK) {
        return status;
    }
    
    /* Read 40 bits from DHT */
    status = DHT_ReadBits(port, timer, config->pin, config->timeout_cycles, data);
    if (status != DHT_OK) {
        return status;
    }
    
    /* Verify checksum */
    uint8_t checksum = DHT_CalculateChecksum(data);
    if (checksum != data[4]) {
        return DHT_ERROR_CHECKSUM;
    }
    
    /* Switch back to output mode and set line high */
    port->MODER &= ~DHT_GPIO_MODER_MASK(config->pin);
    port->MODER |= (DHT_GPIO_MODER_OUTPUT << DHT_GPIO_MODER_SHIFT(config->pin));
    DHT_GPIO_SET_BIT(port, config->pin);
    
    return DHT_OK;
}

