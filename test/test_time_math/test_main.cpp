#include <TimeMath.h>
#include <PrayerIndex.h>
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

void test_calendar_date_key_includes_month_and_year()
{
  TEST_ASSERT_TRUE(sameCalendarDate(2026, 9, 30, 2026, 9, 30));
  TEST_ASSERT_FALSE(sameCalendarDate(2026, 10, 30, 2026, 9, 30));
  TEST_ASSERT_FALSE(sameCalendarDate(2027, 9, 30, 2026, 9, 30));
}

void test_consume_second_ticks_catches_up_without_jumping_forever()
{
  uint32_t last = 1000;
  TEST_ASSERT_EQUAL_UINT8(0, consumeSecondTicks(1999, last));
  TEST_ASSERT_EQUAL_UINT32(1000, last);
  TEST_ASSERT_EQUAL_UINT8(3, consumeSecondTicks(4000, last));
  TEST_ASSERT_EQUAL_UINT32(4000, last);
  TEST_ASSERT_EQUAL_UINT8(5, consumeSecondTicks(20000, last, 5));
  TEST_ASSERT_EQUAL_UINT32(9000, last);
}

void test_prayer_phase_boundaries()
{
  TEST_ASSERT_EQUAL(PrayerPhase::PreAzan, prayerPhase(-3, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Azan, prayerPhase(0, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Azan, prayerPhase(1, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::EqamaCountdown, prayerPhase(2, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::EqamaSeconds, prayerPhase(9, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Eqama, prayerPhase(10, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Sleep, prayerPhase(12, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Azkar, prayerPhase(15, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Outside, prayerPhase(16, 10, 5));
  TEST_ASSERT_EQUAL(PrayerPhase::Azan, prayerPhase(1, 1, 5));
}

void test_prayer_indices_keep_sunset_separate_from_maghrib()
{
  TEST_ASSERT_EQUAL_UINT8(4, PrayerIndex::Sunset);
  TEST_ASSERT_EQUAL_UINT8(5, PrayerIndex::Maghrib);
  TEST_ASSERT_EQUAL_UINT8(5, PrayerIndex::EventIndices[4]);
}
int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_display_hour);
  RUN_TEST(test_minute_delta_wraps_at_midnight);
  RUN_TEST(test_window_can_cross_midnight);
  RUN_TEST(test_forward_minute_distance_wraps_target);
  RUN_TEST(test_calendar_month_lengths);
  RUN_TEST(test_calendar_date_key_includes_month_and_year);
  RUN_TEST(test_consume_second_ticks_catches_up_without_jumping_forever);
  RUN_TEST(test_prayer_phase_boundaries);
  RUN_TEST(test_prayer_indices_keep_sunset_separate_from_maghrib);
  return UNITY_END();
}
