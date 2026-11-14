#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef struct { uint8_t unused; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
extern GPIO_TypeDef test_gpio_a, test_gpio_c;
#define SENSOR_IN_GPIO_Port (&test_gpio_a)
#define SENSOR_IN_Pin ((uint16_t)0x0200)
#define SENSOR_ACTIVE_STATE GPIO_PIN_RESET
#define WARNING_LED_GPIO_Port (&test_gpio_c)
#define WARNING_LED_Pin ((uint16_t)0x0020)
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t state);
void __DMB(void);
#endif
