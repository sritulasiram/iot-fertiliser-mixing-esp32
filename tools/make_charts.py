#!/usr/bin/env python3
"""Generate the result charts in docs/images/ as plain SVG (no dependencies).

Data are the measured values from the FYP report (Chapter 4), see docs/results.md.
Colours are the reference palette's categorical slots 1 (blue) and 2 (orange) plus its neutrals;
the SVGs follow the viewer's light/dark setting. Run:  python3 tools/make_charts.py
"""
import os

OUT = os.path.join(os.path.dirname(__file__), '..', 'docs', 'images')
W, H = 640, 380
ML, MR, MT, MB = 64, 24, 24, 56          # plot margins

STYLE = """
<style>
  :root { --surface:#fcfcfb; --ink:#0b0b0b; --ink2:#52514e; --muted:#898781; --grid:#e1e0d9; --axis:#c3c2b7;
          --s1:#2a78d6; --s2:#eb6834; }
  @media (prefers-color-scheme: dark) {
    :root { --surface:#1a1a19; --ink:#f0efec; --ink2:#c3c2b7; --muted:#898781; --grid:#2c2c2a; --axis:#383835;
            --s1:#3987e5; --s2:#d95926; }
  }
  text { font-family: system-ui, -apple-system, 'Segoe UI', Roboto, sans-serif; }
  .t   { fill: var(--ink2); font-size: 12px; }
  .tm  { fill: var(--muted); font-size: 12px; }
  .ti  { fill: var(--ink);  font-size: 13px; }
  .val { fill: var(--ink);  font-size: 12px; font-weight: 600; }
  .grid{ stroke: var(--grid); stroke-width: 1; }
  .axis{ stroke: var(--axis); stroke-width: 1; }
  .ref { stroke: var(--muted); stroke-width: 1.5; stroke-dasharray: 5 4; fill: none; }
  .l1  { stroke: var(--s1); stroke-width: 2; fill: none; }
  .f1  { fill: var(--s1); }
  .f2  { fill: var(--s2); }
  .dot1{ fill: var(--s1); stroke: var(--surface); stroke-width: 2; }
</style>"""

def esc(s): return s.replace('&', '&amp;').replace('<', '&lt;')

def svg(title, desc, body, h=H):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {h}" width="{W}" height="{h}" '
            f'role="img" aria-labelledby="t d">\n<title id="t">{esc(title)}</title><desc id="d">{esc(desc)}</desc>{STYLE}\n'
            f'<rect width="{W}" height="{h}" fill="var(--surface)"/>\n{body}\n</svg>\n')

class Plot:
    def __init__(self, x0, x1, y0, y1, right=MR):
        self.x0, self.x1, self.y0, self.y1 = x0, x1, y0, y1
        self.l, self.r, self.t, self.b = ML, W - right, MT, H - MB
    def X(self, v): return self.l + (v - self.x0) / (self.x1 - self.x0) * (self.r - self.l)
    def Y(self, v): return self.b - (v - self.y0) / (self.y1 - self.y0) * (self.b - self.t)

def yaxis(p, ticks, label, fmt='{:g}'):
    out = []
    for v in ticks:
        y = p.Y(v)
        out.append(f'<line class="grid" x1="{p.l}" x2="{p.r}" y1="{y:.1f}" y2="{y:.1f}"/>')
        out.append(f'<text class="tm" x="{p.l-8}" y="{y+4:.1f}" text-anchor="end">{fmt.format(v)}</text>')
    out.append(f'<line class="axis" x1="{p.l}" x2="{p.r}" y1="{p.b}" y2="{p.b}"/>')
    out.append(f'<text class="t" x="16" y="{(p.t+p.b)/2:.0f}" text-anchor="middle" transform="rotate(-90 16 {(p.t+p.b)/2:.0f})">{esc(label)}</text>')
    return '\n'.join(out)

def xlabel(p, text): return f'<text class="t" x="{(p.l+p.r)/2:.0f}" y="{H-12}" text-anchor="middle">{esc(text)}</text>'

def bar_path(x, y, w, base, r=4):
    """Bar rising from `base` to `y` with the data-end rounded, baseline end square."""
    h = base - y
    if h <= 0: return ''
    r = min(r, h, w / 2)
    return (f'M{x:.1f},{base:.1f} L{x:.1f},{y+r:.1f} Q{x:.1f},{y:.1f} {x+r:.1f},{y:.1f} '
            f'L{x+w-r:.1f},{y:.1f} Q{x+w:.1f},{y:.1f} {x+w:.1f},{y+r:.1f} L{x+w:.1f},{base:.1f} Z')

# 1 ─ TDS accuracy: sensor vs calibrated meter (agreement with the 1:1 line) ────────────────
def chart_tds_accuracy():
    pts = [('Concentrated', 898, 896.00), ('Diluted 1', 666, 666.67), ('Diluted 2', 458, 458.33), ('Diluted 3', 235, 232.67)]
    p = Plot(0, 1000, 0, 1000, right=150); b = [yaxis(p, range(0, 1001, 250), 'TDS sensor reading (ppm)')]
    for v in range(0, 1001, 250):
        b.append(f'<text class="tm" x="{p.X(v):.1f}" y="{p.b+18}" text-anchor="middle">{v}</text>')
    b.append(xlabel(p, 'Calibrated TDS meter (ppm)'))
    b.append(f'<line class="ref" x1="{p.X(0)}" y1="{p.Y(0)}" x2="{p.X(1000)}" y2="{p.Y(1000)}"/>')
    b.append(f'<text class="tm" x="{p.X(520):.0f}" y="{p.Y(150):.0f}">dashed line = perfect agreement</text>')
    for name, meter, sensor in pts:
        x, y = p.X(meter), p.Y(sensor)
        b.append(f'<circle class="dot1" cx="{x:.1f}" cy="{y:.1f}" r="5"><title>{name}: meter {meter} ppm, sensor {sensor:.1f} ppm</title></circle>')
        b.append(f'<text class="t" x="{x+10:.1f}" y="{y+4:.1f}">{name} ({sensor:.0f})</text>')
    return svg('TDS sensor against calibrated meter',
               'Four solutions from 235 to 898 ppm; the sensor readings sit on the line of perfect agreement, within 4 ppm.', '\n'.join(b))

# 2 ─ TDS stability over 7 days ─────────────────────────────────────────────────────────────
def chart_tds_stability():
    days = [1, 2, 3, 4, 5, 6, 7]; ppm = [257, 253, 255, 260, 256, 258, 256]
    p = Plot(0.6, 7.4, 245, 265); b = [yaxis(p, range(245, 266, 5), 'TDS reading (ppm)')]
    for d in days: b.append(f'<text class="tm" x="{p.X(d):.1f}" y="{p.b+18}" text-anchor="middle">{d}</text>')
    b.append(xlabel(p, 'Day'))
    b.append(f'<line class="ref" x1="{p.l}" x2="{p.r}" y1="{p.Y(258):.1f}" y2="{p.Y(258):.1f}"/>')
    b.append(f'<text class="tm" x="{p.l+6}" y="{p.Y(258)-6:.1f}">calibration value 258 ppm</text>')
    b.append('<polyline class="l1" points="' + ' '.join(f'{p.X(d):.1f},{p.Y(v):.1f}' for d, v in zip(days, ppm)) + '"/>')
    for d, v in zip(days, ppm):
        b.append(f'<circle class="dot1" cx="{p.X(d):.1f}" cy="{p.Y(v):.1f}" r="5"><title>Day {d}: {v} ppm (deviation {abs(v-258)})</title></circle>')
    b.append(f'<text class="val" x="{p.X(2):.1f}" y="{p.Y(253)+20:.1f}" text-anchor="middle">253 (max deviation 5)</text>')
    return svg('TDS sensor stability over seven days',
               'Daily readings of a 258 ppm reference solution stay within 5 ppm of the calibration value.', '\n'.join(b))

# 3 ─ Flow sensor deviation by target volume (grouped bars from zero) ───────────────────────
def chart_flow_deviation():
    targets = [250, 500, 750, 1000]; s1 = [15.0, 21.7, 3.4, 5.0]; s2 = [13.3, 1.7, 6.7, 18.3]
    p = Plot(0, 4, 0, 25); b = [yaxis(p, range(0, 26, 5), 'Deviation from target (ml)')]
    gw = (p.r - p.l) / 4; bw = 32; gap = 2
    for i, t in enumerate(targets):
        cx = p.l + gw * (i + 0.5)
        for j, (v, cls, name) in enumerate([(s1[i], 'f1', 'Flow sensor 1'), (s2[i], 'f2', 'Flow sensor 2')]):
            x = cx - bw - gap / 2 + j * (bw + gap)
            b.append(f'<path class="{cls}" d="{bar_path(x, p.Y(v), bw, p.b)}"><title>{name}, target {t} ml: {v} ml deviation</title></path>')
            b.append(f'<text class="val" x="{x+bw/2:.1f}" y="{p.Y(v)-6:.1f}" text-anchor="middle">{v:g}</text>')
        b.append(f'<text class="tm" x="{cx:.1f}" y="{p.b+18}" text-anchor="middle">{t}</text>')
    b.append(xlabel(p, 'Target volume (ml)'))
    # legend (two series): swatch + name, top right inside the plot
    lx = p.r - 250
    b.append(f'<rect class="f1" x="{lx}" y="{p.t+2}" width="12" height="12" rx="2"/><text class="t" x="{lx+18}" y="{p.t+12}">Flow sensor 1 (4.25 ml/pulse)</text>')
    b.append(f'<rect class="f2" x="{lx}" y="{p.t+22}" width="12" height="12" rx="2"/><text class="t" x="{lx+18}" y="{p.t+32}">Flow sensor 2 (5.25 ml/pulse)</text>')
    return svg('Flow sensor deviation by target volume',
               'Average deviation in millilitres for both flow sensors at 250, 500, 750 and 1000 ml; never more than about 22 ml.', '\n'.join(b))

# 4 ─ Concentration adjustment: % of target achieved ───────────────────────────────────────
def chart_target_achieved():
    rows = [('200', 96.00), ('400', 98.75), ('600', 92.83), ('800', 95.50)]
    p = Plot(0, 4, 0, 108); b = [yaxis(p, range(0, 101, 25), 'Target achieved (%)')]
    gw = (p.r - p.l) / 4; bw = 56
    for i, (t, v) in enumerate(rows):
        x = p.l + gw * (i + 0.5) - bw / 2
        b.append(f'<path class="f1" d="{bar_path(x, p.Y(v), bw, p.b)}"><title>Target {t} ppm: {v}% achieved</title></path>')
        b.append(f'<text x="{x+bw/2:.1f}" y="{p.Y(v)+20:.1f}" text-anchor="middle" style="fill:var(--surface);font-size:13px;font-weight:700">{v:g}%</text>')   # inside the bar: the bars end at the 100% line
        b.append(f'<text class="tm" x="{x+bw/2:.1f}" y="{p.b+18}" text-anchor="middle">{t}</text>')
    b.append(f'<line class="ref" x1="{p.l}" x2="{p.r}" y1="{p.Y(100):.1f}" y2="{p.Y(100):.1f}"/>')
    b.append(f'<text class="tm" x="{p.l+6}" y="{p.Y(100)-6:.1f}">100% = target reached</text>')
    b.append(xlabel(p, 'Target TDS (ppm)'))
    return svg('Fertiliser concentration adjustment',
               'Percentage of the target ppm reached at 200, 400, 600 and 800 ppm: between 92.8 and 98.8 percent.', '\n'.join(b))

# 5 ─ Response time breakdown (horizontal bars) ────────────────────────────────────────────
def chart_response_time():
    rows = [('Solenoid valves open', 10.85), ('Fertiliser dispense (1 L)', 10.79), ('Pumps activation', 1.07)]
    l, r, t = 190, W - 60, 40
    scale = (r - l) / 12.0
    b = []
    for v in range(0, 13, 3):
        x = l + v * scale
        b.append(f'<line class="grid" x1="{x:.1f}" x2="{x:.1f}" y1="{t-10}" y2="{t+rows.__len__()*64}"/>')
        b.append(f'<text class="tm" x="{x:.1f}" y="{t+rows.__len__()*64+16}" text-anchor="middle">{v}</text>')
    b.append(f'<text class="t" x="{(l+r)/2:.0f}" y="{t+rows.__len__()*64+38}" text-anchor="middle">Average response time (seconds)</text>')
    for i, (name, v) in enumerate(rows):
        y = t + i * 64; h = 30
        w = v * scale
        b.append(f'<text class="t" x="{l-10}" y="{y+h/2+4:.1f}" text-anchor="end">{esc(name)}</text>')
        b.append(f'<path class="f1" d="M{l},{y} L{l+w-4:.1f},{y} Q{l+w:.1f},{y} {l+w:.1f},{y+4} L{l+w:.1f},{y+h-4} Q{l+w:.1f},{y+h} {l+w-4:.1f},{y+h} L{l},{y+h} Z"><title>{name}: {v} s</title></path>')
        b.append(f'<text class="val" x="{l+w+8:.1f}" y="{y+h/2+4:.1f}">{v:g} s</text>')
    b.append(f'<text class="ti" x="{ML}" y="{t+3*64+72}">Total 22.17 s, command to completion (stopwatch, via Blynk)</text>')
    return svg('System response time',
               'Average time for solenoid valves to open (10.85 s), fertiliser dispense of one litre (10.79 s) and pump activation (1.07 s); total 22.17 s.', '\n'.join(b), h=t+3*64+96)

if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    for name, fn in [('results-tds-accuracy', chart_tds_accuracy), ('results-tds-stability', chart_tds_stability),
                     ('results-flow-deviation', chart_flow_deviation), ('results-target-achieved', chart_target_achieved),
                     ('results-response-time', chart_response_time)]:
        path = os.path.join(OUT, name + '.svg')
        open(path, 'w').write(fn())
        print('wrote', os.path.relpath(path))
