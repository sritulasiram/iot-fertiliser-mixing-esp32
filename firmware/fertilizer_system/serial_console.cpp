#include "serial_console.h"
#include "settings.h"
#include "dispenser.h"
#include "calibration.h"
#include "level.h"
#include "safety_limits.h"
#include "tds.h"
#include "temperature.h"
#include "net.h"

static void printHelp() {
  Serial.println(
    "Commands:\n"
    "  status                      state, sensors, calibration\n"
    "  dispense <ml> [1|2]         dispense ml per channel (both channels if omitted)\n"
    "  estop                       emergency stop (latches a fault)\n"
    "  ack                         acknowledge a fault\n"
    "  cal run <1|2> [seconds]     run one pump for a fixed time to count flow pulses\n"
    "  cal set <1|2> <measured_ml> set ml/pulse from the volume you collected\n"
    "  cal tds <reference_ppm>     calibrate TDS against a reference solution\n"
    "  cal profile <standard|high|custom>  choose flow factors (4.25/5.25, 5.25/5.50, or your own)\n"
    "  cal show                    show calibration values\n"
    "  limits                      show safety limits; 'limits reset' restores defaults\n"
    "  set <key> <value>           change a limit (saved): max_dispense_ml, min_flow, no_flow_ms,\n"
    "                              level_low, level_med   (serial only, never remote)\n"
    "  cal reset                   restore factory calibration\n"
    "  help");
}

static void printResult(CmdResult r) {
  Serial.printf("-> %s\n", resultName(r));
}

static void showCal() {
  Serial.printf("Flow profile %s: ch1 %.4f ml/pulse, ch2 %.4f ml/pulse. TDS factor %.3f%s\n",
                calProfileName(calProfile()), dispenserFlowFactor(1), dispenserFlowFactor(2), calTds(),
                calTds() != TDS_CALIB_DEFAULT ? " (custom)" : "");
}

static void showStatus() {
  Serial.printf("State %s, fault %s\n", stateName(dispenserState()), faultName(dispenserFault()));
  Serial.printf("TDS %.0f ppm, water temp %.1f C (%s)\n", tdsPpm(), tempC(), tempValid() ? "live" : "fixed");
  Serial.printf("Tank 1 %s (%d), tank 2 %s (%d)\n",
                levelName(levelOf(0)), levelRaw(0), levelName(levelOf(1)), levelRaw(1));
  Serial.printf("Device %s, firmware %s\n", netDeviceId(), FW_VERSION);
  showCal();
  limitsPrint();
  Serial.printf("Run-on compensation: ch1 %.1f ml, ch2 %.1f ml\n", dispenserRunOnMl(1), dispenserRunOnMl(2));
}

static void handle(char* line) {
  char* save = nullptr;
  char* cmd = strtok_r(line, " \t", &save);
  if (!cmd) return;
  char* a1 = strtok_r(nullptr, " \t", &save);
  char* a2 = strtok_r(nullptr, " \t", &save);
  char* a3 = strtok_r(nullptr, " \t", &save);

  if (!strcasecmp(cmd, "help") || !strcmp(cmd, "?")) {
    printHelp();
  } else if (!strcasecmp(cmd, "status")) {
    showStatus();
  } else if (!strcasecmp(cmd, "dispense")) {
    long ml = a1 ? atol(a1) : 0;
    uint8_t mask = 0x03;
    if (a2) { int c = atoi(a2); mask = (c == 1) ? 0x01 : (c == 2) ? 0x02 : 0x00; }
    printResult(ml <= 0 ? CmdResult::BAD_VOLUME : dispenserRequest((unsigned long)ml, mask));
  } else if (!strcasecmp(cmd, "estop")) {
    dispenserEstop();
  } else if (!strcasecmp(cmd, "ack")) {
    printResult(dispenserAckFault());
  } else if (!strcasecmp(cmd, "limits")) {
    if (a1 && !strcasecmp(a1, "reset")) { limitsReset(); Serial.println("Limits reset to defaults."); }
    limitsPrint();
  } else if (!strcasecmp(cmd, "set")) {
    if (!a1 || !a2) { Serial.println("usage: set <key> <value>   (see 'limits')"); }
    else {
      const char* err = limitsSet(a1, atof(a2));
      if (err) Serial.printf("-> rejected: %s\n", err);
      else { Serial.println("-> OK (saved)"); limitsPrint(); }
    }
  } else if (!strcasecmp(cmd, "cal")) {
    if (a1 && !strcasecmp(a1, "run")) {
      int c = a2 ? atoi(a2) : 0;
      float secs = a3 ? atof(a3) : 0.0f;
      printResult(dispenserCalRun((uint8_t)c, (unsigned long)(secs * 1000.0f)));
    } else if (a1 && !strcasecmp(a1, "set")) {
      int c = a2 ? atoi(a2) : 0;
      float ml = a3 ? atof(a3) : 0.0f;
      float f = 0;
      CmdResult r = dispenserCalSetFlow((uint8_t)c, ml, &f);
      printResult(r);
      if (r == CmdResult::NO_CAL_RUN) Serial.println("Run 'cal run <channel>' first and collect the output.");
    } else if (a1 && !strcasecmp(a1, "tds")) {
      float ref = a2 ? atof(a2) : 0.0f;
      float f = 0;
      if (tdsCalibrate(ref, &f)) Serial.printf("-> OK, TDS factor %.3f (saved)\n", f);
      else Serial.println("-> BAD_VALUE (probe in the reference solution? reading must be above 10 ppm)");
    } else if (a1 && !strcasecmp(a1, "profile")) {
      CalProfile p;
      if (!calParseProfile(a2, &p)) Serial.println("-> usage: cal profile <standard|high|custom>");
      else {
        CmdResult r = dispenserSetCalProfile(p);
        printResult(r);
        if (r == CmdResult::NO_CUSTOM) Serial.println("No custom calibration saved yet: run 'cal run' and 'cal set' first.");
      }
    } else if (a1 && !strcasecmp(a1, "show")) {
      showCal();
    } else if (a1 && !strcasecmp(a1, "reset")) {
      dispenserCalReset();
    } else {
      printHelp();
    }
  } else {
    Serial.printf("Unknown command '%s'. Type 'help'.\n", cmd);
  }
}

void consoleBegin() {
  Serial.println("Serial console ready. Type 'help'.");
}

void consoleLoop() {
  static char buf[64];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (len) { buf[len] = '\0'; len = 0; handle(buf); }
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
}
