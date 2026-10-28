#!/bin/zsh
# measure.sh <outdir> [seconds]: records the frames Dolphin presents (SurfaceFlinger) and the power and thermals
# (poll.sh of the Eden bench, already on the console) while the game runs, then summarizes them.
A=~/Library/Android/sdk/platform-tools/adb
D=${0:A:h}
PKG=${PKG:-org.dolphinemu.dolphinemu.odin3}
OUT=$1; DUR=${2:-60}
mkdir -p $OUT
# SurfaceFlinger keeps the layers of the games stopped before, and the running game has an empty second
# SurfaceView: the newest layer that has frames is the game.
L=
for C in $($A shell "dumpsys SurfaceFlinger --list" | tr -d '\r' | grep -F "SurfaceView[$PKG/" |
    grep -oE 'SurfaceView\[[^]]*\]\(BLAST\)#[0-9]+' | sort -u -t'#' -k2 -nr); do
  $A shell "dumpsys SurfaceFlinger --latency '$C'" | awk 'NF == 3 && $2 > 0 { f = 1 } END { exit !f }' &&
    { L=$C; break; }
done
[ -z "$L" ] && { echo "no Dolphin surface for $PKG"; exit 1; }
echo "layer=$L start=$(date +%T)" > $OUT/context.txt
$A shell sh /data/local/tmp/poll.sh $DUR > $OUT/poll.csv &
t0=$SECONDS
while [ $((SECONDS - t0)) -lt $DUR ]; do
  # SurfaceFlinger keeps only the last 128 presents (~2 s at 60 fps), so sample faster than that.
  $A shell "dumpsys SurfaceFlinger --latency '$L'" >> $OUT/sf_latency.txt
  sleep 1
done
wait
python3 -I $D/summarize.py $OUT | tee $OUT/summary.txt
