#!/usr/bin/env python3
"""
HID UPS Battery Monitor
Reads and displays battery status from STM32 HID UPS device

Report Format (15 bytes total):
  Byte 0:     Report ID (0x01)
  Byte 1:     Config flags (2 bits)
              bit 0: Rechargeable
              bit 1: Capacity Mode
  Bytes 2-3:  Design Capacity (16-bit little-endian, mAh)
  Bytes 4-5:  Full Charge Capacity (16-bit little-endian, mAh)
  Bytes 6-7:  Voltage (16-bit little-endian, mV)
  Bytes 8-9:  Config Voltage (16-bit little-endian, mV)
  Bytes 10-11: Remaining Capacity (16-bit little-endian, mAh) [DYNAMIC]
  Bytes 12-13: Runtime to Empty (16-bit little-endian, minutes) [DYNAMIC]
  Byte 14:    PresentStatus flags (4 bits)
              bit 0: AC Present
              bit 1: Discharging
              bit 2: Charging
              bit 3: Below Capacity Limit
"""

import hid
import struct
import time
import sys

# HID Vendor and Product IDs for STM32 HID UPS
VENDOR_ID = 0x0483   # STMicroelectronics
PRODUCT_ID = 0x5750  # HID UPS (may need adjustment based on actual PID)

def find_ups_device(vendor_id=VENDOR_ID, product_id=PRODUCT_ID):
    """Find and open the HID UPS device"""
    print("Searching for HID UPS device...")

    # List all HID devices to help find the right one
    devices = hid.enumerate(vendor_id)

    if not devices:
        print(f"No devices found with VID 0x{vendor_id:04X}")
        print("\nAll HID devices:")
        for dev in hid.enumerate():
            print(f"  VID: 0x{dev['vendor_id']:04X} PID: 0x{dev['product_id']:04X} - {dev['product_string']}")
        return None

    # Try to find UPS device by product string
    ups_device = None
    for dev in devices:
        if 'UPS' in str(dev.get('product_string', '')).upper():
            ups_device = dev
            break

    if not ups_device and devices:
        # If no UPS found by name, use first STM32 device
        ups_device = devices[0]

    if ups_device:
        print(f"Found: {ups_device['product_string']} (VID: 0x{ups_device['vendor_id']:04X}, PID: 0x{ups_device['product_id']:04X})")

        try:
            h = hid.device()
            h.open(ups_device['vendor_id'], ups_device['product_id'])
            h.set_nonblocking(False)
            return h
        except Exception as e:
            print(f"Error opening device: {e}")
            return None

    return None

def decode_report(data):
    """Decode the 15-byte HID report"""
    if len(data) < 15:
        print(f"Warning: Expected 15 bytes, got {len(data)} bytes")
        print(f"Raw data: {' '.join(f'{b:02X}' for b in data)}")
        return None

    # Byte 0: Report ID
    report_id = data[0]

    # Byte 1: Config flags
    config = data[1]
    rechargeable = bool(config & (1 << 0))
    capacity_mode = bool(config & (1 << 1))

    # Bytes 2-3: Design Capacity (16-bit LE, mAh)
    design_capacity = struct.unpack('<H', bytes(data[2:4]))[0]

    # Bytes 4-5: Full Charge Capacity (16-bit LE, mAh)
    full_charge_capacity = struct.unpack('<H', bytes(data[4:6]))[0]

    # Bytes 6-7: Voltage (16-bit LE, mV)
    voltage = struct.unpack('<H', bytes(data[6:8]))[0]

    # Bytes 8-9: Config Voltage (16-bit LE, mV)
    config_voltage = struct.unpack('<H', bytes(data[8:10]))[0]

    # Bytes 10-11: Remaining Capacity (16-bit LE, mAh) - DYNAMIC
    remaining_capacity = struct.unpack('<H', bytes(data[10:12]))[0]

    # Bytes 12-13: Runtime to Empty (16-bit LE, minutes) - DYNAMIC
    runtime_to_empty = struct.unpack('<H', bytes(data[12:14]))[0]

    # Byte 14: PresentStatus flags
    present_status = data[14]
    ac_present = bool(present_status & (1 << 0))
    discharging = bool(present_status & (1 << 1))
    charging = bool(present_status & (1 << 2))
    below_capacity_limit = bool(present_status & (1 << 3))

    # Calculate battery percentage
    if full_charge_capacity > 0:
        battery_percent = (remaining_capacity * 100) / full_charge_capacity
    else:
        battery_percent = 0

    return {
        'report_id': report_id,
        'ac_present': ac_present,
        'charging': charging,
        'discharging': discharging,
        'below_capacity_limit': below_capacity_limit,
        'capacity_mode': capacity_mode,
        'rechargeable': rechargeable,
        'remaining_capacity': remaining_capacity,
        'full_charge_capacity': full_charge_capacity,
        'design_capacity': design_capacity,
        'voltage': voltage,
        'config_voltage': config_voltage,
        'runtime_to_empty': runtime_to_empty,
        'battery_percent': battery_percent
    }

def print_status(status):
    """Print battery status in human-readable format"""
    print("\n" + "="*60)
    print("UPS BATTERY STATUS")
    print("="*60)

    # Power status
    power_status = "AC Power" if status['ac_present'] else "Battery Power"
    print(f"Power Source:      {power_status}")

    if status['charging']:
        print(f"Charging:          Yes")
    elif status['discharging']:
        print(f"Discharging:       Yes")

    # Battery level
    print(f"\nBattery Level:     {status['battery_percent']:.1f}%")
    print(f"Remaining:         {status['remaining_capacity']} mAh")
    print(f"Full Capacity:     {status['full_charge_capacity']} mAh")
    print(f"Design Capacity:   {status['design_capacity']} mAh")

    # Voltage
    print(f"\nVoltage:           {status['voltage']/1000:.2f} V")
    print(f"Config Voltage:    {status['config_voltage']/1000:.2f} V")

    # Runtime
    hours = status['runtime_to_empty'] // 60
    minutes = status['runtime_to_empty'] % 60
    print(f"Runtime to Empty:  {hours}h {minutes}m")

    # Flags
    print(f"\nRechargeable:      {'Yes' if status['rechargeable'] else 'No'}")
    print(f"Capacity Mode:     {'Enabled' if status['capacity_mode'] else 'Disabled'}")

    # Warning
    if status['below_capacity_limit']:
        print(f"\n⚠️  WARNING: Battery below capacity limit!")

    print("="*60)

    # Raw data
    print(f"\nReport ID: 0x{status['report_id']:02X}")

def main():
    """Main function"""
    import argparse

    parser = argparse.ArgumentParser(description='Monitor STM32 HID UPS battery status')
    parser.add_argument('-c', '--continuous', action='store_true',
                       help='Continuously poll battery status')
    parser.add_argument('-i', '--interval', type=float, default=2.0,
                       help='Polling interval in seconds (default: 2.0)')
    parser.add_argument('-r', '--raw', action='store_true',
                       help='Show raw byte data')
    parser.add_argument('-v', '--vid', type=lambda x: int(x, 0), default=VENDOR_ID,
                       help=f'USB Vendor ID (default: 0x{VENDOR_ID:04X})')
    parser.add_argument('-p', '--pid', type=lambda x: int(x, 0), default=PRODUCT_ID,
                       help=f'USB Product ID (default: 0x{PRODUCT_ID:04X})')

    args = parser.parse_args()

    # Find and open device with specified VID/PID
    device = find_ups_device(args.vid, args.pid)
    if not device:
        print("\nFailed to find or open HID UPS device")
        print("\nTips:")
        print("  1. Make sure the device is plugged in")
        print("  2. Try running with sudo/administrator privileges")
        print("  3. Use --vid and --pid to specify correct IDs")
        print("  4. Install hidapi: pip install hidapi")
        return 1

    try:
        if args.continuous:
            print(f"\nPolling every {args.interval} seconds (Press Ctrl+C to stop)...")
            while True:
                try:
                    # Read report (GET_REPORT request)
                    # For HID devices, we can use get_feature_report with report ID
                    data = device.get_feature_report(0x01, 15)

                    if args.raw:
                        print(f"\nRaw data ({len(data)} bytes): {' '.join(f'{b:02X}' for b in data)}")

                    status = decode_report(data)
                    if status:
                        print_status(status)
                    else:
                        print("Error: Invalid report data")

                    time.sleep(args.interval)

                except KeyboardInterrupt:
                    print("\n\nStopped by user")
                    break
        else:
            # Single read
            data = device.get_feature_report(0x01, 14)

            if args.raw:
                print(f"\nRaw data ({len(data)} bytes): {' '.join(f'{b:02X}' for b in data)}")

            status = decode_report(data)
            if status:
                print_status(status)
            else:
                print("Error: Invalid report data")

    except Exception as e:
        print(f"\nError reading from device: {e}")
        return 1
    finally:
        device.close()

    return 0

if __name__ == '__main__':
    sys.exit(main())
