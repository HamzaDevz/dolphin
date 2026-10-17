# summarize.py <outdir>: frame rate and pacing from sf_latency.txt, power and heat from poll.csv.
import collections
import csv
import statistics
import sys

out = sys.argv[1]
presents = set()
for line in open(f"{out}/sf_latency.txt"):
    fields = line.split()
    if len(fields) == 3 and fields[1].isdigit() and 0 < int(fields[1]) < 1 << 62:
        presents.add(int(fields[1]))
presents = sorted(presents)
if len(presents) < 2:
    sys.exit("no frames")
duration = (presents[-1] - presents[0]) / 1e9
gaps = [(b - a) / 1e6 for a, b in zip(presents, presents[1:])]
median = statistics.median(gaps)
per_second = collections.Counter(int((t - presents[0]) / 1e9) for t in presents)
seconds = [per_second[i] for i in range(int(duration))]
line = (f"fps={(len(presents) - 1) / duration:.2f} min_1s={min(seconds) if seconds else 0}"
        f" stutters={sum(g > 1.5 * median for g in gaps)} longest_gap_ms={max(gaps):.1f}"
        f" frames={len(presents)} duration_s={duration:.1f}")
try:
    rows = list(csv.DictReader(open(f"{out}/poll.csv")))
    watts = [-int(r["batt_ma"]) * int(r["batt_mv"]) / 1e6 for r in rows if r["batt_ma"]]
    line += (f" gpu_busy={statistics.mean(int(r['gpu_busy']) for r in rows):.0f}%"
             f" gpu_tmax={max(int(r['gpu_tmax']) for r in rows) / 1000:.0f}C"
             f" batt_w={statistics.mean(watts):.2f}")
except (OSError, KeyError, ValueError, statistics.StatisticsError):
    pass
print(line)
