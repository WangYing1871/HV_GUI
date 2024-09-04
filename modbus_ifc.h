#pragma once

/*** holding registers ***/

// MFC rate in flow-state (2 words)
// default rate is read-only
#define REG_MFC_RATE_SV 0
#define REG_MFC_RATE_DEFAULT 2
#define REG_MFC_RATE_PV 4

// pump speed in recycle-state (1 word, unit: permil of full-speed)
// default speed is read-only
#define REG_PUMP_SPEED_SV 6
#define REG_PUMP_SPEED_DEFAULT 7
#define REG_PUMP_SPEED_PV 8
#define REG_PUMP_SPEED_MIN 9
#define REG_PUMP_SPEED_MAX 10

// others
#define REG_TEMPERATURE_DEC 11
#define REG_TEMPERATURE_FRA 12
#define REG_PRESSURE_DEC 13
#define REG_PRESSURE_FRA 14

// working modes, valid values:
// - 1 -> flow-mode,
// - 2 -> recycle-mode
// - 3 -> mix-mode (auto switching between flow & recycle)
#define REG_SET_MODE 15
#define REG_CUR_MODE 16
#define REG_CUR_STATE 17
// duration of flow & recycle state in mix-mode (unit:s)
#define REG_FLOW_DURATION 18
#define REG_RECYCLE_DURATION 19

// status polling interval (in ms)
#define REG_UPDATE_INTERVAL 20

// system alive time (high-16 bits & low-16 bits)
#define REG_HEARTBEAT_LW 21
#define REG_HEARTBEAT_HI 22

#define REG_END 23

/*** coils ***/

/// control command
#define COIL_ONOFF_MFC 0
#define COIL_ONOFF_PUMP 1
#define COIL_ONOFF_VALVE 2

/// reset command (triggering with write operation)
#define COIL_DEFAULT_SPEED_PUMP 3
#define COIL_DEFAULT_RATE_MFC 4

/// peripheral status (init success or failed)
#define COIL_STATUS_MFC 5
#define COIL_STATUS_BMP280 6
#define COIL_STATUS_PUMP 7
#define COIL_STATUS_VALVE 8
#define COIL_STATUS_MODBUS 9

/// the status of last executed modbus control cmd
/// most notably: mfc set/get command success or not
#define COIL_STATUS_LATEST 10

#define COIL_END 11
