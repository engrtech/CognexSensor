#include "main.h"
#include "sensor_serial.h"
#include "usbd_cdc_if.h"
#include <string.h>

#define QUEUE_SIZE 64U
#define COMMAND_SIZE 16U

static volatile uint8_t sensor_state;
static volatile uint8_t usb_connected;
static volatile uint8_t reset_pending;
static volatile uint8_t tx_complete;
static volatile uint8_t rx_overflow;
static volatile uint8_t event_overflow;
static volatile uint8_t rx_head, rx_tail;
static volatile uint8_t tx_head, tx_tail;
static uint8_t rx_queue[QUEUE_SIZE];
static uint8_t tx_queue[QUEUE_SIZE];
static uint8_t tx_buffer[5];
static uint8_t tx_started;
static uint8_t last_sent_state, last_sent_valid;
static char command[COMMAND_SIZE];
static uint8_t command_length, discard_command;

static uint8_t read_sensor(void)
{
  return (HAL_GPIO_ReadPin(SENSOR_IN_GPIO_Port, SENSOR_IN_Pin) == SENSOR_ACTIVE_STATE);
}

/* Foreground caller holds the interrupt lock against USB callbacks. */
static void enqueue_state(uint8_t state)
{
  uint8_t next = (uint8_t)((tx_head + 1U) % QUEUE_SIZE);
  if (next == tx_tail)
  {
    event_overflow = 1U;
    return;
  }
  tx_queue[tx_head] = state;
  __DMB();
  tx_head = next;
}

void SensorSerial_Init(void)
{
  /* Establish the startup baseline without emitting an unsolicited line. */
  sensor_state = read_sensor();
  HAL_GPIO_WritePin(WARNING_LED_GPIO_Port, WARNING_LED_Pin,
                    sensor_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void poll_sensor(void)
{
  uint8_t state = read_sensor();
  if (state != sensor_state)
  {
    sensor_state = state;
    HAL_GPIO_WritePin(WARNING_LED_GPIO_Port, WARNING_LED_Pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    if (usb_connected != 0U)
    {
      enqueue_state(state);
    }
  }
}

void SensorSerial_USBConnectionFromISR(uint8_t connected)
{
  /* Never replay board arrivals from an old/disconnected USB session. */
  usb_connected = connected;
  rx_head = rx_tail = 0U;
  tx_head = tx_tail = 0U;
  rx_overflow = event_overflow = 0U;
  tx_complete = 0U;
  reset_pending = 1U;
}

void SensorSerial_ReceiveFromISR(const uint8_t *data, uint32_t length)
{
  if (rx_overflow != 0U)
  {
    return;
  }
  for (uint32_t i = 0U; i < length; i++)
  {
    uint8_t next = (uint8_t)((rx_head + 1U) % QUEUE_SIZE);
    if (next == rx_tail)
    {
      rx_overflow = 1U;
      return;
    }
    rx_queue[rx_head] = data[i];
    __DMB();
    rx_head = next;
  }
}

void SensorSerial_TransmitCompleteFromISR(void)
{
  tx_complete = 1U;
}

static void consume_byte(uint8_t byte)
{
  if ((byte == '\r') || (byte == '\n'))
  {
    if (discard_command == 0U)
    {
      while ((command_length != 0U) && (command[command_length - 1U] == ' '))
      {
        command_length--;
      }
      command[command_length] = '\0';
      if (strcmp(command, "SENS?") == 0)
      {
        /* Queries intentionally return a line even if the state is unchanged. */
        enqueue_state(read_sensor());
      }
    }
    command_length = discard_command = 0U;
    return;
  }
  if (discard_command != 0U)
  {
    return;
  }
  if (((byte < 0x20U) && (byte != '\t')) || (byte > 0x7EU) ||
      (command_length >= (COMMAND_SIZE - 1U)))
  {
    command_length = 0U;
    discard_command = 1U;
    return;
  }
  if ((byte == ' ') || (byte == '\t'))
  {
    if ((command_length == 0U) || (command[command_length - 1U] == ' '))
    {
      return;
    }
    byte = ' ';
  }
  if ((byte >= 'a') && (byte <= 'z'))
  {
    byte = (uint8_t)(byte - 'a' + 'A');
  }
  command[command_length++] = (char)byte;
}

void SensorSerial_Process(void)
{
  uint32_t interrupt_state = __get_PRIMASK();
  __disable_irq();
  if (reset_pending != 0U)
  {
    reset_pending = 0U;
    /* Reconnect establishes a baseline without replaying old arrivals. */
    SensorSerial_Init();
    command_length = discard_command = 0U;
    tx_started = last_sent_valid = 0U;
    tx_complete = 0U;
  }
  /* Poll once each nonblocking main-loop pass; no sensor interrupt or delay. */
  poll_sensor();
  if (tx_complete != 0U)
  {
    tx_complete = 0U;
    tx_started = 0U;
  }
  __set_PRIMASK(interrupt_state);

  for (uint32_t i = 0U; i < QUEUE_SIZE; i++)
  {
    interrupt_state = __get_PRIMASK();
    __disable_irq();
    if ((reset_pending != 0U) || (usb_connected == 0U))
    {
      __set_PRIMASK(interrupt_state);
      return;
    }
    if (rx_overflow != 0U)
    {
      rx_tail = rx_head;
      rx_overflow = 0U;
      command_length = 0U;
      discard_command = 1U;
    }
    /* Backpressure query parsing while the transmit queue is full. */
    if ((rx_tail == rx_head) || (((tx_head + 1U) % QUEUE_SIZE) == tx_tail))
    {
      __set_PRIMASK(interrupt_state);
      break;
    }
    uint8_t byte = rx_queue[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1U) % QUEUE_SIZE);
    consume_byte(byte);
    __set_PRIMASK(interrupt_state);
  }

  interrupt_state = __get_PRIMASK();
  __disable_irq();
  if ((usb_connected != 0U) && (reset_pending == 0U) && (tx_started == 0U))
  {
    /* A long host stall can exhaust the queue. Recover the current state once
       queued events drain; disconnected operation is not an event recorder. */
    if ((tx_tail == tx_head) && (event_overflow != 0U))
    {
      event_overflow = 0U;
      if ((last_sent_valid == 0U) || (last_sent_state != sensor_state))
      {
        enqueue_state(sensor_state);
      }
    }
    if (tx_tail != tx_head)
    {
      uint8_t state = tx_queue[tx_tail];
      uint16_t length = (state != 0U) ? 4U : 5U;
      memcpy(tx_buffer, (state != 0U) ? "ON\r\n" : "OFF\r\n", length);
      tx_complete = 0U;
      if (CDC_Transmit_FS(tx_buffer, length) == USBD_OK)
      {
        tx_tail = (uint8_t)((tx_tail + 1U) % QUEUE_SIZE);
        last_sent_state = state;
        last_sent_valid = tx_started = 1U;
      }
      /* On BUSY the queue entry remains for the next pass. The buffer is kept
         alive until the USB completion callback before another line is sent. */
    }
  }
  __set_PRIMASK(interrupt_state);
}
