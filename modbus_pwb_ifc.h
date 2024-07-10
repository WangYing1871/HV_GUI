#pragma once

/*** holding registers ***/

/// status registers
// voltage set value (V)
#define PWB_REG_SVV_CH0 0
#define PWB_REG_SVV_CH1 1
#define PWB_REG_SVV_CH2 2
#define PWB_REG_SVV_CH3 3

// voltage present value (V)
#define PWB_REG_PVV_CH0 4
#define PWB_REG_PVV_CH1 5
#define PWB_REG_PVV_CH2 6
#define PWB_REG_PVV_CH3 7

// current present value (uA)
#define PWB_REG_PVI_CH0 8
#define PWB_REG_PVI_CH1 9
#define PWB_REG_PVI_CH2 10
#define PWB_REG_PVI_CH3 11

// trip current settings (uA)
#define PWB_REG_TRIP_CH0 12
#define PWB_REG_TRIP_CH1 13
#define PWB_REG_TRIP_CH2 14
#define PWB_REG_TRIP_CH3 15

// ramp up/down settings (step in V)
#define PWB_REG_RUP_STEP 16
#define PWB_REG_RDN_STEP 17

// status polling interval (in ms)
#define PWB_REG_UPDATE_INTERVAL 18

// system alive time (high-16 bits & low-16 bits)
/// alive-time = polling-times * update_interval
#define PWB_REG_HEARTBEAT_LW 19
#define PWB_REG_HEARTBEAT_HI 20

// others
#define PWB_REG_TEMPERATURE_INT 21
#define PWB_REG_TEMPERATURE_FLT 22
#define PWB_REG_HUMIDITY_INT 23
#define PWB_REG_HUMIDITY_FLT 24
// 64-bit uuid, addr range (little endian): 25-28
#define PWB_REG_BOARD_UUID 25

// control command registers
#define PWB_REG_CLR_TRIPS 29
#define PWB_REG_RESET 30

// V and I measurment interval (better shorter than REG_UPDATE_INTERVAL)
#define REG_MEASURE_INTERVAL 31

#define PWB_REG_END 32

/*** coils ***/

/// status coils
// voltage tripped or not
#define PWB_COIL_TRIPPED_CH0 0
#define PWB_COIL_TRIPPED_CH1 1
#define PWB_COIL_TRIPPED_CH2 2
#define PWB_COIL_TRIPPED_CH3 3

/// control command coils
// turn voltage on/off
#define PWB_COIL_ONOFF_CH0 4
#define PWB_COIL_ONOFF_CH1 5
#define PWB_COIL_ONOFF_CH2 6
#define PWB_COIL_ONOFF_CH3 7

#define PWB_COIL_END 8
