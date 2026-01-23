# Testing the HID UPS Device

## Python Test Script

The `test_ups.py` script allows you to read and monitor the battery status from the STM32 HID UPS device.

### Requirements

Install the `hidapi` library:

```bash
# Linux/macOS
pip3 install hidapi

# Windows
pip install hidapi
```

### Usage

**Single reading:**
```bash
python3 test_ups.py
```

**Continuous monitoring (poll every 2 seconds):**
```bash
python3 test_ups.py -c
```

**Continuous monitoring with custom interval:**
```bash
python3 test_ups.py -c -i 1.0  # Poll every 1 second
```

**Show raw byte data:**
```bash
python3 test_ups.py -r
```

**Specify custom VID/PID:**
```bash
python3 test_ups.py --vid 0x0483 --pid 0x5750
```

### macOS Testing

On macOS, you can compare the Python script output with the system UPS status:

```bash
# View UPS status in system
pmset -g ps

# View detailed UPS info
ioreg -l -w0 | grep -i ups -A 20

# Monitor with Python script
python3 test_ups.py -c
```

### Windows Testing

On Windows, check the UPS status in Control Panel → Power Options while running:

```bash
python test_ups.py -c
```

Press the blue USER button on the NUCLEO board to toggle between:
- **AC Power, 100% battery** (7200 mAh)
- **Battery Power, 5% critical** (360 mAh, low battery warning)

### Expected Output

```
UPS BATTERY STATUS
============================================================
Power Source:      AC Power
Charging:          Yes

Battery Level:     100.0%
Remaining:         7200 mAh
Full Capacity:     7200 mAh
Design Capacity:   7200 mAh

Voltage:           12.00 V
Config Voltage:    12.00 V

Runtime to Empty:  60h 0m

Rechargeable:      Yes
Capacity Mode:     Enabled
============================================================
```

## Report Format

The HID UPS device sends 14-byte reports:

| Bytes | Field | Type | Description |
|-------|-------|------|-------------|
| 0 | Report ID | uint8 | Always 0x01 |
| 1 | Status Flags | uint8 | 6-bit flags (see below) |
| 2-3 | Remaining Capacity | uint16 LE | Battery remaining in mAh |
| 4-5 | Full Charge Capacity | uint16 LE | Full capacity in mAh |
| 6-7 | Design Capacity | uint16 LE | Design capacity in mAh |
| 8-9 | Voltage | uint16 LE | Current voltage in mV |
| 10-11 | Config Voltage | uint16 LE | Nominal voltage in mV |
| 12-13 | Runtime to Empty | uint16 LE | Remaining runtime in minutes |

### Status Flags (Byte 1)

| Bit | Flag | Description |
|-----|------|-------------|
| 0 | Capacity Mode | Battery capacity reporting enabled |
| 1 | Below Capacity Limit | Battery critically low (≤10%) |
| 2 | Charging | Battery is charging |
| 3 | Discharging | Battery is discharging |
| 4 | AC Present | AC power connected |
| 5 | Rechargeable | Battery is rechargeable |

## Troubleshooting

**"No devices found"**
- Check that the USB cable is connected
- Try running with `sudo` on Linux/macOS
- On Windows, run Command Prompt as Administrator
- Use `--vid` and `--pid` flags if the defaults don't match your device

**"Error opening device"**
- Make sure no other program is using the device
- On Linux, you may need udev rules for non-root access
- Install hidapi: `pip3 install hidapi`

**Permission denied (Linux)**

Create a udev rule:
```bash
sudo nano /etc/udev/rules.d/99-stm32-ups.rules
```

Add:
```
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", MODE="0666"
```

Then reload:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Testing Shutdown Behavior

**Windows:**
1. Configure Power Options → UPS settings for automatic shutdown
2. Run the Python script in continuous mode
3. Press the blue button to simulate 5% battery
4. Windows should display low battery warning and potentially shut down

**macOS:**
1. The system should automatically detect the UPS
2. Run: `pmset -g ps` to verify UPS is recognized
3. Press the blue button to simulate 5% battery
4. macOS may display low battery warnings (note: macOS UPS updates are infrequent)
