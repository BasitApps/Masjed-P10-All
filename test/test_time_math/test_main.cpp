#include <TimeMath.h>
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_minute_delta_wraps_at_midnight()
{
  TEST_ASSERT_EQUAL_INT16(3, signedMinuteDelta(2, 1439));
  TEST_ASSERT_EQUAL_INT16(-3, signedMinuteDelta(1439, 2));
  TEST_ASSERT_EQUAL_INT16(0, signedMinuteDelta(0, 0));
}

void test_window_can_cross_midnight()
{
  TEST_ASSERT_TRUE(minuteInWindow(1438, 1437, 1450));
  TEST_ASSERT_TRUE(minuteInWindow(5, 1437, 1450));
  TEST_ASSERT_FALSE(minuteInWindow(20, 1437, 1450));
}

void test_forward_minute_distance_wraps_target()
{
  TEST_ASSERT_EQUAL_INT16(3, forwardMinuteDistance(2, 1439 + 6));
  TEST_ASSERT_EQUAL_INT16(2, forwardMinuteDistance(1439, 1));
  TEST_ASSERT_EQUAL_INT16(0, forwardMinuteDistance(120, 120));
}

void test_calendar_month_lengths()
{
  TEST_ASSERT_EQUAL_UINT8(29, daysInMonth(2028, 2));
  TEST_ASSERT_EQUAL_UINT8(28, daysInMonth(2100, 2));
  TEST_ASSERT_EQUAL_UINT8(30, daysInMonth(2026, 9));
}

void test_display_hour()
{
  for (uint8_t h=0;h<24;++h) {
    TEST_ASSERT_EQUAL_UINT8(h,displayHour(h,true));
    TEST_ASSERT_EQUAL_UINT8(h%12?h%12:12,displayHour(h,false));
  }
}
int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_display_hour);
  RUN_TEST(test_minute_delta_wraps_at_midnight);
  RUN_TEST(test_window_can_cross_midnight);
  RUN_TEST(test_forward_minute_distance_wraps_target);
  RUN_TEST(test_calendar_month_lengths);
  return UNITY_END();
}
