/**
 * test_messages/test_main.cpp
 *
 * Tests unitarios para Messages.h — verifica tamaños de structs,
 * serialización/deserialización mediante memcpy y coherencia de campos.
 *
 * Framework: Unity (incluido en PlatformIO)
 * Ejecutar: pio test -e test_native
 */

#include <unity.h>
#include <string.h>
#include <stdint.h>
#include "../../shared/Messages.h"

// =============================================================================
// TESTS: Tamaños de estructuras (deben caber en 8 bytes CAN)
// =============================================================================

void test_struct_sizes_fit_can_payload(void) {
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_Heartbeat_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_BCM_AlarmState_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_BCM_LightsState_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_BCM_FlameConfig_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_BCM_FlameState_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_ICM_EngineData_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_ICM_Status_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_ICM_AdcRaw_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_UCE_Engine_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_UCE_Temps_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_UCE_Ignition_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_UCE_Injection_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_CMD_Alarm_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_CMD_Lights_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_CMD_Flame_t));
    TEST_ASSERT_LESS_OR_EQUAL(CAN_MAX_DLC, sizeof(MSG_CMD_Tune_t));
}

void test_can_max_dlc_is_8(void) {
    TEST_ASSERT_EQUAL(8, CAN_MAX_DLC);
}

// =============================================================================
// TESTS: Serialización/deserialización via memcpy (simula TX/RX CAN)
// =============================================================================

void test_heartbeat_serialize_deserialize(void) {
    MSG_Heartbeat_t tx = { .status = STATUS_OK, .uptime_s = 123456 };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));

    MSG_Heartbeat_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_HEX8(STATUS_OK, rx.status);
    TEST_ASSERT_EQUAL_UINT32(123456, rx.uptime_s);
}

void test_icm_engine_data_serialize_deserialize(void) {
    MSG_ICM_EngineData_t tx = {
        .rpm      = 3500,
        .map_mbar = 950,
        .clt_c    = 85,
        .iat_c    = 30,
        .batt_dv  = 142,   // 14.2 V
    };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));

    MSG_ICM_EngineData_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_UINT16(3500,  rx.rpm);
    TEST_ASSERT_EQUAL_UINT16(950,   rx.map_mbar);
    TEST_ASSERT_EQUAL_INT8  (85,    rx.clt_c);
    TEST_ASSERT_EQUAL_INT8  (30,    rx.iat_c);
    TEST_ASSERT_EQUAL_UINT8 (142,   rx.batt_dv);
}

void test_bcm_flame_state_serialize_deserialize(void) {
    MSG_BCM_FlameState_t tx = { .active = 1, .pattern = 5, .frame = 12 };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));

    MSG_BCM_FlameState_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_UINT8(1,  rx.active);
    TEST_ASSERT_EQUAL_UINT8(5,  rx.pattern);
    TEST_ASSERT_EQUAL_UINT8(12, rx.frame);
}

void test_uce_ignition_serialize_deserialize(void) {
    MSG_UCE_Ignition_t tx = {
        .advance_cdeg = 150,   // 15.0 grados
        .dwell_us     = 3500,
        .knock        = 0,
    };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));

    MSG_UCE_Ignition_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_INT16 (150,  rx.advance_cdeg);
    TEST_ASSERT_EQUAL_UINT16(3500, rx.dwell_us);
    TEST_ASSERT_EQUAL_UINT8 (0,    rx.knock);
}

void test_cmd_tune_serialize_deserialize(void) {
    MSG_CMD_Tune_t tx = { .table = 1, .idx = 42, .value = 1234 };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));

    MSG_CMD_Tune_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_UINT8 (1,    rx.table);
    TEST_ASSERT_EQUAL_UINT8 (42,   rx.idx);
    TEST_ASSERT_EQUAL_UINT16(1234, rx.value);
}

// =============================================================================
// TESTS: Coherencia de valores límite
// =============================================================================

void test_icm_battery_voltage_encoding(void) {
    // 14.2 V se codifica como 142 (deciVoltios)
    uint8_t batt_dv = 142;
    float voltage = batt_dv / 10.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 14.2f, voltage);

    // 12.0 V
    batt_dv = 120;
    voltage = batt_dv / 10.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, voltage);
}

void test_uce_temps_encoding(void) {
    // 87.5°C → 875 (centidegrees)
    MSG_UCE_Temps_t tx = { .clt_cdeg = 875, .iat_cdeg = 285 };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));
    MSG_UCE_Temps_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    float clt = rx.clt_cdeg / 10.0f;
    float iat = rx.iat_cdeg / 10.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 87.5f, clt);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.5f, iat);
}

void test_icm_engine_data_negative_temps(void) {
    // Temperaturas negativas (invierno, arranque en frío)
    MSG_ICM_EngineData_t tx = { .clt_c = -10, .iat_c = -5 };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));
    MSG_ICM_EngineData_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_INT8(-10, rx.clt_c);
    TEST_ASSERT_EQUAL_INT8(-5,  rx.iat_c);
}

void test_adc_raw_max_values(void) {
    // ADC 12 bits → máximo 4095
    MSG_ICM_AdcRaw_t tx = { .adc0 = 4095, .adc1 = 4095, .adc2 = 0, .adc3 = 2048 };
    uint8_t buf[8] = {0};
    memcpy(buf, &tx, sizeof(tx));
    MSG_ICM_AdcRaw_t rx = {};
    memcpy(&rx, buf, sizeof(rx));

    TEST_ASSERT_EQUAL_UINT16(4095, rx.adc0);
    TEST_ASSERT_EQUAL_UINT16(4095, rx.adc1);
    TEST_ASSERT_EQUAL_UINT16(0,    rx.adc2);
    TEST_ASSERT_EQUAL_UINT16(2048, rx.adc3);
}

// =============================================================================
// MAIN
// =============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_struct_sizes_fit_can_payload);
    RUN_TEST(test_can_max_dlc_is_8);
    RUN_TEST(test_heartbeat_serialize_deserialize);
    RUN_TEST(test_icm_engine_data_serialize_deserialize);
    RUN_TEST(test_bcm_flame_state_serialize_deserialize);
    RUN_TEST(test_uce_ignition_serialize_deserialize);
    RUN_TEST(test_cmd_tune_serialize_deserialize);
    RUN_TEST(test_icm_battery_voltage_encoding);
    RUN_TEST(test_uce_temps_encoding);
    RUN_TEST(test_icm_engine_data_negative_temps);
    RUN_TEST(test_adc_raw_max_values);

    return UNITY_END();
}
