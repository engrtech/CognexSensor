# PTEM sensor trigger bench plan

[TriggerTest.TapPlan](TriggerTest.TapPlan) is the saved PTEM plan for testing the CognexMCU's USB sensor notifications. [DataMan_Serial_Commands.md](DataMan_Serial_Commands.md) lists the reader commands for subsequent integration.

## Open on another PC

Use the installed/tested versions or compatible packages:

- PTEM 2.1.2; OpenTAP 9.34.0.
- PTEM Serial Port 1.1.0, PTEM Test Steps 1.1.0, and PTEM Variables.

Open the plan and check the saved Serial Port instrument. It currently selects **COM9**, 115200, 8-N-1, no handshake. Change COM9 to the sensor MCU's actual port. The DataMan uses a separate instrument/COM port when integrated.

## Current behavior

1. Serial Read Data waits up to 10,000 ms and assigns its trimmed String output to `DUT_IN`.
2. If Else compares `DUT_IN` with `BOARD_PRESENT` (String default `ON`).
3. Its `Trigger` child prints `Trigger!`; its `No Board` child prints `No Board Yet`.

The corrected firmware uses 0 V -> ON / warning LED on, and 3.3 V -> OFF / warning LED off. The PTEM comparison remains ON = board present. Reflash the corrected firmware before checking this polarity.

The earlier physical bench stimulus was manually switching J20 pin 1 between 3.3 V and GND using the previous active-HIGH firmware; that test established communications, not the corrected physical polarity. ON/OFF reception and an ON -> DUT_IN -> true-branch print passed. An empty read timeout also selected the false branch; this does not establish a received-OFF branch test. Cached outputs in the saved plan are historical, not fresh measurements.

This is a print-only demonstration: it contains no DataMan trigger command. Actual machine-sensor conditioning, complete-line processing of combined ON/OFF reads, separate timeout handling, and conveyor timing remain to be verified. The stock read implementation polls at 10 ms and adds a 20 ms collection wait after its first nonempty read; these are not guaranteed end-to-end latency bounds.

The plan was copied unchanged from the user's saved file on 2026-11-14. Source SHA-256: `9246def7e1ff8afb2fc5b883cc5e191ef9755ccf16b7641f90a4bb4a6ff2decf`.
