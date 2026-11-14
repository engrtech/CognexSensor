from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import argparse
import shutil
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description='Verify the built CognexMCU image and project configuration.')
parser.add_argument('--tool-bin', default='')
args = parser.parse_args()
nm_on_path = shutil.which('arm-none-eabi-nm')
if args.tool_bin:
    tool = Path(args.tool_bin)
elif nm_on_path:
    tool = Path(nm_on_path).parent
else:
    tool = Path('C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin')
nm = tool / ('arm-none-eabi-nm.exe' if (tool / 'arm-none-eabi-nm.exe').exists() else 'arm-none-eabi-nm')
elf = root / 'review/build/CognexMCU.elf'
binary = (root / 'review/build/CognexMCU.bin').read_bytes()
symbols_text = subprocess.check_output([str(nm), '--defined-only', str(elf)], text=True)
symbols = {}
for line in symbols_text.splitlines():
    match = re.match(r'([0-9A-Fa-f]+)\s+(\w)\s+(\S+)$', line)
    if match:
        symbols[match[3]] = (int(match[1], 16), match[2])
for name, irq in [('OTG_FS_IRQHandler', 67)]:
    address, kind = symbols[name]
    assert kind == 'T', f'{name} is not a strong handler'
    vector = struct.unpack_from('<I', binary, 4 * (16 + irq))[0]
    assert vector == (address | 1), f'{name} vector mismatch'
for name in ['SensorSerial_Process', 'poll_sensor', 'USBD_CDC_TransmitPacket']:
    assert name in symbols, f'Missing {name}'
assert not any(name.startswith(('CAN_APP_', 'PWM_', 'UART_Provision', 'SCPI_LED_')) for name in symbols)
assert 'SensorSerial_EdgeFromISR' not in symbols
assert symbols['EXTI9_5_IRQHandler'][1] == 'W'
assert struct.unpack_from('<I', binary, 4 * (16 + 23))[0] == (symbols['Default_Handler'][0] | 1)
assert ET.parse(root / '.project').findtext('name') == 'CognexMCU'
source_roots = {node.get('name') for node in ET.parse(root / '.cproject').iter('entry')
                if node.get('kind') == 'sourcePath'}
assert source_roots == {'Core', 'Drivers', 'Middlewares', 'USB_DEVICE'}, source_roots
assert not any((root / 'Core/Src').glob('can_app.c'))
ioc = dict(line.split('=', 1) for line in (root / 'CognexMCU.ioc').read_text().splitlines() if '=' in line)
assert ioc['ProjectManager.ProjectName'] == 'CognexMCU'
assert ioc['PA9.Signal'] == 'GPIO_Input'
assert 'NVIC.EXTI9_5_IRQn' not in ioc
assert ioc['PC5.Signal'] == 'GPIO_Output'
assert ioc['PC5.GPIO_Label'] == 'WARNING_LED'
assert ioc['PA11.Signal'] == 'USB_OTG_FS_DM'
assert ioc['PA12.Signal'] == 'USB_OTG_FS_DP'
assert ioc['RCC.PLLQCLKFreq_Value'] == '48000000'
assert ioc['RCC.HSE_VALUE'] == '8000000'
assert not any(value.startswith('CAN') for value in ioc.values())
main_header = (root / 'Core/Inc/main.h').read_text()
assert '#define SENSOR_IN_Pin GPIO_PIN_9' in main_header
assert '#define SENSOR_ACTIVE_STATE GPIO_PIN_SET' in main_header
assert '#define WARNING_LED_Pin GPIO_PIN_5' in main_header
assert '#define WARNING_LED_GPIO_Port GPIOC' in main_header
main_source = (root / 'Core/Src/main.c').read_text()
assert 'GPIO_MODE_INPUT' in main_source
assert 'HAL_NVIC_EnableIRQ(EXTI9_5_IRQn)' not in main_source
assert '__HAL_RCC_GPIOC_CLK_ENABLE()' in main_source
cdc = (root / 'USB_DEVICE/App/usbd_cdc_if.c').read_text()
for call in ['SensorSerial_ReceiveFromISR(Buf, *Len)',
             'SensorSerial_USBConnectionFromISR(1U)',
             'SensorSerial_USBConnectionFromISR(0U)',
             'SensorSerial_TransmitCompleteFromISR()']:
    assert call in cdc, call
sources = []
for folder in ['Core', 'USB_DEVICE']:
    for path in sorted((root / folder).rglob('*')):
        if path.is_file():
            sources.append({'path': str(path.relative_to(root)),
                            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
outputs = []
for extension in ['elf', 'bin', 'map']:
    path = root / f'review/build/CognexMCU.{extension}'
    outputs.append({'path': str(path.relative_to(root)), 'bytes': path.stat().st_size,
                    'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
report = {
    'project_setup_date': '2026-11-14',
    'project': str(root),
    'behavior': 'Polled ON/OFF on state change; WARNING LED21 follows input; SENS? returns current ON/OFF; no startup output',
    'input': 'FuSa1 J20 pin 1, PA9, active HIGH, 0-3.3 V; J20 pin 3 ground',
    'usb': 'Native USB CDC on PA11/PA12; 8 MHz HSE and 48 MHz USB clock',
    'arm_compile_and_link': 'passed',
    'sensor_sampling': 'Nonblocking main-loop polling; no sensor EXTI enabled',
    'warning_led': 'PC5 -> R53 (330 ohms) -> orange WARNING LED21; HIGH = on, independent of USB connection',
    'input_series_resistor': 'R10 = 100 ohms; R31 = 10k pulldown on J20 side must be included in external divider loading',
    'elf_interrupt_vectors': 'OTG_FS points to strong USB handler; EXTI9_5 remains unused/default',
    'project_configuration_checks': 'passed',
    'legacy_can_pwm_uart_code_in_linked_image': False,
    'warnings': 'Consult the build log; vendor support files may emit unused-parameter warnings',
    'hardware_flashed': False,
    'hardware_usb_enumeration_input_and_warning_led_test': 'pending',
    'software_protocol_runtime_tests': 'See the native protocol/LED test step in the Firmware build workflow',
    'source_files': sources,
    'outputs': outputs,
}
(root / 'CognexMCU_Sensor_Verification.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('Passed: ARM outputs, USB vector, polling/LED configuration, and active-source checks.')
print('Hardware programming and runtime sensor/USB/LED tests remain pending.')
