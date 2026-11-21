/*
 * IoT-Based Fertiliser Mixing System for Precision Farming — firmware v1.5.0
 * Sri Tulasi Ram Rajalingam, Muhammad Muzakkir Mohd Nadzri, Afandi Ahmad, Mohamad Khairi Ishak
 * IJIE 18(1), 186-201, 2026 — DOI 10.30880/ijie.2026.18.01.014, https://journal.uthm.edu.my/index.php/ijie/article/view/24386
 * MIT License.
 *
 * Arduino IDE: open this folder as a sketch. Copy config.h.example to config.h first.
 * Setup details: ../README.md   Safety requirements: ../../docs/safety.md
 */
#include "settings.h"
#include "dispenser.h"
#include "level.h"
#include "tds.h"
#include "net.h"
#include "calibration.h"
#include "safety_limits.h"
#include "temperature.h"
#include "serial_console.h"
#include <esp_task_wdt.h>

static void startWatchdog() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms     = WDT_TIMEOUT_S * 1000;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic  = true;
  if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) esp_task_wdt_init(&cfg);
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);   // watch this (loop) task: a stall resets the chip, outputs go LOW
}

void setup() {
  dispenserSafeOutputs();   // SAF-06: outputs OFF before anything else
  Serial.begin(115200);
  Serial.printf("\nFertiliser mixing firmware %s\n", FW_VERSION);

  startWatchdog();          // SAF-07
  calBegin();               // stored calibration (flash) or factory defaults
  limitsBegin();            // stored safety limits (flash) or defaults
  dispenserBegin();
  levelBegin();
  tdsBegin();
  tempBegin();              // no-op unless ENABLE_DS18B20
  consoleBegin();
  netBegin();               // non-blocking: system works with no network (SAF-10)

  Serial.println("System ready.");
}

void loop() {
  esp_task_wdt_reset();
  tdsSample();
  tempLoop();
  consoleLoop();            // serial commands (type "help")
  dispenserLoop();          // safety first: runs before any network work
  netLoop();
}
