# HID Descriptor Comparison: Our UPS vs Real UPS

## Key Differences

### 1. Collection Types

**Real UPS (from NUT issue #3227):**
```
0x09, 0x24,  /* Usage (Sink) */
0xA1, 0x00,  /* Collection (Physical) */
```

**Our UPS:**
```
0x09, 0x24,  /* Usage (Sink) */
0xA1, 0x02,  /* Collection (Logical) */
```

❌ **Difference**: Real UPS uses **Physical Collection (0x00)**, we use **Logical Collection (0x02)**

### 2. Report Types

**Real UPS:**
- Uses **FEATURE reports (0xB1)** for battery configuration/capabilities
- Uses separate **INPUT reports (0x81)** for status updates
- Has two report sections with same Report ID (0x0B)

**Our UPS:**
- Uses only **INPUT reports (0x81)** for everything
- Single report section with Report ID (0x01)

❌ **Difference**: Real UPS separates static battery info (FEATURE) from dynamic status (INPUT)

### 3. Units Definition

**Real UPS:**
```
0x67, 0x01, 0x10, 0x10, 0x00,  /* Unit (0x00101001) */
0x55, 0x00,                    /* Unit Exponent (0) */
```

**Our UPS:**
- No Unit (0x67) definitions
- No Unit Exponent (0x55) definitions

❌ **Difference**: Real UPS defines proper HID units for voltage, capacity, time

### 4. Report Structure

**Real UPS Report Structure:**
```
Feature Report (Report ID 0x0B):
- Configuration fields
- Battery capabilities (static)

Input Report (Report ID 0x0B):
- Dynamic status flags (ACPresent, Discharging, Charging, BelowCapacityLimit)
- RemainingCapacity
- RunTimeToEmpty
```

**Our UPS Report Structure:**
```
Input Report (Report ID 0x01):
- Status flags
- RemainingCapacity
- FullChargeCapacity
- DesignCapacity
- Voltage
- ConfigVoltage
- RunTimeToEmpty
```

⚠️ **Issue**: We put everything in one INPUT report including static values

### 5. Status Flags Location

**Real UPS:**
- Status flags (ACPresent, Discharging, Charging, BelowCapacityLimit) are in a **separate INPUT report section**
- Located at the END of the descriptor after all capacity/voltage fields

**Our UPS:**
- Status flags at the BEGINNING of the report
- Everything in one section

❌ **Difference**: Real UPS has status flags in separate logical INPUT collection

## Real UPS Descriptor (Decoded)

```
05 84           Usage Page (Power Device)
09 04           Usage (UPS)
A1 01           Collection (Application)
  05 84           Usage Page (Power Device)
  09 24           Usage (Sink)
  A1 00           Collection (Physical)          <-- Physical, not Logical!
    85 0B           Report ID (11)

    /* FEATURE Report Section */
    09 25           Usage (...)
    09 1F           Usage (...)
    75 04           Report Size (4)
    95 02           Report Count (2)
    15 00           Logical Minimum (0)
    25 0F           Logical Maximum (15)
    65 00           Unit (None)
    B1 03           Feature (Cnst,Var,Abs)       <-- FEATURE report!

    05 85           Usage Page (Battery System)
    09 D1           Usage (...)
    09 2C           Usage (CapacityMode)
    09 8B           Usage (Rechargeable)
    75 01           Report Size (1)
    95 03           Report Count (3)
    25 01           Logical Maximum (1)
    B1 03           Feature (Cnst,Var,Abs)       <-- FEATURE report!

    /* More FEATURE fields for static battery info */
    09 83           Usage (DesignCapacity)
    09 8C           Usage (...)
    09 8D           Usage (...)
    09 8E           Usage (...)
    75 18           Report Size (24)
    95 04           Report Count (4)
    67 01 10 10 00  Unit (0x00101001)            <-- Units defined!
    55 00           Unit Exponent (0)
    27 FE FF FF 00  Logical Maximum (16777214)
    B1 03           Feature (Cnst,Var,Abs)

    /* Voltage as FEATURE */
    05 84           Usage Page (Power Device)
    09 40           Usage (ConfigVoltage)
    75 10           Report Size (16)
    95 01           Report Count (1)
    67 21 D1 F0 00  Unit (Voltage)               <-- Voltage units!
    55 05           Unit Exponent (5)
    27 FE FF 00 00  Logical Maximum (65534)
    B1 03           Feature (Cnst,Var,Abs)

    /* Dynamic INPUT fields */
    05 85           Usage Page (Battery System)
    09 67           Usage (FullChargeCapacity)
    75 18           Report Size (24)
    95 01           Report Count (1)
    67 01 10 10 00  Unit (0x00101001)
    55 00           Unit Exponent (0)
    27 FE FF FF 00  Logical Maximum (16777214)
    B1 83           Feature (Cnst,Var,Abs,Vol)

    09 66           Usage (RemainingCapacity)
    95 01           Report Count (1)
    B1 82           Feature (Data,Var,Abs,Vol)

    09 66           Usage (RemainingCapacity)
    95 01           Report Count (1)
    81 82           Input (Data,Var,Abs,Vol)     <-- INPUT for dynamic value!

    09 68           Usage (RunTimeToEmpty)
    75 10           Report Size (16)
    95 01           Report Count (1)
    66 01 10        Unit (0x1001)
    55 00           Unit Exponent (0)
    27 FE FF 00 00  Logical Maximum (65534)
    81 83           Input (Cnst,Var,Abs,Vol)

    /* Separate INPUT section for status flags */
    05 84           Usage Page (Power Device)
    09 02           Usage (PresentStatus)
    A1 02           Collection (Logical)         <-- Separate collection!
      05 85           Usage Page (Battery System)
      09 D0           Usage (ACPresent)
      09 45           Usage (Discharging)
      09 44           Usage (Charging)
      09 42           Usage (BelowCapacityLimit)
      75 01           Report Size (1)
      95 04           Report Count (4)
      25 01           Logical Maximum (1)
      81 83           Input (Cnst,Var,Abs,Vol)   <-- Status as INPUT!
      95 04           Report Count (4)
      81 83           Input (padding)
    C0              End Collection (Logical)
  C0              End Collection (Physical)
C0              End Collection (Application)
```

## Key Insights for macOS Compatibility

### 1. Physical vs Logical Collection
Real UPS devices use **Physical Collection** for the Sink, not Logical. This might affect OS parsing.

### 2. FEATURE vs INPUT Reports
- **FEATURE reports**: Static/configuration data (DesignCapacity, ConfigVoltage, etc.)
- **INPUT reports**: Dynamic/changing data (RemainingCapacity, RunTimeToEmpty, status flags)

macOS likely only monitors **INPUT reports** for updates. Putting static values in INPUT might confuse the OS.

### 3. Separate Status Collection
Real UPS has status flags (ACPresent, Discharging, Charging) in a **separate nested Logical Collection** under PresentStatus usage.

### 4. Unit Definitions
Real devices define proper HID units:
- `0x67, 0x01, 0x10, 0x10, 0x00` for capacity (likely mAh)
- `0x67, 0x21, 0xD1, 0xF0, 0x00` for voltage (Volts)
- `0x66, 0x01, 0x10` for time (seconds/minutes)

### 5. Volatile Flags
Real UPS uses `0x82` and `0x83` flags which include **Volatile (0x80)** bit, indicating values that change frequently.

## Recommendations

To improve macOS compatibility:

1. ✅ **Change Sink collection from Logical to Physical**
2. ✅ **Split into FEATURE and INPUT reports**
   - FEATURE: DesignCapacity, FullChargeCapacity, ConfigVoltage (static)
   - INPUT: RemainingCapacity, RunTimeToEmpty, status flags (dynamic)
3. ✅ **Add Unit definitions** for proper value interpretation
4. ✅ **Create separate PresentStatus collection** for status flags
5. ✅ **Add Volatile flags** to dynamic INPUT fields

This structure would more closely match real commercial UPS devices that macOS recognizes and updates properly.

## Sources
- [Network UPS Tools - Issue #3227 (Real UPS Descriptor)](https://github.com/networkupstools/nut/issues/3227)
- [USBHID-UPS Driver Documentation](https://networkupstools.org/docs/man/usbhid-ups.html)
- [NUT Developer Guide - Creating New Drivers](https://networkupstools.org/docs/developer-guide.chunked/ar01s04.html)
