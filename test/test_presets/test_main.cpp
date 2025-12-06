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

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_known_buttons_have_presets);
  RUN_TEST(test_findPresetById_valid_range);
  RUN_TEST(test_unknown_button_returns_null);
  RUN_TEST(test_ids_and_buttons_are_unique);
  return UNITY_END();
}

void setup() {
  delay(2000); // give time for serial to attach
  runUnityTests();
}

void loop() {
  // not used in tests
}
