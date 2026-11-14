#include "main.h"
#include "sensor_serial.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

GPIO_TypeDef test_gpio_a, test_gpio_c;
static GPIO_PinState input_level, led_level;
static uint32_t irq_mask;
static uint8_t blocked, in_flight;
static uint8_t *flight_pointer;
static uint8_t flight_bytes[5];
static uint16_t flight_length;
static char lines[512][6];
static unsigned line_count;

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
  assert(port == SENSOR_IN_GPIO_Port && pin == SENSOR_IN_Pin);
  return input_level;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
  assert(port == WARNING_LED_GPIO_Port && pin == WARNING_LED_Pin);
  led_level = state;
}
uint32_t __get_PRIMASK(void) { return irq_mask; }
void __disable_irq(void) { irq_mask = 1U; }
void __set_PRIMASK(uint32_t state) { irq_mask = state; }
void __DMB(void) { }

uint8_t CDC_Transmit_FS(uint8_t *data, uint16_t length)
{
  assert(irq_mask == 1U);
  assert(!in_flight);
  if (blocked) { return USBD_BUSY; }
  assert(length == 4U || length == 5U);
  assert(line_count < 512U);
  memcpy(lines[line_count], data, length);
  lines[line_count++][length] = '\0';
  flight_pointer = data;
  memcpy(flight_bytes, data, length);
  flight_length = length;
  in_flight = 1U;
  return USBD_OK;
}

static void process(void)
{
  if (in_flight) { assert(memcmp(flight_pointer, flight_bytes, flight_length) == 0); }
  SensorSerial_Process();
  assert(irq_mask == 0U);
  if (in_flight) { assert(memcmp(flight_pointer, flight_bytes, flight_length) == 0); }
}
static void complete(void)
{
  assert(in_flight);
  assert(memcmp(flight_pointer, flight_bytes, flight_length) == 0);
  in_flight = 0U;
  SensorSerial_TransmitCompleteFromISR();
  process();
}
static void receive(const char *text)
{
  SensorSerial_ReceiveFromISR((const uint8_t *)text, (uint32_t)strlen(text));
  process();
}
static void session(GPIO_PinState state, uint8_t connected)
{
  in_flight = blocked = 0U;
  input_level = state;
  SensorSerial_Init();
  SensorSerial_USBConnectionFromISR(0U);
  process();
  if (connected) {
    SensorSerial_USBConnectionFromISR(1U);
    process();
  }
  line_count = 0U;
}

int main(void)
{
  session(GPIO_PIN_RESET, 1U);
  assert(led_level == GPIO_PIN_RESET);
  for (unsigned i = 0; i < 100U; i++) { process(); }
  assert(line_count == 0U);
  input_level = GPIO_PIN_SET;
  process();
  assert(led_level == GPIO_PIN_SET && line_count == 1U);
  assert(strcmp(lines[0], "ON\r\n") == 0);
  for (unsigned i = 0; i < 100U; i++) { process(); }
  assert(line_count == 1U);
  input_level = GPIO_PIN_RESET;
  process();
  assert(led_level == GPIO_PIN_RESET && line_count == 1U);
  complete();
  assert(line_count == 2U && strcmp(lines[1], "OFF\r\n") == 0);
  complete();
  assert(line_count == 2U);

  session(GPIO_PIN_SET, 1U);
  assert(led_level == GPIO_PIN_SET && line_count == 0U);
  receive("sen");
  assert(line_count == 0U);
  receive("s?\r\n");
  assert(line_count == 1U && strcmp(lines[0], "ON\r\n") == 0);
  complete();
  receive("  sens? \n");
  assert(line_count == 2U && strcmp(lines[1], "ON\r\n") == 0);
  complete();
  receive("*IDN?\nLED1 ON\nUNKNOWN\r\n");
  assert(line_count == 2U);
  receive("012345678901234567890\r\nSENS?\n");
  assert(line_count == 3U && strcmp(lines[2], "ON\r\n") == 0);
  complete();

  session(GPIO_PIN_RESET, 1U);
  blocked = 1U;
  input_level = GPIO_PIN_SET;
  process();
  assert(led_level == GPIO_PIN_SET && line_count == 0U);
  input_level = GPIO_PIN_RESET;
  process();
  assert(led_level == GPIO_PIN_RESET && line_count == 0U);
  blocked = 0U;
  process();
  assert(line_count == 1U && strcmp(lines[0], "ON\r\n") == 0);
  complete();
  assert(line_count == 2U && strcmp(lines[1], "OFF\r\n") == 0);
  complete();

  session(GPIO_PIN_RESET, 0U);
  input_level = GPIO_PIN_SET;
  process();
  assert(led_level == GPIO_PIN_SET && line_count == 0U);
  input_level = GPIO_PIN_RESET;
  process();
  assert(led_level == GPIO_PIN_RESET && line_count == 0U);
  SensorSerial_USBConnectionFromISR(1U);
  process();
  assert(line_count == 0U);
  receive("SENS?\n");
  assert(line_count == 1U && strcmp(lines[0], "OFF\r\n") == 0);
  complete();

  session(GPIO_PIN_RESET, 1U);
  for (unsigned i = 0; i < 128U; i++) {
    input_level = (i % 2U == 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    process();
    assert(line_count == i + 1U);
    assert(strcmp(lines[i], (i % 2U == 0U) ? "ON\r\n" : "OFF\r\n") == 0);
    complete();
  }
  puts("PASS: startup silence, change-only ON/OFF, warning LED, queries, USB buffering, disconnect, and queue wraparound.");
  return 0;
}
