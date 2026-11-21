// Host-side unit tests for firmware/fertilizer_system/logic.h
// Build and run:  c++ -std=c++17 -Wall -Wextra -I firmware/fertilizer_system tests/test_logic.cpp -o /tmp/test_logic && /tmp/test_logic
#include <cstdio>
#include <cmath>
#include "logic.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)
static bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

int main() {
  // median
  { int a[] = {5, 1, 3};            CHECK(logic::medianOf(a, 3) == 3); }
  { int a[] = {9, 1, 5, 3};         CHECK(logic::medianOf(a, 4) == 4); }          // (3+5)/2
  { int a[] = {7};                  CHECK(logic::medianOf(a, 1) == 7); }
  { int a[] = {4095, 0, 100, 100, 100}; CHECK(logic::medianOf(a, 5) == 100); }    // outliers rejected
  { int a[] = {3, 1, 2};            logic::medianOf(a, 3); CHECK(a[0] == 3 && a[1] == 1 && a[2] == 2); }  // input untouched
  CHECK(logic::medianOf(nullptr, 0) == 0);

  // level classification, boundaries inclusive on the lower side (matches 0-700, 701-1200, 1201+)
  CHECK(logic::classifyLevel(0, 700, 1200) == 0);
  CHECK(logic::classifyLevel(700, 700, 1200) == 0);
  CHECK(logic::classifyLevel(701, 700, 1200) == 1);
  CHECK(logic::classifyLevel(1200, 700, 1200) == 1);
  CHECK(logic::classifyLevel(1201, 700, 1200) == 2);
  CHECK(logic::classifyLevel(4095, 700, 1200) == 2);

  // clamp
  CHECK(near(logic::clampf(5, 0, 3), 3));
  CHECK(near(logic::clampf(-1, 0, 3), 0));
  CHECK(near(logic::clampf(2, 0, 3), 2));

  // max run time: 500 ml at 1 ml/s = 500 s + grace; at 40 ml/s = 12.5 s + grace
  CHECK(logic::maxRunMs(500, 1.0f, 5000) == 505000UL);
  CHECK(logic::maxRunMs(500, 40.0f, 5000) == 17500UL);

  // run-on estimate: EMA, never negative, capped
  CHECK(near(logic::updateRunOn(0, 8, 500, 20), 4));        // first observation halves in
  CHECK(near(logic::updateRunOn(4, 8, 500, 20), 6));
  CHECK(near(logic::updateRunOn(10, 100, 500, 20), 20));    // capped at maxMl
  CHECK(near(logic::updateRunOn(10, 100, 10, 20), 5));      // capped at half the request
  CHECK(near(logic::updateRunOn(0, -5, 500, 20), 0));       // never negative

  // early stop
  CHECK(!logic::reachedTarget(90, 5, 100));
  CHECK(logic::reachedTarget(95, 5, 100));
  CHECK(logic::reachedTarget(100, 0, 100));
  CHECK(!logic::reachedTarget(99.9f, 0, 100));

  // suggested limit
  CHECK(near(logic::suggestMinFlow(91.2f), 45.6f));

  if (failures) { std::printf("%d test(s) FAILED\n", failures); return 1; }
  std::printf("all logic tests passed\n");
  return 0;
}
