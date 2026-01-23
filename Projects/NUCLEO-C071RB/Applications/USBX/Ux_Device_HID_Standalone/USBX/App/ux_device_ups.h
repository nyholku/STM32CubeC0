/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_ups.h
  * @author  MCD Application Team
  * @brief   USBX Device HID UPS applicative header file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UX_DEVICE_UPS_H__
#define __UX_DEVICE_UPS_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "ux_api.h"
#include "ux_device_class_hid.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* UPS Battery State structure */
typedef struct
{
  uint8_t ac_present;              /* AC power present flag */
  uint8_t charging;                /* Battery charging flag */
  uint8_t discharging;             /* Battery discharging flag */
  uint8_t below_capacity_limit;    /* Below capacity limit flag */
  uint8_t capacity_mode;           /* Capacity mode */
  uint8_t rechargeable;            /* Battery is rechargeable */
  uint16_t remaining_capacity;     /* Remaining battery capacity in mAh */
  uint16_t full_charge_capacity;   /* Full charge capacity in mAh */
  uint16_t design_capacity;        /* Design capacity in mAh */
  uint16_t voltage;                /* Present voltage in mV */
  uint16_t config_voltage;         /* Nominal/config voltage in mV */
  uint16_t runtime_to_empty;       /* Runtime to empty in minutes */
} UPS_BatteryStateTypeDef;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
VOID USBD_HID_UPS_Activate(VOID *hid_instance);
VOID USBD_HID_UPS_Deactivate(VOID *hid_instance);
UINT USBD_HID_UPS_SetReport(UX_SLAVE_CLASS_HID *hid_instance,
                            UX_SLAVE_CLASS_HID_EVENT *hid_event);
UINT USBD_HID_UPS_GetReport(UX_SLAVE_CLASS_HID *hid_instance,
                            UX_SLAVE_CLASS_HID_EVENT *hid_event);

/* USER CODE BEGIN EFP */
VOID USBX_DEVICE_HID_UPS_Task(VOID);
VOID USBX_DEVICE_HID_UPS_UpdateBatteryState(UPS_BatteryStateTypeDef *battery_state);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif  /* __UX_DEVICE_UPS_H__ */
