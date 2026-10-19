/**
 * test_protocol/test_main.cpp
 *
 * Tests unitarios para Protocol.h — verifica que todos los IDs CAN
 * están definidos, no hay colisiones, y los rangos por nodo son correctos.
 *
 * Framework: Unity (incluido en PlatformIO)
 * Ejecutar: pio test -e test_native
 */

#include <unity.h>
#include "../../shared/Protocol.h"

// =============================================================================
// HELPERS
// =============================================================================

static bool ids_overlap(uint32_t a, uint32_t b) {
    return a == b;
}

// =============================================================================
// TESTS: Rangos por nodo
// =============================================================================

void test_bcm_ids_in_range(void) {
    TEST_ASSERT_TRUE(CAN_BCM_HEARTBEAT   >= 0x100 && CAN_BCM_HEARTBEAT   <= 0x1FF);
    TEST_ASSERT_TRUE(CAN_BCM_ALARM_STATE >= 0x100 && CAN_BCM_ALARM_STATE <= 0x1FF);
    TEST_ASSERT_TRUE(CAN_BCM_LIGHTS_STATE>= 0x100 && CAN_BCM_LIGHTS_STATE<= 0x1FF);
    TEST_ASSERT_TRUE(CAN_BCM_FLAME_CONFIG>= 0x100 && CAN_BCM_FLAME_CONFIG<= 0x1FF);
    TEST_ASSERT_TRUE(CAN_BCM_FLAME_STATE >= 0x100 && CAN_BCM_FLAME_STATE <= 0x1FF);
}

void test_bcm_cmd_ids_in_range(void) {
    TEST_ASSERT_TRUE(CAN_CMD_BCM_ALARM  >= 0x180 && CAN_CMD_BCM_ALARM  <= 0x1FF);
    TEST_ASSERT_TRUE(CAN_CMD_BCM_LIGHTS >= 0x180 && CAN_CMD_BCM_LIGHTS <= 0x1FF);
    TEST_ASSERT_TRUE(CAN_CMD_BCM_FLAME  >= 0x180 && CAN_CMD_BCM_FLAME  <= 0x1FF);
}

void test_icm_ids_in_range(void) {
    TEST_ASSERT_TRUE(CAN_ICM_HEARTBEAT  >= 0x300 && CAN_ICM_HEARTBEAT  <= 0x3FF);
    TEST_ASSERT_TRUE(CAN_ICM_ENGINE_DATA>= 0x300 && CAN_ICM_ENGINE_DATA<= 0x3FF);
    TEST_ASSERT_TRUE(CAN_ICM_STATUS     >= 0x300 && CAN_ICM_STATUS     <= 0x3FF);
    TEST_ASSERT_TRUE(CAN_ICM_ADC_RAW    >= 0x300 && CAN_ICM_ADC_RAW    <= 0x3FF);
}

void test_uce_ids_in_range(void) {
    TEST_ASSERT_TRUE(CAN_UCE_HEARTBEAT >= 0x400 && CAN_UCE_HEARTBEAT <= 0x41F);
    TEST_ASSERT_TRUE(CAN_UCE_ENGINE    >= 0x400 && CAN_UCE_ENGINE    <= 0x41F);
    TEST_ASSERT_TRUE(CAN_UCE_TEMPS     >= 0x400 && CAN_UCE_TEMPS     <= 0x41F);
    TEST_ASSERT_TRUE(CAN_UCE_IGNITION  >= 0x400 && CAN_UCE_IGNITION  <= 0x41F);
    TEST_ASSERT_TRUE(CAN_UCE_INJECTION >= 0x400 && CAN_UCE_INJECTION <= 0x41F);
    TEST_ASSERT_TRUE(CAN_UCE_STATUS    >= 0x400 && CAN_UCE_STATUS    <= 0x41F);
}

void test_gateway_id_in_range(void) {
    TEST_ASSERT_TRUE(CAN_GW_HEARTBEAT >= 0x700 && CAN_GW_HEARTBEAT <= 0x7FE);
}

void test_broadcast_id_is_0x7FF(void) {
    TEST_ASSERT_EQUAL_HEX32(0x7FF, CAN_BROADCAST_RESET);
}

// =============================================================================
// TESTS: Sin colisiones entre IDs
// =============================================================================

void test_no_id_collisions(void) {
    uint32_t all_ids[] = {
        CAN_BCM_HEARTBEAT,
        CAN_BCM_ALARM_STATE,
        CAN_BCM_LIGHTS_STATE,
        CAN_BCM_FLAME_CONFIG,
        CAN_BCM_FLAME_STATE,
        CAN_CMD_BCM_ALARM,
        CAN_CMD_BCM_LIGHTS,
        CAN_CMD_BCM_FLAME,
        CAN_ICM_HEARTBEAT,
        CAN_ICM_ENGINE_DATA,
        CAN_ICM_STATUS,
        CAN_ICM_ADC_RAW,
        CAN_UCE_HEARTBEAT,
        CAN_UCE_ENGINE,
        CAN_UCE_TEMPS,
        CAN_UCE_IGNITION,
        CAN_UCE_INJECTION,
        CAN_UCE_STATUS,
        CAN_CMD_UCE_TUNE,
        CAN_CMD_UCE_MODE,
        CAN_GW_HEARTBEAT,
        CAN_BROADCAST_RESET,
    };

    const size_t n = sizeof(all_ids) / sizeof(all_ids[0]);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            TEST_ASSERT_FALSE_MESSAGE(
                ids_overlap(all_ids[i], all_ids[j]),
                "Colisión de IDs CAN detectada"
            );
        }
    }
}

// =============================================================================
// TESTS: Flags de status
// =============================================================================

void test_status_flags_no_overlap(void) {
    // Cada flag debe ser una potencia de 2 única
    TEST_ASSERT_EQUAL_HEX8(0x01, STATUS_OK);
    TEST_ASSERT_EQUAL_HEX8(0x02, STATUS_ERROR);
    TEST_ASSERT_EQUAL_HEX8(0x04, STATUS_INIT);
    TEST_ASSERT_EQUAL_HEX8(0x08, STATUS_CALIBRATING);
    // Sin solapamiento
    TEST_ASSERT_EQUAL(0, STATUS_OK & STATUS_ERROR);
    TEST_ASSERT_EQUAL(0, STATUS_OK & STATUS_INIT);
    TEST_ASSERT_EQUAL(0, STATUS_ERROR & STATUS_CALIBRATING);
}

void test_light_flags_no_overlap(void) {
    TEST_ASSERT_EQUAL_HEX8(0x01, LIGHT_POSITION);
    TEST_ASSERT_EQUAL_HEX8(0x02, LIGHT_LOW_BEAM);
    TEST_ASSERT_EQUAL_HEX8(0x04, LIGHT_HIGH_BEAM);
    TEST_ASSERT_EQUAL_HEX8(0x08, LIGHT_FOG_FRONT);
    TEST_ASSERT_EQUAL_HEX8(0x10, LIGHT_FOG_REAR);
    // Todos caben en un byte
    uint8_t all = LIGHT_POSITION | LIGHT_LOW_BEAM | LIGHT_HIGH_BEAM |
                  LIGHT_FOG_FRONT | LIGHT_FOG_REAR;
    TEST_ASSERT_EQUAL_HEX8(0x1F, all);
}

// =============================================================================
// TESTS: IDs dentro del rango CAN estándar (11 bits, 0x000-0x7FF)
// =============================================================================

void test_all_ids_fit_11bits(void) {
    uint32_t all_ids[] = {
        CAN_BCM_HEARTBEAT, CAN_BCM_ALARM_STATE, CAN_BCM_LIGHTS_STATE,
        CAN_BCM_FLAME_CONFIG, CAN_BCM_FLAME_STATE, CAN_CMD_BCM_ALARM,
        CAN_CMD_BCM_LIGHTS, CAN_CMD_BCM_FLAME, CAN_ICM_HEARTBEAT,
        CAN_ICM_ENGINE_DATA, CAN_ICM_STATUS, CAN_ICM_ADC_RAW,
        CAN_UCE_HEARTBEAT, CAN_UCE_ENGINE, CAN_UCE_TEMPS, CAN_UCE_IGNITION,
        CAN_UCE_INJECTION, CAN_UCE_STATUS, CAN_CMD_UCE_TUNE, CAN_CMD_UCE_MODE,
        CAN_GW_HEARTBEAT, CAN_BROADCAST_RESET,
    };
    const size_t n = sizeof(all_ids) / sizeof(all_ids[0]);
    for (size_t i = 0; i < n; i++) {
        TEST_ASSERT_TRUE_MESSAGE(all_ids[i] <= 0x7FF, "ID fuera de rango CAN estándar");
    }
}

// =============================================================================
// MAIN
// =============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bcm_ids_in_range);
    RUN_TEST(test_bcm_cmd_ids_in_range);
    RUN_TEST(test_icm_ids_in_range);
    RUN_TEST(test_uce_ids_in_range);
    RUN_TEST(test_gateway_id_in_range);
    RUN_TEST(test_broadcast_id_is_0x7FF);
    RUN_TEST(test_no_id_collisions);
    RUN_TEST(test_status_flags_no_overlap);
    RUN_TEST(test_light_flags_no_overlap);
    RUN_TEST(test_all_ids_fit_11bits);

    return UNITY_END();
}
