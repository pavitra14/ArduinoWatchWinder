#include <Arduino.h>
#include <unity.h>
#include "PresetSystem.h"
#include "IRRemoteMap.h"

void setUp() {}
void tearDown() {}

void test_known_buttons_have_presets() {
  const IRButton buttons[] = {
    IRButton::BTN_1, IRButton::BTN_2, IRButton::BTN_3, IRButton::BTN_4,
    IRButton::BTN_5, IRButton::BTN_6, IRButton::BTN_7, IRButton::BTN_8
  };
  uint8_t expectedId = 1;
  for (IRButton btn : buttons) {
    const PresetConfig* p = findPreset(btn);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT8(expectedId, p->id);
    ++expectedId;
  }
}

void test_findPresetById_valid_range() {
  for (uint8_t id = 1; id <= 8; ++id) {
    const PresetConfig* p = findPresetById(id);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT8(id, p->id);
  }
}

void test_unknown_button_returns_null() {
  TEST_ASSERT_NULL(findPreset(IRButton::BTN_UNKNOWN));
}

void test_ids_and_buttons_are_unique() {
  bool seenIds[9] = {};
  bool seenButtons[256] = {};
  const IRButton buttons[] = {
    IRButton::BTN_1, IRButton::BTN_2, IRButton::BTN_3, IRButton::BTN_4,
    IRButton::BTN_5, IRButton::BTN_6, IRButton::BTN_7, IRButton::BTN_8
  };

  for (IRButton btn : buttons) {
    const PresetConfig* p = findPreset(btn);
    TEST_ASSERT_NOT_NULL(p);

    TEST_ASSERT_FALSE_MESSAGE(seenIds[p->id], "Duplicate preset id detected");
    seenIds[p->id] = true;

    const uint8_t btnIdx = static_cast<uint8_t>(btn);
    TEST_ASSERT_FALSE_MESSAGE(seenButtons[btnIdx], "Duplicate button mapping detected");
    seenButtons[btnIdx] = true;
  }
}

void test_preset7_alternating_duty_cycle() {
  const PresetConfig* p = findPreset(IRButton::BTN_7);
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_EQUAL_UINT8(7, p->id);
  TEST_ASSERT_EQUAL(MotorSelection::BOTH, p->motors);
  TEST_ASSERT_EQUAL(StepperDir::CW, p->direction);
  TEST_ASSERT_EQUAL(StepperDir::CW, p->motor2Direction);
  TEST_ASSERT_TRUE(p->alternateDirection);
  TEST_ASSERT_EQUAL(StepperDir::CCW, p->altDirection);
  TEST_ASSERT_EQUAL(PresetMode::DUTY_CYCLE, p->mode);
  TEST_ASSERT_EQUAL_UINT32(10UL * 60UL * 1000UL, p->runMs);
  TEST_ASSERT_EQUAL_UINT32(10UL * 60UL * 1000UL, p->restMs);
  TEST_ASSERT_EQUAL_UINT8(128, p->brightness);
  TEST_ASSERT_EQUAL_UINT8(255, p->color.r);
  TEST_ASSERT_EQUAL_UINT8(255, p->color.g);
  TEST_ASSERT_EQUAL_UINT8(255, p->color.b);
}

void test_preset8_opposing_duty_cycle() {
  const PresetConfig* p = findPreset(IRButton::BTN_8);
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_EQUAL_UINT8(8, p->id);
  TEST_ASSERT_EQUAL(MotorSelection::BOTH, p->motors);
  TEST_ASSERT_EQUAL(StepperDir::CW, p->direction);
  TEST_ASSERT_EQUAL(StepperDir::CCW, p->motor2Direction);
  TEST_ASSERT_FALSE(p->alternateDirection);
  TEST_ASSERT_EQUAL(PresetMode::DUTY_CYCLE, p->mode);
  TEST_ASSERT_EQUAL_UINT32(10UL * 60UL * 1000UL, p->runMs);
  TEST_ASSERT_EQUAL_UINT32(10UL * 60UL * 1000UL, p->restMs);
  TEST_ASSERT_EQUAL_UINT8(128, p->brightness);
  TEST_ASSERT_EQUAL_UINT8(255, p->color.r);
  TEST_ASSERT_EQUAL_UINT8(255, p->color.g);
  TEST_ASSERT_EQUAL_UINT8(0, p->color.b);
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_known_buttons_have_presets);
  RUN_TEST(test_findPresetById_valid_range);
  RUN_TEST(test_unknown_button_returns_null);
  RUN_TEST(test_ids_and_buttons_are_unique);
  RUN_TEST(test_preset7_alternating_duty_cycle);
  RUN_TEST(test_preset8_opposing_duty_cycle);
  return UNITY_END();
}

void setup() {
  delay(2000); // give time for serial to attach
  runUnityTests();
}

void loop() {
  // not used in tests
}
