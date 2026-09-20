# M5Stack Cardputer v1.1 Board Implementation Plan

## Project Goal

Implement board support for **M5Stack Cardputer v1.1** in the xiaozhi-esp32-cardputer-v1.1 project.

## Hardware Specifications

### MCU & Core
- **MCU:** ESP32-S3FN8 (Stamp-S3A module)
- **Flash:** 8MB
- **PSRAM:** None (disabled with CONFIG_SPIRAM=n)

### Display
- **Type:** ST7789V2 LCD
- **Size:** 1.14 inch
- **Resolution:** 240x135
- **Interface:** SPI (3-wire)
- **Pins:**
  - MOSI: GPIO35
  - SCLK: GPIO36
  - CS: GPIO37
  - DC: GPIO34
  - RST: GPIO33
  - Backlight: GPIO38

### Audio
- **Microphone:** SPM1423 (PDM digital MEMS)
  - SCK (CLK): GPIO43
  - DIN (DAT): GPIO46
- **Speaker:** NS4168 I2S amplifier
  - BCLK: GPIO41
  - WS (LRCK): GPIO43
  - DOUT: GPIO42
- **Note:** GPIO43 is shared between PDM mic clock and I2S speaker WS. Uses different I2S peripherals (I2S_NUM_0 for PDM, I2S_NUM_1 for speaker).
- **No I2C audio codec** (unlike Cardputer ADV which uses ES8311)

### Keyboard
- **Controller:** 74HC138 3-to-8 line decoder
- **Decoder pins:**
  - A0: GPIO8
  - A1: GPIO9
  - A2: GPIO11
- **Input rows (7 pins with pull-up):**
  - GPIO7, GPIO6, GPIO5, GPIO4, GPIO3, GPIO15, GPIO13
- **Matrix:** 8 columns × 7 rows = 56 possible keys
- **Logical layout:** 4 rows × 14 columns (standard QWERTY keyboard)

### SD Card
- **Interface:** SPI
- **Pins:**
  - MISO: GPIO39
  - MOSI: GPIO14
  - SCLK: GPIO40
  - CS: GPIO12

### Battery
- **ADC:** GPIO10 (ADC1_CHANNEL_2)
- **Unit:** ADC_UNIT_1

### Buttons
- **BOOT:** GPIO0

## Reference Sources

1. **Official M5Stack Demo:** https://github.com/m5stack/M5Cardputer-UserDemo
   - Keyboard scanner: `main/hal/keyboard/keyboard.cpp`
   - Battery ADC: `main/hal/bat/adc_read.c`
   - SD card: `main/hal/sdcard/sdcard.cpp`

2. **Existing Implementation:** `main/boards/m5stack-cardputer-adv/`
   - TCA8418 keyboard driver (I2C)
   - WiFi config UI
   - Reference for board structure

## Implementation Status (Updated)

### Completed ✓
- [x] Directory created: `main/boards/m5stack-cardputer-v11/`
- [x] `config.h` - Board pin definitions
- [x] `hc138_keyboard.h` - Keyboard class declaration
- [x] `config.json` - Build configuration
- [x] `hc138_keyboard.cc` - Keyboard driver implementation
- [x] `wifi_config_ui.h` - WiFi config UI header
- [x] `wifi_config_ui.cc` - WiFi config UI implementation
- [x] `m5stack_cardputer_v11.cc` - Board implementation
- [x] Modified `main/CMakeLists.txt` - Added board config option
- [x] Modified `main/Kconfig.projbuild` - Added board config option
- [x] **Build tested** - Compiles successfully with 8MB Flash config
- [x] **Flash to device** - Successfully flashed via COM14 on 2026-09-20
- [x] **Partition table verified** - Uses `partitions/v2/8m.csv` (not 16m!)

### Pending ⏳
- [ ] Verify keyboard scanning
- [ ] Verify audio (PDM mic + I2S speaker)
- [ ] Verify display
- [ ] Verify battery ADC
- [ ] Verify SD card

## No-PSRAM Implications

Since the M5Stack Cardputer v1.1 has NO PSRAM (`CONFIG_SPIRAM=n`):

### Automatically Disabled Features
| Feature | Reason | Config |
|---------|--------|--------|
| AFE Audio Processor | Requires PSRAM for memory allocation | `USE_AUDIO_PROCESSOR` depends on SPIRAM |
| AFE Wake Word | Requires PSRAM for AFE processing | `USE_AFE_WAKE_WORD` depends on SPIRAM |
| Custom Wake Word (Multinet) | Requires PSRAM | `USE_CUSTOM_WAKE_WORD` depends on SPIRAM |
| Device-Side AEC | Depends on USE_AUDIO_PROCESSOR | Auto-disabled |
| Server-Side AEC | Depends on USE_AUDIO_PROCESSOR | Auto-disabled |
| LVGL Image Cache | Only works with PSRAM | `#if CONFIG_SPIRAM` guard in lcd_display.cc |

### Default Behavior for v1.1
- **Wake Word:** Disabled (`WAKE_WORD_DISABLED`) - no voice activation
- **Audio Processing:** No noise suppression, no AFE
- **Memory:** All allocations use internal DRAM (~8MB available)
- **Display:** LVGL image cache disabled (PNG optimization not available)

### Workarounds Applied
- Board config.json includes `"CONFIG_SPIRAM=n"` to prevent accidental PSRAM enable
- Board manifest sets `"psram_size": 0` to indicate no PSRAM hardware
- `lcd_display.cc` uses `#if CONFIG_SPIRAM` guard to skip PSRAM-dependent code

### Recommendations for Developers
1. Do NOT add `esp_psram` component dependencies to this board's CMakeLists.txt
2. Do NOT use `AFE_MEMORY_ALLOC_MORE_PSRAM` - use `AFE_MEMORY_ALLOC_MORE_DRAM` instead
3. Monitor free heap size - no PSRAM means less memory for audio buffers
4. Consider lowering audio sample rate if memory pressure is high

## Build System

### ESP-IDF Location
```
C:\Users\Asus\esp\v5.5.2\esp-idf
```

### Build Commands
```powershell
# Set up ESP-IDF environment
$env:IDF_PATH = "C:\Users\Asus\esp\v5.5.2\esp-idf"
& "$env:IDF_PATH\export.ps1"

# Build for v1.1
idf.py set-target esp32s3
idf.py set-config BOARD_TYPE_M5STACK_CARDPUTER_V11=y
idf.py build

# Flash
idf.py -p COMx flash

# Monitor
idf.py monitor
```

## Flash & Partition Configuration

### ⚠️ IMPORTANT: 8MB Flash Only (NO 16MB!)

The M5Stack Cardputer v1.1 has **8MB Flash** only, NOT 16MB. Do NOT configure for 16MB partition table.

### sdkconfig Settings (VERIFIED WORKING)
```
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/8m.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions/v2/8m.csv"
```

### Build Output (8MB config)
```
xiaozhi.bin binary size 0x27a240 bytes (2.5MB)
Smallest app partition is 0x2f0000 bytes (3MB)
0x75dc0 bytes (16%) free
```

### Flash Command (8MB)
```bash
python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 build\bootloader\bootloader.bin 0x8000 build\partition_table\partition-table.bin 0xd000 build\ota_data_initial.bin 0x20000 build\xiaozhi.bin 0x60000 build\generated_assets.bin
```

### Successful Flash Test (2026-09-20)
- Port: COM14
- Chip: ESP32-S3 (QFN56) (revision v0.2)
- Flash: 8MB (GD)
- MAC: 48:ca:43:b7:5a:98
- All partitions flashed successfully

### Partition Layout (8MB)
| Partition | Offset | Size |
|-----------|--------|------|
| bootloader | 0x0000 | 16KB |
| nvs | 0x9000 | 16KB |
| partition table | 0x8000 | 8KB |
| otadata | 0xd000 | 8KB |
| phy_init | 0xf000 | 4KB |
| ota_0 | 0x20000 | 3MB |
| ota_1 | - | 3MB |
| assets (SPIFFS) | 0x600000 | 2MB |

### Common Mistakes to Avoid
- ❌ Do NOT use `partitions/v2/16m.csv`
- ❌ Do NOT set `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`
- ❌ Do NOT set `CONFIG_PARTITION_TABLE_FILENAME="partitions/v2/16m.csv"`
- ❌ If building for 16MB, partition table will fail: "Partitions tables occupies 16.0MB of flash which does not fit in configured flash size 8MB"

### Build Commands
```powershell
# Set up ESP-IDF environment
$env:IDF_PATH = "C:\Users\Asus\esp\v5.5.2\esp-idf"
& "$env:IDF_PATH\export.ps1"

# Build for v1.1 (8MB Flash, No PSRAM)
idf.py set-target esp32s3
idf.py set-config BOARD_TYPE_M5STACK_CARDPUTER_V11=y
idf.py build

# Flash
idf.py -p COMx flash

# Monitor
idf.py monitor
```

## Key Design Decisions

### 1. Keyboard Scanning Approach
- Use FreeRTOS task (priority 5) for polling at ~40Hz
- Scan all 8 decoder outputs sequentially
- Map (decoder_col, input_bit) to logical (row, col) using lookup tables
- Support both KeyEvent callback (full HID) and LegacyKey callback (navigation)

### 2. Audio Configuration
- Use `NoAudioCodecSimplexPdm` class (same as Cardputer ADV)
- Configure two I2S peripherals:
  - I2S_NUM_0: PDM mic mode (SCK=GPIO43, DIN=GPIO46)
  - I2S_NUM_1: I2S speaker mode (BCLK=GPIO41, WS=GPIO43, DOUT=GPIO42)
- GPIO43 shared but safe because different I2S peripherals

### 3. WiFi Config UI
- Reuse pattern from `m5stack-cardputer-adv/wifi_config_ui.cc`
- Support both scan mode and saved networks mode
- Keyboard navigation with arrow keys, ENTER to select, ESC to cancel

## Files to Create

### `hc138_keyboard.cc`
```c
// Key features:
// - 74HC138 decoder control via GPIO8/9/11
// - Input scanning via GPIO7/6/5/4/3/15/13
// - FreeRTOS task for non-blocking scanning
// - Key mapping table: (dec_col, in_bit) -> (log_row, log_col)
// - Supports KeyEvent and LegacyKey callbacks
```

### `wifi_config_ui.h/cc`
```c
// Key features:
// - State machine: SCANNING -> SELECT_WIFI -> INPUT_PASSWORD -> CONNECTING
// - Also supports: SAVED_LIST -> INPUT_PASSWORD
// - Keyboard navigation
// - Render to display
```

### `m5stack_cardputer_v11.cc`
```c
// Key features:
// - Initialize SPI display (ST7789V2)
// - Initialize boot button (GPIO0)
// - Initialize 74HC138 keyboard
// - Configure audio (PDM mic + I2S speaker)
// - Handle key events (volume, brightness, WiFi config)
```

## Testing Checklist

### Completed ✓
- [x] Compile without errors
- [x] Flash to device via COM14 (2026-09-20)

### Pending ⏳
- [ ] Keyboard scans all 56 positions
- [ ] Arrow keys map correctly (UP/DOWN/LEFT/RIGHT)
- [ ] Enter key works in menus
- [ ] ESC cancels WiFi config
- [ ] W key enters WiFi scan mode
- [ ] S key shows saved networks
- [ ] Volume control (UP/DOWN keys)
- [ ] Brightness control (LEFT/RIGHT keys)
- [ ] Boot button toggles chat state
- [ ] Battery ADC reads correctly
- [ ] SD card mounts
- [ ] Audio capture/playback works
- [ ] Display renders correctly

## Notes

1. The keyboard scan mapping is inferred from M5Stack demo. May need verification against actual PCB wiring.
2. GPIO43 shared pin requires careful I2S configuration to avoid conflicts.
3. No PSRAM means limited memory for LVGL/display operations.
