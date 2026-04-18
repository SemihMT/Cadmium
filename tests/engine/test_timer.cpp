#include <catch2/catch_all.hpp>
#include <cadmium/core/engine.hpp>
#include <thread>

TEST_CASE("Timer - DeltaTimeClamped never exceeds maxDelta", "[timer]")
{
  Cadmium::Timer timer;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  float dt = timer.DeltaTimeClamped(0.05f);
  REQUIRE(dt <= 0.05f);
}

TEST_CASE("Timer - DeltaTime is positive", "[timer]")
{
  Cadmium::Timer timer;
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  float dt = timer.DeltaTime();
  REQUIRE(dt > 0.0f);
}
