/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_ups.c
  * @author  MCD Application Team
  * @brief   USBX Device HID UPS applicative source file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_ups.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
UX_SLAVE_CLASS_HID *hid_ups;
__IO uint8_t User_Button_State = 0U;

/* Periodic report period - send report every ~2 seconds */
#define REPORT_PERIOD_MS 2000  /* Send INPUT report every 2 seconds */

/* Debug: count GET_REPORT calls from host to understand Windows polling behavior */
static volatile uint32_t get_report_call_count = 0;

/* Default battery state - INVERTED: Start with low battery (5%) to test macOS reading */
static UPS_BatteryStateTypeDef ups_battery_state = {
  .ac_present = 0,              /* AC power NOT present - running on battery */
  .charging = 0,                /* Not charging */
  .discharging = 1,             /* Discharging - on battery */
  .below_capacity_limit = 1,    /* Below capacity limit - critical low */
  .capacity_mode = 1,           /* Capacity mode enabled */
  .rechargeable = 1,            /* Battery is rechargeable */
  .remaining_capacity = 360,    /* 360 mAh (5% of 7200 mAh) */
  .full_charge_capacity = 7200, /* 7200 mAh (typical UPS battery) */
  .design_capacity = 7200,      /* 7200 mAh design capacity */
  .voltage = 12000,             /* 12000 mV (12V nominal) */
  .config_voltage = 12000,      /* 12000 mV (12V nominal) */
  .runtime_to_empty = 10        /* 10 minutes runtime left */
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static VOID BuildUPSReport(UX_SLAVE_CLASS_HID_EVENT *hid_event);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  USBD_HID_UPS_Activate
  *         This function is called when insertion of a HID UPS device.
  * @param  hid_instance: Pointer to the hid class instance.
  * @retval none
  */
VOID USBD_HID_UPS_Activate(VOID *hid_instance)
{
  /* USER CODE BEGIN USBD_HID_UPS_Activate */

  /* Save the HID UPS instance */
  hid_ups = (UX_SLAVE_CLASS_HID *) hid_instance;

  /* USER CODE END USBD_HID_UPS_Activate */

  return;
}

/**
  * @brief  USBD_HID_UPS_Deactivate
  *         This function is called when extraction of a HID UPS device.
  * @param  hid_instance: Pointer to the hid class instance.
  * @retval none
  */
VOID USBD_HID_UPS_Deactivate(VOID *hid_instance)
{
  /* USER CODE BEGIN USBD_HID_UPS_Deactivate */
  UX_PARAMETER_NOT_USED(hid_instance);

  /* Reset the HID UPS instance */
  hid_ups = UX_NULL;

  /* USER CODE END USBD_HID_UPS_Deactivate */

  return;
}

/**
  * @brief  USBD_HID_UPS_SetReport
  *         This function is invoked when the host sends a HID SET_REPORT
  *         to the application over Endpoint 0.
  * @param  hid_instance: Pointer to the hid class instance.
  * @param  hid_event: Pointer to structure of the hid event.
  * @retval status
  */
UINT USBD_HID_UPS_SetReport(UX_SLAVE_CLASS_HID *hid_instance,
                            UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_HID_UPS_SetReport */
  UX_PARAMETER_NOT_USED(hid_instance);
  UX_PARAMETER_NOT_USED(hid_event);
  /* USER CODE END USBD_HID_UPS_SetReport */

  return status;
}

/**
  * @brief  USBD_HID_UPS_GetReport
  *         This function is invoked when host is requesting event through
  *         control GET_REPORT request.
  * @param  hid_instance: Pointer to the hid class instance.
  * @param  hid_event: Pointer to structure of the hid event.
  * @retval status
  */
UINT USBD_HID_UPS_GetReport(UX_SLAVE_CLASS_HID *hid_instance,
                            UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_HID_UPS_GetReport */
  uint8_t *buf = hid_event->ux_device_class_hid_event_buffer;
  uint8_t config_byte = 0;

  UX_PARAMETER_NOT_USED(hid_instance);

  /* DEBUG: Track how many times Windows calls GetReport */
  get_report_call_count++;

  /* Build 9-byte FEATURE report: static battery data
   * Byte 0:    Config flags (Rechargeable, CapacityMode) + 6-bit padding
   * Bytes 1-2: DesignCapacity (16-bit LE, mAh)
   * Bytes 3-4: FullChargeCapacity (16-bit LE, mAh)
   * Bytes 5-6: Voltage (16-bit LE, mV)
   * Bytes 7-8: ConfigVoltage (16-bit LE, mV)
   */
  hid_event->ux_device_class_hid_event_length = 9;

  if (ups_battery_state.rechargeable)
    config_byte |= (1 << 0);
  if (ups_battery_state.capacity_mode)
    config_byte |= (1 << 1);
  buf[0] = config_byte;

  buf[1] = (uint8_t)(ups_battery_state.design_capacity & 0xFF);
  buf[2] = (uint8_t)((ups_battery_state.design_capacity >> 8) & 0xFF);

  buf[3] = (uint8_t)(ups_battery_state.full_charge_capacity & 0xFF);
  buf[4] = (uint8_t)((ups_battery_state.full_charge_capacity >> 8) & 0xFF);

  buf[5] = (uint8_t)(ups_battery_state.voltage & 0xFF);
  buf[6] = (uint8_t)((ups_battery_state.voltage >> 8) & 0xFF);

  buf[7] = (uint8_t)(ups_battery_state.config_voltage & 0xFF);
  buf[8] = (uint8_t)((ups_battery_state.config_voltage >> 8) & 0xFF);

  /* USER CODE END USBD_HID_UPS_GetReport */

  return status;
}

/* USER CODE BEGIN 1 */

/**
  * @brief  USBX_DEVICE_HID_UPS_Task
  *         Run HID UPS task
  * @param  none
  * @retval none
  */
VOID USBX_DEVICE_HID_UPS_Task(VOID)
{
  UX_SLAVE_DEVICE *device;
  UX_SLAVE_CLASS_HID_EVENT hid_event;
  static uint32_t last_report_time = 0;
  uint32_t current_time;
  uint8_t send_report = 0;

  device = &_ux_system_slave->ux_system_slave_device;
  ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

  /* Check if the device state already configured */
  if ((device->ux_slave_device_state == UX_DEVICE_CONFIGURED) && (hid_ups != UX_NULL))
  {
    /* Check if user button is pressed to simulate battery state change */
    if (User_Button_State)
    {
      /* INVERTED LOGIC: Toggle between low battery (5%) and AC present (100%) */
      if (ups_battery_state.ac_present)
      {
        /* Simulate power failure - switch back to battery at 5% (360 mAh of 7200 mAh) */
        ups_battery_state.ac_present = 0;
        ups_battery_state.discharging = 1;
        ups_battery_state.charging = 0;
        ups_battery_state.remaining_capacity = 360; /* 5% of 7200 mAh */
        ups_battery_state.runtime_to_empty = 10; /* 10 minutes left */
        ups_battery_state.below_capacity_limit = 1; /* Critical low battery */
      }
      else
      {
        /* Simulate AC restoration with full battery */
        ups_battery_state.ac_present = 1;
        ups_battery_state.discharging = 0;
        ups_battery_state.charging = 0;
        ups_battery_state.remaining_capacity = 7200; /* 100% - full capacity */
        ups_battery_state.runtime_to_empty = 3600;
        ups_battery_state.below_capacity_limit = 0; /* AC present, not low */
      }

      /* Reset User Button state */
      User_Button_State = 0U;

      /* Force immediate report on state change */
      send_report = 1;
      last_report_time = HAL_GetTick();
    }

    /* Check if it's time to send a periodic report (every 2 seconds) */
    current_time = HAL_GetTick();
    if ((current_time - last_report_time) >= REPORT_PERIOD_MS)
    {
      send_report = 1;
      last_report_time = current_time;
    }

    /* Send INPUT report if needed */
    if (send_report)
    {
      /* Build the report */
      BuildUPSReport(&hid_event);

      /* Send an event to the hid (INPUT report on interrupt endpoint) */
      ux_device_class_hid_event_set(hid_ups, &hid_event);
    }
  }
}

/**
  * @brief  USBX_DEVICE_HID_UPS_UpdateBatteryState
  *         Update the battery state
  * @param  battery_state: Pointer to the battery state structure
  * @retval none
  */
VOID USBX_DEVICE_HID_UPS_UpdateBatteryState(UPS_BatteryStateTypeDef *battery_state)
{
  UX_SLAVE_DEVICE *device;
  UX_SLAVE_CLASS_HID_EVENT hid_event;

  device = &_ux_system_slave->ux_system_slave_device;

  /* Update the internal battery state */
  if (battery_state != NULL)
  {
    ups_battery_state = *battery_state;

    /* Automatically set below_capacity_limit flag based on battery level (10% threshold) */
    uint16_t low_battery_threshold = ups_battery_state.full_charge_capacity / 10; /* 10% of capacity */
    if (ups_battery_state.remaining_capacity <= low_battery_threshold)
    {
      ups_battery_state.below_capacity_limit = 1;
    }
    else
    {
      ups_battery_state.below_capacity_limit = 0;
    }

    /* Check if the device is configured */
    if ((device->ux_slave_device_state == UX_DEVICE_CONFIGURED) && (hid_ups != UX_NULL))
    {
      /* Build the report */
      ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));
      BuildUPSReport(&hid_event);

      /* Send an event to the hid */
      ux_device_class_hid_event_set(hid_ups, &hid_event);
    }
  }
}

/**
  * @brief  BuildUPSReport
  *         Build UPS HID report from battery state.
  * @param  hid_event: Pointer to structure of the hid event.
  * @retval None
  */
static VOID BuildUPSReport(UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
  uint8_t status_byte = 0;
  uint8_t *buf = hid_event->ux_device_class_hid_event_buffer;

  /* Build 5-byte INPUT report: dynamic battery data (sent on interrupt endpoint)
   * Bytes 0-1: RemainingCapacity (16-bit LE, mAh)
   * Bytes 2-3: RunTimeToEmpty (16-bit LE, minutes)
   * Byte 4:    Status flags (4 bits) + padding (4 bits)
   *            bit 0: ACPresent
   *            bit 1: Discharging
   *            bit 2: Charging
   *            bit 3: BelowCapacityLimit
   */
  hid_event->ux_device_class_hid_event_length = 5;

  buf[0] = (uint8_t)(ups_battery_state.remaining_capacity & 0xFF);
  buf[1] = (uint8_t)((ups_battery_state.remaining_capacity >> 8) & 0xFF);

  buf[2] = (uint8_t)(ups_battery_state.runtime_to_empty & 0xFF);
  buf[3] = (uint8_t)((ups_battery_state.runtime_to_empty >> 8) & 0xFF);

  if (ups_battery_state.ac_present)
    status_byte |= (1 << 0);
  if (ups_battery_state.discharging)
    status_byte |= (1 << 1);
  if (ups_battery_state.charging)
    status_byte |= (1 << 2);
  if (ups_battery_state.below_capacity_limit)
    status_byte |= (1 << 3);
  buf[4] = status_byte;
}

/* USER CODE END 1 */
