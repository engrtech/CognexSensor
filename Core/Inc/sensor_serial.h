#ifndef SENSOR_SERIAL_H
#define SENSOR_SERIAL_H

#include <stdint.h>

void SensorSerial_Init(void);
void SensorSerial_Process(void);
void SensorSerial_USBConnectionFromISR(uint8_t connected);
void SensorSerial_ReceiveFromISR(const uint8_t *data, uint32_t length);
void SensorSerial_TransmitCompleteFromISR(void);

#endif
