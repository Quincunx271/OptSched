#include "opt-sched/Scheduler/logger.h"

#include <sstream>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

using namespace llvm::opt_sched;

namespace {
class LoggerTest : public ::testing::Test {
protected:
  LoggerTest() : old{Logger::GetLogStream()} { Logger::SetLogStream(log); }

  ~LoggerTest() override { Logger::SetLogStream(old); }

  std::string getLog() const { return log.str(); }

private:
  std::ostream &old;
  std::ostringstream log;
};

TEST_F(LoggerTest, EventWorks) {
  Logger::Event("SomeEventID", "key", 42, "key2", "value2", "key3", true,
                "key4", 123ull, "key5", -123ll);
  EXPECT_THAT(
      getLog(),
      ::testing::MatchesRegex(
          R"(EVENT: \{"event_id": "SomeEventID", "key": 42, "key2": "value2", "key3": true, "key4": 123, "key5": -123, "time": [0-9]+\})"
          "\n"));
}

TEST_F(LoggerTest, EmptyEventIncludesOnlyTime) {
  Logger::Event("SomeEventID");
  EXPECT_THAT(getLog(),
              ::testing::MatchesRegex(
                  R"(EVENT: \{"event_id": "SomeEventID", "time": [0-9]+\})"
                  "\n"));
}

template <typename T, T V> struct value_t { static constexpr auto value = V; };

#define CX(...) (+value_t<decltype(__VA_ARGS__), (__VA_ARGS__)>::value)

TEST(Logger, fnv1a_works_for_some_basic_strings) {
  EXPECT_EQ(0xcbf29ce484222325ull, CX(Logger::detail::fnv1a("")));
  EXPECT_EQ(0x6ef05bd7cc857c54ull, CX(Logger::detail::fnv1a("Hello, World!")));
  EXPECT_EQ(0xaf639d4c8601817full, CX(Logger::detail::fnv1a(" ")));
  EXPECT_EQ(0x835de747d2297216ull, CX(Logger::detail::fnv1a("0s$mBsJ^2X")));
  EXPECT_EQ(
      0xa55cd0397ebd239dull,
      CX(Logger::detail::fnv1a(
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
          "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
          "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
          "aliquip ex ea commodo consequat. Duis aute irure dolor in "
          "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
          "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
          "culpa qui officia deserunt mollit anim id est laborum.")));
}
} // namespace
