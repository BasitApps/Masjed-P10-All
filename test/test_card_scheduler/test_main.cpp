#include "../../src/CardScheduler.h"
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_valid_order_accepts_permutation()
{
  const uint8_t order[CardScheduler::CARD_COUNT] =
      {3, 0, 11, 1, 2, 4, 5, 6, 7, 8, 9, 10};
  TEST_ASSERT_TRUE(CardScheduler::validOrder(order));
}

void test_invalid_order_rejects_duplicate_and_out_of_range()
{
  uint8_t order[CardScheduler::CARD_COUNT] =
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10};
  TEST_ASSERT_FALSE(CardScheduler::validOrder(order));
  order[11] = CardScheduler::CARD_COUNT;
  TEST_ASSERT_FALSE(CardScheduler::validOrder(order));
}

void test_next_skips_disabled_and_wraps()
{
  const uint8_t order[CardScheduler::CARD_COUNT] =
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  const uint16_t enabled = (1u << 2) | (1u << 7);
  TEST_ASSERT_EQUAL_UINT8(7, CardScheduler::nextIndex(order, enabled, 2));
  TEST_ASSERT_EQUAL_UINT8(2, CardScheduler::nextIndex(order, enabled, 7));
}

void test_all_disabled_falls_back_to_first_default_card()
{
  const uint8_t order[CardScheduler::CARD_COUNT] =
      {8, 4, 1, 0, 2, 3, 5, 6, 7, 9, 10, 11};
  TEST_ASSERT_EQUAL_UINT8(0, CardScheduler::nextIndex(order, 0, 8));
}

void test_small_font_info_cards_move_to_bottom_on_narrow_panels()
{
  TEST_ASSERT_TRUE(CardScheduler::smallFontBottomInfo(2, 64)); // day name
  TEST_ASSERT_TRUE(CardScheduler::smallFontBottomInfo(3, 32)); // Gregorian date
  TEST_ASSERT_TRUE(CardScheduler::smallFontBottomInfo(4, 64)); // Hijri date
  TEST_ASSERT_FALSE(CardScheduler::smallFontBottomInfo(2, 128));
  TEST_ASSERT_FALSE(CardScheduler::smallFontBottomInfo(7, 64)); // clock stays on top
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_valid_order_accepts_permutation);
  RUN_TEST(test_invalid_order_rejects_duplicate_and_out_of_range);
  RUN_TEST(test_next_skips_disabled_and_wraps);
  RUN_TEST(test_all_disabled_falls_back_to_first_default_card);
  RUN_TEST(test_small_font_info_cards_move_to_bottom_on_narrow_panels);
  return UNITY_END();
}
