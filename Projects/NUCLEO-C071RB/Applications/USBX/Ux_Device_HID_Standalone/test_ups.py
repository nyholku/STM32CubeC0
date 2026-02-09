#!/usr/bin/env python3
"""
HID UPS Battery Monitor - STM32

FEATURE report (15 bytes) - polled by host via GET_REPORT (includes all data for Windows):
  Byte 0:     Config flags (2 bits + 6 bits padding)
              bit 0: Rechargeable
              bit 1: Capacity Mode
  Bytes 1-2:  Design Capacity (16-bit LE, mAh)
  Bytes 3-4:  Full Charge Capacity (16-bit LE, mAh)
  Bytes 5-6:  Voltage (16-bit LE, mV)
  Bytes 7-8:  Config Voltage (16-bit LE, mV)
  Bytes 9-10: Remaining Capacity (16-bit LE, mAh)
  Bytes 11-12: Runtime to Empty (16-bit LE, minutes)
  Byte 13:    Relative State Of Charge (8-bit, 0-100%)
  Byte 14:    PresentStatus flags (4 bits + 4 bits padding)
              bit 0: AC Present
              bit 1: Discharging
              bit 2: Charging
              bit 3: Below Capacity Limit

INPUT report (5 bytes) - pushed by device every 2 s on interrupt endpoint (macOS uses this):
  Bytes 0-1:  Remaining Capacity (16-bit LE, mAh)
  Bytes 2-3:  Runtime to Empty (16-bit LE, minutes)
  Byte 4:     PresentStatus flags (4 bits + 4 bits padding)
"""

import hid
import struct
import time
import sys

VENDOR_ID  = 0x0483   # STMicroelectronics
PRODUCT_ID = 0x5750


def find_ups_device(vendor_id=VENDOR_ID, product_id=PRODUCT_ID):
    """Find and open the HID UPS device."""
    print("Searching for HID UPS device...")
    devices = hid.enumerate(vendor_id)

    if not devices:
        print(f"No devices found with VID 0x{vendor_id:04X}")
        print("\nAll HID devices:")
        for dev in hid.enumerate():
            print(f"  VID: 0x{dev['vendor_id']:04X} PID: 0x{dev['product_id']:04X} "
                  f"- {dev['product_string']}")
        return None

    ups_device = None
    for dev in devices:
        if 'UPS' in str(dev.get('product_string', '')).upper():
            ups_device = dev
            break
    if not ups_device:
        ups_device = devices[0]

    print(f"Found: {ups_device['product_string']} "
          f"(VID: 0x{ups_device['vendor_id']:04X}, PID: 0x{ups_device['product_id']:04X})")
    try:
        h = hid.device()
        h.open(ups_device['vendor_id'], ups_device['product_id'])
        return h
    except Exception as e:
        print(f"Error opening device: {e}")
        return None


def read_feature(device, show_raw=False):
    """Read and decode the 15-byte FEATURE report (all data including percentage).
    hidapi prepends a 0x00 byte when descriptor has no Report ID."""
    data = device.get_feature_report(0x00, 16)  # 16 = 1 prepended + 15 data

    if show_raw:
        print(f"  Raw FEATURE ({len(data)} bytes): {' '.join(f'{b:02X}' for b in data)}")

    if data and data[0] == 0x00:
        data = data[1:]

    if len(data) < 15:
        print(f"  Warning: FEATURE report too short ({len(data)} bytes)")
        return None, None

    config = data[0]
    percentage = data[13]
    flags = data[14]

    static = {
        'rechargeable':         bool(config & 0x01),
        'capacity_mode':        bool(config & 0x02),
        'design_capacity':      struct.unpack_from('<H', bytes(data), 1)[0],
        'full_charge_capacity': struct.unpack_from('<H', bytes(data), 3)[0],
        'voltage':              struct.unpack_from('<H', bytes(data), 5)[0],
        'config_voltage':       struct.unpack_from('<H', bytes(data), 7)[0],
    }

    dynamic = {
        'remaining_capacity':   struct.unpack_from('<H', bytes(data), 9)[0],
        'runtime_to_empty':     struct.unpack_from('<H', bytes(data), 11)[0],
        'percentage':           percentage,
        'ac_present':           bool(flags & 0x01),
        'discharging':          bool(flags & 0x02),
        'charging':             bool(flags & 0x04),
        'below_capacity_limit': bool(flags & 0x08),
    }

    return static, dynamic


def decode_input(data):
    """Decode a 5-byte INPUT report (dynamic data).  Returns None if too short."""
    if len(data) < 5:
        return None
    flags = data[4]
    return {
        'remaining_capacity':   struct.unpack_from('<H', bytes(data), 0)[0],
        'runtime_to_empty':     struct.unpack_from('<H', bytes(data), 2)[0],
        'ac_present':           bool(flags & 0x01),
        'discharging':          bool(flags & 0x02),
        'charging':             bool(flags & 0x04),
        'below_capacity_limit': bool(flags & 0x08),
    }


def read_input(device, show_raw=False):
    """Blocking read of the next INPUT report from the interrupt endpoint."""
    device.set_nonblocking(False)
    data = device.read(6)   # allow 6 in case of extra byte

    if show_raw:
        print(f"  Raw INPUT ({len(data)} bytes): {' '.join(f'{b:02X}' for b in data)}")

    if not data:
        return None
    # read() normally does NOT prepend, but handle it defensively
    if len(data) >= 6 and data[0] == 0x00:
        data = data[1:]
    return decode_input(data)


def print_status(static, dynamic):
    """Print combined battery status."""
    print("\n" + "=" * 55)
    print(" UPS BATTERY STATUS")
    print("=" * 55)

    if dynamic:
        print(f"  Power Source:      {'AC Power' if dynamic['ac_present'] else 'Battery Power'}")
        if dynamic['charging']:
            print(f"  Charging:          Yes")
        elif dynamic['discharging']:
            print(f"  Discharging:       Yes")

        if static and static['full_charge_capacity'] > 0:
            pct = dynamic['remaining_capacity'] * 100.0 / static['full_charge_capacity']
            print(f"  Battery Level:     {pct:.1f}%")
        print(f"  Remaining:         {dynamic['remaining_capacity']} mAh")

        h, m = divmod(dynamic['runtime_to_empty'], 60)
        print(f"  Runtime to Empty:  {h}h {m}m")

        if dynamic['below_capacity_limit']:
            print(f"  *** WARNING: Battery below capacity limit ***")
    else:
        print(f"  [Dynamic data not available]")

    if static:
        print(f"  Full Capacity:     {static['full_charge_capacity']} mAh")
        print(f"  Design Capacity:   {static['design_capacity']} mAh")
        print(f"  Voltage:           {static['voltage']/1000:.2f} V")
        print(f"  Config Voltage:    {static['config_voltage']/1000:.2f} V")
        print(f"  Rechargeable:      {'Yes' if static['rechargeable'] else 'No'}")
        print(f"  Capacity Mode:     {'Enabled' if static['capacity_mode'] else 'Disabled'}")

    print("=" * 55)


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Monitor STM32 HID UPS battery status')
    parser.add_argument('-c', '--continuous', action='store_true',
                       help='Continuously monitor (blocks on INPUT reports)')
    parser.add_argument('-r', '--raw', action='store_true',
                       help='Show raw report bytes')
    parser.add_argument('-v', '--vid', type=lambda x: int(x, 0), default=VENDOR_ID,
                       help=f'USB Vendor ID (default: 0x{VENDOR_ID:04X})')
    parser.add_argument('-p', '--pid', type=lambda x: int(x, 0), default=PRODUCT_ID,
                       help=f'USB Product ID (default: 0x{PRODUCT_ID:04X})')
    args = parser.parse_args()

    device = find_ups_device(args.vid, args.pid)
    if not device:
        print("\nFailed to find or open HID UPS device")
        print("Tips:")
        print("  1. Make sure the device is plugged in")
        print("  2. Try running with sudo/administrator privileges")
        print("  3. Install hidapi: pip install hidapi")
        return 1

    try:
        # --- FEATURE report: all data (read once via GET_REPORT) ---
        print("Reading FEATURE report (all data via GET_REPORT)...")
        static, dynamic_from_feature = read_feature(device, show_raw=args.raw)
        if static:
            print(f"  DesignCap={static['design_capacity']} "
                  f"FullCap={static['full_charge_capacity']} "
                  f"V={static['voltage']}mV")
            if dynamic_from_feature:
                print(f"  RemainingCap={dynamic_from_feature['remaining_capacity']} "
                      f"RelativeSOC={dynamic_from_feature['percentage']}%")
        else:
            print("  Failed to read FEATURE report")

        # --- INPUT report: dynamic data from interrupt endpoint (optional) ---
        if args.continuous:
            print("\nListening for INPUT reports from interrupt endpoint... (Ctrl+C to stop)\n")
            print("(Using FEATURE data as baseline)\n")
            print_status(static, dynamic_from_feature)
            while True:
                try:
                    dynamic = read_input(device, show_raw=args.raw)
                    if dynamic:
                        print_status(static, dynamic)
                except KeyboardInterrupt:
                    print("\nStopped.")
                    break
        else:
            # Single-shot: poll for one INPUT report with 5 s timeout
            print("Waiting for INPUT report from interrupt endpoint (timeout 5 s)...")
            device.set_nonblocking(True)
            dynamic_from_input = None
            deadline = time.time() + 5.0
            while time.time() < deadline:
                data = device.read(6)
                if data:
                    if args.raw:
                        print(f"  Raw INPUT ({len(data)} bytes): "
                              f"{' '.join(f'{b:02X}' for b in data)}")
                    if len(data) >= 6 and data[0] == 0x00:
                        data = data[1:]
                    dynamic_from_input = decode_input(data)
                    break
                time.sleep(0.1)

            if dynamic_from_input:
                print("  Received INPUT report from interrupt endpoint")
                print_status(static, dynamic_from_input)
            else:
                print("  No INPUT report from interrupt endpoint (using FEATURE data)")
                print_status(static, dynamic_from_feature)

    except Exception as e:
        print(f"\nError: {e}")
        return 1
    finally:
        device.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())
