# Measured Performance

Results from the final-year project evaluation of the prototype. **Source:** *FYP 2 Final Report* (Chapter 4), the build described in the [published paper](https://penerbit.uthm.edu.my/ojs/index.php/ijie/article/view/24386).

> **Scope.** These numbers were measured on the **paper prototype firmware** (flow factors 4.25 and 5.25 ml/pulse, which is now the `standard` profile, TDS factor 2.3, Blynk only, no safety layer). The current firmware (1.3.0) has **not been re-measured**. Each test used 3 repeats per point.

## TDS sensor accuracy

Concentrated AB solution (about 900 ppm) diluted in steps; each point read three times and compared with a calibrated TDS meter.

| Solution | Sensor average (ppm) | Reference meter (ppm) | Deviation (ppm) | Accuracy |
|---|---|---|---|---|
| Concentrated | 896.00 | 898 | 4.00 | 99.56% |
| Diluted 1 | 666.67 | 666 | 0.67 | 99.90% |
| Diluted 2 | 458.33 | 458 | 0.33 | 99.93% |
| Diluted 3 | 232.67 | 235 | 2.37 | 99.01% |

![TDS sensor against calibrated meter](images/results-tds-accuracy.svg)

Each point is one solution; the dashed line is perfect agreement. Sensor rating: 0-1000 ppm, ±10% full scale at 25 °C. All tests are inside that range.

## TDS calibration stability

Same standard solution (natural mineral water, 258 ppm) read once a day for 7 days after one calibration.

| Day | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| Reading (ppm) | 257 | 253 | 255 | 260 | 256 | 258 | 256 |

![TDS sensor stability over seven days](images/results-tds-stability.svg)

Maximum deviation from the calibration value: **5 ppm** (day 2). Temperature was assumed constant (23 °C) throughout.

## Flow sensor accuracy

Volumes were dispensed into a measuring container; accuracy is `1 − deviation / target`.

**Flow sensor 1 (4.25 ml/pulse)**

| Target (ml) | Average (ml) | Deviation (ml) | Accuracy |
|---|---|---|---|
| 250 | 235.0 | 15.0 | 94.00% |
| 500 | 478.3 | 21.7 | 95.66% |
| 750 | 746.6 | 3.4 | 99.54% |
| 1000 | 995.0 | 5.0 | 99.50% |

**Flow sensor 2 (5.25 ml/pulse)**

| Target (ml) | Average (ml) | Deviation (ml) | Accuracy |
|---|---|---|---|
| 250 | 236.7 | 13.3 | 94.68% |
| 500 | 501.7 | 1.7 | 99.66% |
| 750 | 756.7 | 6.7 | 99.10% |
| 1000 | 1018.3 | 18.3 | 98.17% |

![Flow sensor deviation by target volume](images/results-flow-deviation.svg)

**Observation.** Relative error is largest at the smallest volume (about 94% at 250 ml) and the deviation is a few to ~20 ml throughout. This is consistent with the granularity of one flow pulse (4-5 ml) plus pump run-on after switch-off, and was the motivation for the adaptive run-on compensation added in firmware 1.5.0 ([architecture.md](architecture.md)); its effect has not yet been re-measured. Prefer volumes of 500 ml or more when accuracy matters, or calibrate with a long run ([calibration.md](calibration.md)).

## Concentration adjustment

Increasing amounts of concentrated AB solution added to a baseline, with the TDS sensor read against the calibrated meter.

| Initial (ppm) | Target (ppm) | AB added (ml) | Sensor (ppm) | Meter (ppm) | Sensor deviation (ppm) | Target achieved |
|---|---|---|---|---|---|---|
| 103 | 200 | 200 | 187 | 192 | 13 | 96.00% |
| 192 | 400 | 300 | 392 | 395 | 8 | 98.75% |
| 395 | 600 | 400 | 559 | 557 | 1 | 92.83% |
| 557 | 800 | 500 | 769 | 764 | 31 | 95.50% |

![Fertiliser concentration adjustment](images/results-target-achieved.svg)

The ppm rises close to linearly with the volume of concentrate added; the maximum sensor-to-meter deviation was 31 ppm.

## System response time

Timed with a stopwatch from the Blynk command, 3 trials each, dispensing 1 litre.

| Action | Average (s) |
|---|---|
| Solenoid valves open | 10.85 |
| Pumps activation | 1.07 |
| Fertiliser dispense | 10.79 |
| **Total** | **22.17** |

![System response time](images/results-response-time.svg)

The report's *solenoid valve* time includes the app-to-device path and the start of flow, not just the valve's own ≤ 0.15 s mechanical opening. Treat it as an end-to-end figure.

*The charts are generated from these tables by [`tools/make_charts.py`](../tools/make_charts.py) (plain SVG, follows your light or dark theme).*

## Limitations of these results

- 3 repeats per point, no confidence intervals.
- Sensor accuracy limited to the TDS probe's 0-1000 ppm range.
- Tap water stood in for fertiliser in the flow tests; real concentrate has a different viscosity.
- Firmware has changed since (see [CHANGELOG](../CHANGELOG.md)); re-run the tests, and the [bench tests](bench-tests.md), on 1.3.0 before quoting these numbers for it.
