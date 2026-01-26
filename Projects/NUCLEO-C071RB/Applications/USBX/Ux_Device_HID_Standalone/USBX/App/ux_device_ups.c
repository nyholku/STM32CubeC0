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

/* Default battery state - initialized to simulate a UPS on AC power with full battery */
static UPS_BatteryStateTypeDef ups_battery_state = {
  .ac_present = 1,              /* AC power is present */
  .charging = 0,                /* Not charging (battery full) */
  .discharging = 0,             /* Not discharging (on AC) */
  .below_capacity_limit = 0,    /* Above capacity limit */
  .capacity_mode = 1,           /* Capacity mode enabled */
  .rechargeable = 1,            /* Battery is rechargeable */
  .remaining_capacity = 7200,   /* 7200 mAh (100% of capacity) */
  .full_charge_capacity = 7200, /* 7200 mAh (typical UPS battery) */
  .design_capacity = 7200,      /* 7200 mAh design capacity */
  .voltage = 12000,             /* 12000 mV (12V nominal) */
  .config_voltage = 12000,      /* 12000 mV (12V nominal) */
  .runtime_to_empty = 3600      /* 3600 minutes (60 hours) runtime */
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
  UX_PARAMETER_NOT_USED(hid_instance);

  /* Build and return the current UPS report */
  BuildUPSReport(hid_event);

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
      /* Simulate battery state change - toggle between AC present and low battery */
      if (ups_battery_state.ac_present)
      {
        /* Simulate critical battery - switch to battery at 5% (360 mAh of 7200 mAh) */
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
  uint8_t config_byte, status_byte;
  uint8_t *buf = hid_event->ux_device_class_hid_event_buffer;

  /* UPS report length: 14 bytes of data (USBX adds Report ID automatically when report_id = UX_TRUE)
   * Buffer layout: data starts at buf[0], Report ID prepended by USBX */
  hid_event->ux_device_class_hid_event_length = 14;

  /* Byte 0: Static configuration flags (Rechargeable, CapacityMode) */
  config_byte = 0;
  if (ups_battery_state.rechargeable)
    config_byte |= (1 << 0);
  if (ups_battery_state.capacity_mode)
    config_byte |= (1 << 1);
  buf[0] = config_byte;

  /* Bytes 1-2: Design capacity (16-bit little-endian, mAh) */
  buf[1] = (uint8_t)(ups_battery_state.design_capacity & 0xFF);
  buf[2] = (uint8_t)((ups_battery_state.design_capacity >> 8) & 0xFF);

  /* Bytes 3-4: Full charge capacity (16-bit little-endian, mAh) */
  buf[3] = (uint8_t)(ups_battery_state.full_charge_capacity & 0xFF);
  buf[4] = (uint8_t)((ups_battery_state.full_charge_capacity >> 8) & 0xFF);

  /* Bytes 5-6: Voltage (16-bit little-endian, mV) */
  buf[5] = (uint8_t)(ups_battery_state.voltage & 0xFF);
  buf[6] = (uint8_t)((ups_battery_state.voltage >> 8) & 0xFF);

  /* Bytes 7-8: Config voltage (16-bit little-endian, mV) */
  buf[7] = (uint8_t)(ups_battery_state.config_voltage & 0xFF);
  buf[8] = (uint8_t)((ups_battery_state.config_voltage >> 8) & 0xFF);

  /* Bytes 9-10: Remaining capacity (16-bit little-endian, mAh) - DYNAMIC/VOLATILE */
  buf[9] = (uint8_t)(ups_battery_state.remaining_capacity & 0xFF);
  buf[10] = (uint8_t)((ups_battery_state.remaining_capacity >> 8) & 0xFF);

  /* Bytes 11-12: Runtime to empty (16-bit little-endian, minutes) - DYNAMIC/VOLATILE */
  buf[11] = (uint8_t)(ups_battery_state.runtime_to_empty & 0xFF);
  buf[12] = (uint8_t)((ups_battery_state.runtime_to_empty >> 8) & 0xFF);

  /* Byte 13: PresentStatus flags (ACPresent, Discharging, Charging, BelowCapacityLimit) */
  status_byte = 0;
  if (ups_battery_state.ac_present)
    status_byte |= (1 << 0);
  if (ups_battery_state.discharging)
    status_byte |= (1 << 1);
  if (ups_battery_state.charging)
    status_byte |= (1 << 2);
  if (ups_battery_state.below_capacity_limit)
    status_byte |= (1 << 3);
  buf[13] = status_byte;
}

/* USER CODE END 1 */
