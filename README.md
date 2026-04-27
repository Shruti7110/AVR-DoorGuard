# AVR-DoorGuard
A custom bootloader and door monitoring application for the ATmega328P (Arduino Uno).

The bootloader performs hardware health checks on every power-on before handing control to the application. The application continuously monitors a door's state using an ADXL345 accelerometer and a hall effect sensor, with temperature monitoring via an LM35 sensor.

---

## Repository Structure

```
AVR-DoorGuard/
├── MyBootloader/          # Bootloader project (Atmel Studio)
│   ├── main.c
│   ├── uart.c / uart.h
│   ├── i2c.c / i2c.h
│   └── adxl345.c / adxl345.h
│
├── MyApp/                 # Application project (Atmel Studio)
│   ├── main.c
│   ├── uart.c / uart.h
│   ├── i2c.c / i2c.h
│   └── adxl345.c / adxl345.h
│
└── README.md
```

---

## Hardware

| Component | Description |
|-----------|-------------|
| ATmega328P | Arduino Uno |
| ADXL345 | Accelerometer via I2C (SDO → GND, address 0x53) |
| LM35 | Analog temperature sensor on A1 |
| Hall Effect Sensor | 3.3V when door closed, <0.3V when open, on A0 |
| LED | Built-in on PB5 (Pin 13) |
| USBasp | Programmer for bootloader upload |

### Wiring

| Sensor | Arduino Pin |
|--------|------------|
| ADXL345 SDA | A4 |
| ADXL345 SCL | A5 |
| LM35 OUT | A1 |
| Hall Sensor OUT | A0 |

---

## Bootloader

### What It Does
Runs at every power-on before the application. Performs four hardware checks:

1. **ADXL345** — Verifies device ID (`0xE5`). Saves XYZ baseline (door closed position) to EEPROM.
2. **Internal Temperature** — ATmega328P internal sensor must read 0–85°C.
3. **External Temperature** — LM35 on A1 must read 0–85°C.
4. **Voltage** — Hall sensor supply on A0 must be above 3.0V.

If all checks pass, bootloader jumps to application via watchdog reset. If any check fails, an error code is blinked on the LED and the system halts.

### Error Codes

| Blinks | Error |
|--------|-------|
| 2 | ADXL345 not found |
| 3 | Internal temperature out of range |
| 4 | Voltage too low |
| 5 | External temperature out of range |
| 1 slow | No application found in flash |

### Linker Settings (Atmel Studio)

| Setting | Value |
|---------|-------|
| Memory Settings | `.text=0x7C00` |
| Other Linker Flags | `-Wl,--section-start=.text=0x7C00` |

### Fuse Settings

| Fuse | Value | Meaning |
|------|-------|---------|
| lfuse | `0xFF` | 16MHz external crystal |
| hfuse | `0xDA` | Jump to bootloader on reset, 2KB section |
| efuse | `0xFD` | Brown-out detection at 2.7V |

---

## Application

### What It Does
Reads ADXL345 baseline from EEPROM (saved by bootloader) and enters a continuous monitoring loop every 500ms:

- Reads live ADXL345 XYZ and compares to baseline (±20 counts threshold)
- Reads hall sensor voltage on A0
- Reads LM35 temperature on A1
- Reports door state over UART at 9600 baud

### Door State Logic

| Condition | State |
|-----------|-------|
| ADXL within ±20 of baseline AND voltage > 3.0V | DOOR CLOSED |
| ADXL outside ±20 of baseline AND voltage < 0.3V | DOOR OPEN |
| Temperature > 85°C | HALT — overheat warning |

Both sensors must agree for a definitive door open/closed reading, providing redundancy against false triggers.

### No Linker Settings Needed
Application compiles to default address `0x0000`.

---

## Uploading

### Tools Required (Atmel Studio External Tools)

#### 1. Full Chip Erase
```
-C"...avrdude.conf" -v -patmega328p -cusbasp -Pusb -e -Ulock:w:0xFF:m
```
Use when chip is locked. Clears all flash and resets lock bits.

#### 2. Set Bootloader Fuses
```
-C"...avrdude.conf" -v -patmega328p -cusbasp -Pusb -Uefuse:w:0xFD:m -Uhfuse:w:0xDA:m -Ulfuse:w:0xFF:m
```
Run once per Arduino after chip erase.

#### 3. Upload Bootloader via USBasp
```
-C"...avrdude.conf" -v -patmega328p -cusbasp -Pusb "-Uflash:w:$(ProjectDir)Debug\$(TargetName).hex:i"
```

#### 4. Upload Application via USBasp
```
-C"...avrdude.conf" -v -patmega328p -cusbasp -Pusb -D "-Uflash:w:$(ProjectDir)Debug\$(TargetName).hex:i"
```
Note the `-D` flag — prevents chip erase, preserving the bootloader.

### Upload Order (First Time)
1. Chip Erase
2. Set Fuses
3. Build and upload bootloader
4. Build and upload application (with `-D`)
5. Power cycle → 3 fast blinks → checks run → app starts

---

## UART Output (9600 baud)

**Bootloader:**
```
========================
   BOOTLOADER READY
========================

Running system checks...
Checking ADXL345... OK
Baseline stored
Checking Temperature... OK
Checking external temp... OK
Checking Voltage... OK

ALL CHECKS PASSED
JUMPING TO APP
```

**Application:**
```
DOOR CLOSED
DOOR CLOSED
DOOR OPEN
DOOR OPEN
DOOR CLOSED
```

---

## EEPROM Map

| Address | Data | Size |
|---------|------|------|
| 0x00 | ADXL baseline X | 2 bytes |
| 0x02 | ADXL baseline Y | 2 bytes |
| 0x04 | ADXL baseline Z | 2 bytes |

---

## Built With
- Atmel Studio 7
- AVR-GCC
- avrdude 6.3
- USBasp programmer

---

## License
MIT
