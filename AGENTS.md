# CognexSensor / CognexMCU

This is STM32F405 firmware for a FuSa1 carrier, imported as the STM32CubeIDE project **CognexMCU**.

- Poll PA9 through carrier J20 pin 1; LOW means board present (ON); HIGH means no board (OFF). J20 pin 3 is ground. The external input interface must supply 0–3.3 V.
- Orange WARNING LED21 is PC5, active HIGH, and follows the sensor even without a USB connection.
- USB CDC uses PA11/PA12. Emit `ON\r\n` or `OFF\r\n` only when the sampled state changes. No unsolicited startup/reconnect line.
- `SENS?` followed by CR/LF or LF is the only command; reply with the current ON/OFF line. Do not add counters, other commands, CAN, PWM, or relay behavior unless requested.
- Main application: `Core/Src/sensor_serial.c`. GPIO initialization/polarity: `Core/Src/main.c`, `Core/Inc/main.h`. Keep `CognexMCU.ioc` consistent.
- Build: `pwsh -File review/build.ps1`. Pass `-ToolBin` when needed; the script also detects an ARM toolchain on PATH.
- Check: `python tools/verify_firmware.py`. Native protocol/LED tests: `gcc -std=c11 -Wall -Wextra -Werror -Itests/stubs -ICore/Inc Core/Src/sensor_serial.c tests/test_sensor_serial.c -o tests/test_sensor_serial`, then run the executable.
- GitHub Actions builds and checks the project and publishes downloadable firmware artifacts.
- Compilation and software checks do not establish electrical or physical USB/sensor/LED operation. Do not connect to hardware or program a device unless the user requests it.
- Local CB_02 historical files are outside the shared project; do not restore that application's CAN/PWM initialization.
