#include <PrayerMethodConfig.h>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_standard_method_parameters()
{
  const PrayerMethodConfig egypt = prayerMethodConfig(0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 19.5f, egypt.fajrAngle);
  TEST_ASSERT_TRUE(egypt.maghribInMinutes);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, egypt.maghribValue);
  TEST_ASSERT_FALSE(egypt.ishaInMinutes);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 17.5f, egypt.ishaValue);

  const PrayerMethodConfig makkah = prayerMethodConfigForMode(1, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.5f, makkah.fajrAngle);
  TEST_ASSERT_FALSE(makkah.ishaInMinutes);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 17.0f, makkah.ishaValue);

  const PrayerMethodConfig makkahFixed = prayerMethodConfigForMode(1, 1);
  TEST_ASSERT_TRUE(makkahFixed.ishaInMinutes);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, makkahFixed.ishaValue);

  const PrayerMethodConfig custom = prayerMethodConfig(2);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, custom.fajrAngle);
  TEST_ASSERT_FALSE(custom.ishaInMinutes);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 17.5f, custom.ishaValue);

  const PrayerMethodConfig mwl = prayerMethodConfig(3);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, mwl.fajrAngle);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 17.0f, mwl.ishaValue);

  const PrayerMethodConfig karachi = prayerMethodConfig(4);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, karachi.fajrAngle);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, karachi.ishaValue);
}

void test_makkah_mode_is_binary()
{
  TEST_ASSERT_TRUE(validMakkahIshaMode(0));
  TEST_ASSERT_TRUE(validMakkahIshaMode(1));
  TEST_ASSERT_FALSE(validMakkahIshaMode(2));
}

void test_invalid_method_uses_egypt_defaults()
{
  const PrayerMethodConfig fallback = prayerMethodConfig(255);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 19.5f, fallback.fajrAngle);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 17.5f, fallback.ishaValue);
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_standard_method_parameters);
  RUN_TEST(test_makkah_mode_is_binary);
  RUN_TEST(test_invalid_method_uses_egypt_defaults);
  return UNITY_END();
}
