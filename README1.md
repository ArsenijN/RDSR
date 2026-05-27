# RDSR — Portable RDS FM Radio

Arduino Nano V3 · SSD1306 128×64 · EC11 Encoder · Si4703 · MicroSD

---

## Wiring

| Arduino Nano | Peripheral            | Note                          |
|--------------|-----------------------|-------------------------------|
| A4 (SDA)     | OLED SDA + Si4703 SDA | I²C shared bus                |
| A5 (SCL)     | OLED SCL + Si4703 SCL | I²C shared bus                |
| D2           | Encoder CLK           | INT0 — interrupt-capable      |
| D3           | Encoder DT            | INT1 — interrupt-capable      |
| D4           | Encoder SW            | Button (active-low, pull-up)  |
| D8           | SD CS                 | Chip select                   |
| D9           | Si4703 RESET          | Required by Si4703 library    |
| D10          | (keep as OUTPUT)      | SPI master SS — must be output|
| D11          | SD MOSI               |                               |
| D12          | SD MISO               |                               |
| D13          | SD SCK                |                               |
| 3.3V         | OLED VCC, Si4703 VCC  | Via level converter for Si4703|
| GND          | All grounds           |                               |

**Level converter:** Si4703 is a 3.3 V device. Use your level converter on
SDA/SCL/RESET lines between Nano (5 V) and Si4703 (3.3 V).
The SSD1306 module usually has its own 3.3 V regulator and is 5 V tolerant on
I²C lines, but check your specific module's datasheet.

**Encoder EC11:** The 3-pin side is CLK / DT / SW (GND shared). The 2-pin side
is the momentary push-switch (NO contact between SW pin and GND).
Wire the 2-pin side between D4 and GND — the firmware uses INPUT_PULLUP, so no
external resistor needed.

---

## Libraries (Arduino Library Manager)

- **U8g2** by olikraus — display driver
- **SD** — built-in with Arduino IDE
- **SPI** / **Wire** / **EEPROM** — built-in

When Si4703 is added:
- **SparkFun Si4703 FM Radio Receiver Arduino Library** by SparkFun Electronics

---

## SD Card Setup

Format the card as **FAT16 or FAT32**.

Directory structure:
```
/
├── menu.cfg          ← menu definition (see sd_example/menu.cfg)
└── bitmaps/
    ├── sd_card.xbm   ← raw XBM bitmap files (see below)
    └── ...
```

### Creating bitmap files for the SD card

XBM bitmaps must be saved as **raw binary** (just the byte array, no C header).
The easiest way using ImageMagick on Linux:

```bash
# Convert any image to 8×8 XBM, then strip the C header, save raw bytes:
convert your_icon.png -resize 8x8 -monochrome icon.xbm
python3 -c "
import re, sys
data = open('icon.xbm').read()
nums = re.findall(r'0x[0-9a-fA-F]+', data)
open('sd_card.xbm', 'wb').write(bytes(int(x,16) for x in nums))
"
```

Then copy `sd_card.xbm` to `/bitmaps/` on the SD card.

Update `menu.cfg` with matching `bmpW` and `bmpH` values.

---

## Screen flow

```
Power on
  └─► Boot splash (2.2 s)
        ├─► First boot? ──► Welcome screen ──► [press encoder] ──► Settings
        └─► Normal boot
              ├─► "Remember last screen" OFF ──► Home screen
              └─► "Remember last screen" ON  ──► last used screen
```

Navigation:
- **Rotate encoder** — scroll through menu items / tune frequency
- **Short press** — select / confirm
- **Long press** — back (in Settings: go to Home)

---

## EEPROM layout

| Address | Value           | Description                  |
|---------|-----------------|------------------------------|
| 0x00    | 0xAD (sentinel) | 0xFF = first boot not done   |
| 0x01    | screen index    | Last active screen           |
| 0x02    | 0/1             | Language (0=EN, 1=UA)        |
| 0x03    | 0/1             | Remember last screen setting |

---

## MVP scope (this build)

- [x] Boot splash (from Lopaka draft)
- [x] First-boot welcome screen
- [x] Settings menu (SD-loaded or built-in fallback)
- [x] Home screen (UI only — frequency display, RSSI bars, RDS area)
- [x] Encoder navigation (rotate + short press + long press)
- [x] EEPROM persistence (first-boot flag, last screen, language)
- [x] SD card menu loader (labels + XBM bitmaps)

## Next milestones

- [ ] Si4703 init and I²C comms (using RESET on D9)
- [ ] Actual FM tuning from Home screen
- [ ] RDS data parsing → fill RDS line on Home screen
- [ ] Language switching (EN/UA strings)
- [ ] "Remember last screen" toggle in Settings
- [ ] Firmware update from SD card (parse version file, flash via bootloader)
- [ ] Audio stream recording (requires external DAC/ADC module)
