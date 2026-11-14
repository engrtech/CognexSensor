param([string]$ToolBin = '')
$ErrorActionPreference = 'Stop'
if (-not $ToolBin) {
    $gccOnPath = Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
    if ($gccOnPath) { $ToolBin = Split-Path -Parent $gccOnPath.Source }
    else { $ToolBin = 'C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin' }
}
$toolSuffix = if ($env:OS -eq 'Windows_NT') { '.exe' } else { '' }
$gcc = Join-Path $ToolBin ('arm-none-eabi-gcc' + $toolSuffix)
$objcopy = Join-Path $ToolBin ('arm-none-eabi-objcopy' + $toolSuffix)
$size = Join-Path $ToolBin ('arm-none-eabi-size' + $toolSuffix)
foreach ($tool in @($gcc, $objcopy, $size)) {
    if (-not (Test-Path -LiteralPath $tool)) { throw "ARM tool not found: $tool. Set -ToolBin or add the ARM toolchain to PATH." }
}
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    New-Item -ItemType Directory -Force review/build | Out-Null
    $flags = @('-mcpu=cortex-m4','-std=gnu11','-g3','-DDEBUG','-DUSE_HAL_DRIVER','-DSTM32F405xx','-ICore/Inc','-IDrivers/STM32F4xx_HAL_Driver/Inc','-IDrivers/CMSIS/Device/ST/STM32F4xx/Include','-IDrivers/CMSIS/Include','-IUSB_DEVICE/App','-IUSB_DEVICE/Target','-IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc','-IMiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc','-O0','-ffunction-sections','-fdata-sections','-Wall','-Wextra','--specs=nano.specs','-mfpu=fpv4-sp-d16','-mfloat-abi=hard','-mthumb')
    $patterns = @('Core/Src/*.c','Drivers/STM32F4xx_HAL_Driver/Src/*.c','USB_DEVICE/App/*.c','USB_DEVICE/Target/*.c','Middlewares/ST/STM32_USB_Device_Library/Core/Src/*.c','Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/*.c')
    $objects = @()
    foreach ($source in Get-ChildItem -Path $patterns -File) {
        $object = "review/build/$($source.BaseName).o"
        & $gcc @flags -c $source.FullName -o $object
        if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
        $objects += $object
    }
    & $gcc @flags -c Core/Startup/startup_stm32f405rgtx.s -o review/build/startup.o
    if ($LASTEXITCODE -ne 0) { throw 'Assembly failed' }
    $objects += 'review/build/startup.o'
    $link = @('-o','review/build/CognexMCU.elf','-mcpu=cortex-m4','-TSTM32F405RGTX_FLASH.ld','--specs=nosys.specs','-Wl,-Map=review/build/CognexMCU.map','-Wl,--gc-sections','-static','--specs=nano.specs','-mfpu=fpv4-sp-d16','-mfloat-abi=hard','-mthumb','-Wl,--start-group','-lc','-lm','-Wl,--end-group')
    & $gcc @objects @link
    if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
    & $objcopy -O binary review/build/CognexMCU.elf review/build/CognexMCU.bin
    if ($LASTEXITCODE -ne 0) { throw 'Binary export failed' }
    & $size review/build/CognexMCU.elf
} finally { Pop-Location }
