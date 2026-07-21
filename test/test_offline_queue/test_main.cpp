#include <unity.h>

#include <cstdint>
#include <cstring>
#include "storage/OfflineQueuePolicy.h"

void setUp() {}
void tearDown() {}

void test_replay_tokens_are_distinct_from_live_tokens() {
  const uint32_t replay = OfflineQueuePolicy::replayToken(42U);
  const uint32_t live = OfflineQueuePolicy::liveToken(42U);
  TEST_ASSERT_TRUE(OfflineQueuePolicy::isReplayToken(replay));
  TEST_ASSERT_FALSE(OfflineQueuePolicy::isReplayToken(live));
  TEST_ASSERT_EQUAL_UINT32(42U,
                           OfflineQueuePolicy::recordIdFromToken(replay));
}

void test_replay_flag_covers_deferred_retry_and_reboot() {
  TEST_ASSERT_FALSE(OfflineQueuePolicy::shouldMarkReplayed(false, 0U, 7U,
                                                           7U));
  TEST_ASSERT_TRUE(OfflineQueuePolicy::shouldMarkReplayed(true, 0U, 7U,
                                                          7U));
  TEST_ASSERT_TRUE(OfflineQueuePolicy::shouldMarkReplayed(false, 1U, 7U,
                                                          7U));
  TEST_ASSERT_TRUE(OfflineQueuePolicy::shouldMarkReplayed(false, 0U, 6U,
                                                          7U));
}

void test_record_and_byte_limits_are_bounded() {
  TEST_ASSERT_FALSE(
      OfflineQueuePolicy::exceedsLimits(2U, 200U, 100U, 3U, 300U));
  TEST_ASSERT_TRUE(
      OfflineQueuePolicy::exceedsLimits(3U, 200U, 1U, 3U, 300U));
  TEST_ASSERT_TRUE(
      OfflineQueuePolicy::exceedsLimits(2U, 250U, 51U, 3U, 300U));
  TEST_ASSERT_TRUE(
      OfflineQueuePolicy::exceedsLimits(0U, 0U, 301U, 3U, 300U));
}

void test_age_requires_two_valid_ordered_epochs() {
  TEST_ASSERT_EQUAL_UINT32(0U, OfflineQueuePolicy::ageSeconds(0U, 200U));
  TEST_ASSERT_EQUAL_UINT32(0U, OfflineQueuePolicy::ageSeconds(200U, 0U));
  TEST_ASSERT_EQUAL_UINT32(0U, OfflineQueuePolicy::ageSeconds(200U, 199U));
  TEST_ASSERT_EQUAL_UINT32(50U,
                           OfflineQueuePolicy::ageSeconds(150U, 200U));
}

void test_replay_marker_changes_only_the_delivery_copy() {
  char payload[96] =
      "{\"sample_id\":\"d:1:1\",\"replayed\":false,\"simulated\":false}";
  size_t length = std::strlen(payload);
  TEST_ASSERT_TRUE(
      OfflineQueuePolicy::markPayloadReplayed(payload, length));
  TEST_ASSERT_EQUAL_STRING(
      "{\"sample_id\":\"d:1:1\",\"replayed\":true,\"simulated\":false}",
      payload);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(std::strlen(payload)),
                           static_cast<uint32_t>(length));

  char invalid[32] = "{\"replayed\":true}";
  size_t invalidLength = std::strlen(invalid);
  TEST_ASSERT_FALSE(
      OfflineQueuePolicy::markPayloadReplayed(invalid, invalidLength));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_replay_tokens_are_distinct_from_live_tokens);
  RUN_TEST(test_replay_flag_covers_deferred_retry_and_reboot);
  RUN_TEST(test_record_and_byte_limits_are_bounded);
  RUN_TEST(test_age_requires_two_valid_ordered_epochs);
  RUN_TEST(test_replay_marker_changes_only_the_delivery_copy);
  return UNITY_END();
}
