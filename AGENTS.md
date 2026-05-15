# STM32F411 TAS5815 Audio Amplifier Project

## Build

```bash
# Keil UV4 command-line (primary)
"C:\Keil_v5\UV4\UV4.exe" -j0 -b "MDK-ARM\stm32f411totas5815.uvprojx"

# Alternatives
compile.cmd              # batch
compile_project.ps1      # PowerShell (has existence checks for UV4 + project file)
```

Toolchain: **AC6** (ARM Compiler 6), target MCU: **STM32F411CE**. JLink for upload.

Build output: `MDK-ARM/stm32f411totas5815/stm32f411totas5815.axf`

**Always compile after every code change.** Bare-metal project — errors surface only at build time.

## CubeMX Regeneration Constraint

All custom code **must** live between `/* USER CODE BEGIN ... */` and `/* USER CODE END ... */` markers in generated files (`main.c`, `gpio.c`, `i2c.c`, `tim.c`, etc.). Code outside these blocks is overwritten on `.ioc` regeneration.

Custom driver files (`tas5815.c/h`, `menu.c/h`, `oled.c/h`, `at24c02.c/h`, `font.c/h`) are fully user-owned — no markers needed.

## Architecture

Entry point: `Core/Src/main.c` → `main()` initializes peripherals then enters infinite loop.

**Main loop flow:**
1. `Menu_Process()` — handles encoder rotation + button press for all menu navigation
2. `Menu_IsPowerOffPending()` → `System_PowerOff()` (STOP mode, wake on PB9/PB0)
3. If `Menu_GetState() == MENU_STATE_VOLUME`: read TIM2 encoder → clamp 0–99 → set TAS5815 volume → render OLED volume screen with progress bar + status info
4. 3ms delay

**Custom driver files** (all in `Core/Src/` and `Core/Inc/`):
- `tas5815.c` — I2C driver for TAS5815 Class-D amplifier (I2C1, 7-bit addr 0x54)
- `oled.c` — SSD1306 OLED with double-buffering (`OLED_NewFrame` / `OLED_ShowFrame`)
- `at24c02.c` — AT24C02 EEPROM for persisting settings (I2C3, 7-bit addr 0x50)
- `menu.c` — Multi-level menu: Volume → Main → Bluetooth / Voltage / AMP / Modulation / FSW
- `font.c` — Font data (font10x10, afont16x8, font30x30, fontNum30x30)

**CubeMX-generated files** (do not edit outside markers): `gpio.c`, `adc.c`, `i2c.c`, `i2s.c`, `spi.c`, `tim.c`, `usart.c`, `usb_otg.c`, `stm32f4xx_it.c`, `stm32f4xx_hal_msp.c`

## I2C Bus Layout (Critical)

| Bus | SCL | SDA | Devices |
|-----|-----|-----|---------|
| I2C1 | PB6 | PB7 | TAS5815 (0x54) |
| I2C3 | PA8 | PB8 | OLED SSD1306 (0x3C) + AT24C02 (0x50) |

**I2C3 is shared** between OLED and EEPROM. Do not introduce concurrent I2C3 access — current code is sequential.

## Encoder Volume Mechanism

- TIM2 encoder mode on PA0/PA1
- **2 hardware counts per volume step**: `volume = raw_counter >> 1`
- Setting volume in software: `__HAL_TIM_SET_COUNTER(&htim2, volume << 1)`
- Volume range: **0–99** (`VOLUME_MAX = 99`), default 30
- Upper bound clamps counter to 198 (= volume 99); lower bound resets to 0

## EEPROM Layout

| Addr | Content | Flag Addr | Flag Value |
|------|---------|-----------|------------|
| 0x00 | Volume  | 0x01      | 0xA5       |
| 0x02 | Voltage | 0x03      | 0x5A       |
| 0x04 | Modulation | 0x05   | 0x3C       |
| 0x06 | FSW     | 0x07      | 0xC3       |
| 0x08 | PowerOff | 0x09     | 0xD7       |
| 0x0A | AMP Gain | 0x0B     | 0xE1       |

Volume save is throttled: **2-second idle** (`VOLUME_SAVE_DELAY = 2000`) before writing to EEPROM. Do not remove this delay.

All menu settings persist to EEPROM. Menu displays small dot indicator (x=113, r=4) next to currently saved option.

## Power-Off (STOP Mode)

`System_PowerOff()` sets voltage to 5V before entering STOP mode. Wake sources: PB9 (encoder key) and PB0 (USB detect). On wake: restores saved voltage from `g_currentVoltage`, clears OLED, wakes TAS5815, restores volume. Any pending EEPROM save is flushed before entering STOP.

**Auto power-off modes** (configurable in menu):
- 0: No playback 10 minutes
- 1: No playback 30 minutes (default)
- 2: Follow host (triggered by PB0 falling edge)
- 3: Disabled

## Play Detection

- ADC_CHANNEL_6 (PA6): threshold >1240 (≈1V)
- 10 samples per check; **any single sample > threshold** → START
- START state persists for 100ms after last detection
- `g_lastPlayState` used for display and auto power-off timer

## PD Voltage Control

| Voltage | PA9 (PD_5V) | PB10 (PD_20V) |
|---------|-------------|----------------|
| 5V      | HIGH        | LOW            |
| 12V     | LOW         | LOW            |
| 15V*    | LOW         | HIGH           |

*Menu displays "15V" but hardware outputs 20V (PD_20V pin HIGH). Function `SetVoltage(2)` unchanged.

Use `SetVoltage()` and `RestoreVoltage()` functions — do not write GPIO directly for voltage changes.

## Other Hardware Notes

- **ADC play detect** (ADC_CHANNEL_6 / PA6): threshold >1240 (≈1V) = playing
- **ADC voltage readout** (ADC_CHANNEL_9 / PB1): divider 51k/10k → `V = ADC * 3.3 * 6.1 / 4095`
- **I2S1** (PA5=CLK, PA7=SD, PA15=WS) — audio data path to TAS5815
- **QCC3034 Bluetooth**: 
  - PB12=pair/play (0.1s pulse for next track)
  - PB13=VOL-/prev (1s pulse for pairing after BT_EN reset)
  - PB14=VOL+/next (0.1s pulse for prev track)
  - PA10=BT enable (high=on)
  - **Pairing sequence**: PA10 low 500ms → PA10 high → PB13 high 1s → PB13 low
  - PA6=play detect, PB0=USB detect, PB15=LED0, PA4=LED1
- **USB** (PA11/PA12) and **SPI3** (PB3/PB4/PB5) — configured but unused
- **Debug**: USART2 on PA2(TX)/PA3(RX), SWD on PA13/PA14
- **PB9**: Encoder key, EXTI (short press=confirm, long press>1s=back)
- **LSE** (32.768kHz) enabled at startup with timeout protection

## Voltage Control Functions

- `SetVoltage(voltage)`: Sets voltage (0=5V, 1=12V, 2=20V*) and updates `g_currentVoltage`
- `RestoreVoltage()`: Restores voltage from `g_currentVoltage` (used after wake from STOP mode)

*Menu shows "15V" but hardware outputs 20V.
