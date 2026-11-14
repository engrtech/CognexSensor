# DataMan serial commands for PTEM

Use **DMCC**, ASCII, 115200 baud, 8-N-1, no handshake. Select the reader's COM port (previously COM7; the sensor MCU used COM9). In PTEM **Serial Write Data -> String**, enter the literal `\r\n` shown below; our USB capture verified conversion to CR/LF bytes `0D 0A`. Send one command per write. In PowerShell use backtick-r/backtick-n instead.

## Connect and scan

- Identify reader: `||>GET DEVICE.NAME\r\n`
- Check USB DTR requirement: `||>GET COM.USB-REQUIRE-DTR\r\n`
- Allow this PTEM instrument to receive without DTR: `||>SET COM.USB-REQUIRE-DTR OFF\r\n`
- Capture and decode: `||>TRIGGER ON\r\n`

Identity and single-barcode triggering passed on the bench. Triggering starts acquisition and decoding on the reader; follow it with Serial Read Data. No separate decode command is needed. A command acknowledgement is not a barcode result.

If DTR is currently required, first connect using a client with DTR asserted (PowerShell: `$p.DtrEnable = $true`) to change it to OFF. The sensor MCU's `SENS?` command belongs on its separate COM port.

## Two Data Matrix codes

Send these setup commands before triggering:

```text
||>SET MULTICODE.NUM-CODES 2\r\n
||>SET MULTICODE.MAX-NUM-CODES 1 2\r\n
||>SET MULTICODE.PARTIAL-RESULTS OFF\r\n
```

`1` selects Data Matrix. OFF requires the configured count; use `||>SET MULTICODE.PARTIAL-RESULTS ON\r\n` only when accepting fewer codes. Replace both count values for another target count. Check the configuration:

```text
||>GET MULTICODE.NUM-CODES\r\n
||>GET MULTICODE.MAX-NUM-CODES 1\r\n
||>GET MULTICODE.PARTIAL-RESULTS\r\n
```

Two distinct IDs from one trigger remain unverified. Validate both returned IDs; do not assume one serial read contains the complete result.

## Save or restart

- Save the entire current reader configuration: `||>CONFIG.SAVE\r\n`
- Optional restart; reloads saved configuration: `||>REBOOT\r\n`

Save once after intentional configuration changes, not per scan. After REBOOT, release the USB COM port and reopen after the reader returns. DTR-OFF persistence across power cycles/reconnects still needs verification.

References: [Cognex command framing and trigger example](https://docs.cognex.com/dmst_2340/EN/CommunicationsAndProgramming.pdf), [DTR](https://docs.cognex.com/dmst_2532/web/EN/DMCC/Content/Topics/idp10157077808.htm), [multicode commands](https://docs.cognex.com/dmst_618/web/DMCC/default.html), [CONFIG.SAVE](https://docs.cognex.com/dmst_2541/web/EN/DMCC/Content/Topics/idp10153719856.htm), [REBOOT](https://docs.cognex.com/dmst_616/web/DMCC/default.html). These supplement the bench observations; documentation is not proof of a completed physical test.
