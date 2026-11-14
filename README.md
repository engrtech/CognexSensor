# CognexMCU — conveyor sensor to USB serial

[![Firmware build](https://github.com/engrtech/CognexSensor/actions/workflows/firmware.yml/badge.svg)](https://github.com/engrtech/CognexSensor/actions/workflows/firmware.yml)

Repository: [engrtech/CognexSensor](https://github.com/engrtech/CognexSensor). See [REMOTE_WORK.md](REMOTE_WORK.md) for work-laptop setup, browser editing, and downloadable builds.

This firmware does one job: report the conveyor entry sensor as `ON` or `OFF` over the STM32 module's USB COM port. It is independent of the A90 PLC protocol and does not trigger or control the Cognex reader itself.

## Serial behavior

| Situation | Output |
| --- | --- |
| No board → board detected | `ON\r\n` once |
| Board detected → no board | `OFF\r\n` once |
| Sensor remains unchanged | Nothing |
| Send `SENS?\r\n` (or LF only) | Current `ON\r\n` or `OFF\r\n` |

`SENS?` is the only supported command. It is case-insensitive. There is no echo, startup banner, periodic stream, counter, relay control, or additional command set. The orange warning LED follows sensor presence locally. Unknown commands are ignored. A query deliberately returns a response even if the input has not changed.

At startup and USB reconnect, the current input becomes the baseline and no unsolicited state line is sent. Send `SENS?` to get that initial state. The sensor is polled once each nonblocking main-loop pass, with no intentional delay. A line is queued only when the sampled state changes. The orange WARNING LED21 (PC5 through R53, 330 ohms) is ON when the sensor reports board present and OFF otherwise, including at startup and while USB is disconnected. USB itself still uses its normal interrupts. Pulses shorter than a polling interval can be missed; physical timing remains to be measured. USB transmission is nonblocking, and the line buffer stays valid through the transfer-complete callback.

Up to 63 state/query lines can wait behind a USB transfer. A prolonged host stall can lose intermediate changes; the firmware then restores the latest state once queued messages drain. Input changes while USB is disconnected are not recorded or replayed. Open the COM port before loading boards. These notifications are not a guarantee of deterministic PC-side trigger timing.

## Carrier connection

Reference: `C:\Users\vvala\Documents\KiCad\9.0\projects\FuSa1`.

Use the carrier's **J20 ESC header**, which accepts ordinary Dupont jumper leads:

| J20 pin | Use |
| --- | --- |
| **1 — signal** | External input-interface output, **0–3.3 V**. Routes through R10 (100 ohms) to **PA9**. |
| **3 — GND** | Interface ground for a non-isolated divider. |
| 2 | Unconnected in the supplied carrier netlist; not a power source. |

Default polarity: HIGH = board present (`ON`), LOW = no board (`OFF`). `SENSOR_ACTIVE_STATE` in `Core/Inc/main.h` sets the polarity. The existing series R10 (100 ohms) is compatible with digital input use; with at most 1 microamp of input leakage in the 0–3.3 V range, its DC drop is at most 0.1 mV. R31 provides a 10k pulldown on the J20 side of R10 and must be included as a parallel load when choosing the external divider. The existing capacitor/protection components remain in circuit. See ST STM32F405 datasheet Table 48 for GPIO input characteristics: https://www.st.com/resource/en/datasheet/stm32f405rg.pdf .

The 24 V sensor output must not connect directly to J20 pin 1. Divider/protection values and the sensor's PNP/NPN output have not been established or tested here. This firmware assumes a valid 0–3.3 V signal at the header. No voltage-divider circuit was designed by this change.

USB uses **PA11/PA12**. CAN, PWM, motor control, and the old provisioning UART are not initialized. If this physical module has the PA11/PA12 CAN bodge wires described in CB_02's original README, their compatibility with native USB must be resolved before connection/programming. Software reassignment does not remove those wires.

## PTEM bench use

1. Program the new firmware yourself, then connect the module's USB port. No device was programmed as part of creating these files.
2. Select its COM port in PTEM. Windows should identify the product as **CognexMCU Sensor**. COM numbering is assigned by Windows.
3. Use normal serial settings such as 115200, 8-N-1, no flow control. USB CDC does not use the baud setting as a physical UART clock. DTR/RTS are not required by this firmware.
4. Send the ASCII command `SENS?` followed by CR/LF and read one complete line. With the board stationary, check that the reply matches its presence.
5. To monitor arrivals, keep the port open and read newline-terminated `ON`/`OFF` notifications without sending repeated commands. PTEM can trigger the Cognex on `ON` after the required travel delay.

Query replies and change notifications share the same format. Use a stationary input for the communication query, or account for already queued lines in the PTEM plan.

## Build and project history

Project: `C:\STM32Projects\CognexMCU`. Import as an existing STM32CubeIDE project. CubeMX configuration: `CognexMCU.ioc`. Clock reference from CMMTester: 8 MHz HSE, 168 MHz CPU, 48 MHz USB.

Standalone build from the project folder:

```powershell
.\review\build.ps1
```

Outputs: `review/build/CognexMCU.elf`, `.bin`, `.map`. The build script detects an ARM toolchain on PATH and accepts `-ToolBin` for another location. It runs on Windows or Linux with PowerShell. Building does not flash a device. GitHub Actions builds each push and provides an ELF/BIN/MAP artifact tied to that commit.

This project was originally copied from `C:\STM32Projects\CB_02`. The USB CDC stack and clock configuration come from the separate `C:\STM32Projects\CMMTester` reference. Neither source project was modified. On the original PC, legacy CB_02 sources remain under `reference/CB_02/Core`. That local archive, old fixtures, and old review records are excluded from the shared repository. The shared project contains the active sensor application and its current build outputs.

Validation: ARM compilation/linking and configuration checks are recorded in `CognexMCU_Sensor_Verification.json`. Physical USB enumeration, header polarity, warning LED behavior, and board-detection timing still require bench verification.


## Software verification

The `tests` directory exercises the actual `sensor_serial.c` with simulated GPIO and USB: startup silence, change-only ON/OFF, warning LED behavior, status queries, USB backpressure and buffer lifetime, reconnect behavior, and queue wraparound. It does not replace physical electrical/USB testing.

After an ARM build, `python tools/verify_firmware.py` checks GPIO/USB configuration, linked application symbols and interrupt vectors, and writes firmware hashes. The **Firmware build** workflow runs both checks. See [REMOTE_WORK.md](REMOTE_WORK.md) for downloading its fresh firmware artifact.
