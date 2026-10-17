#!/bin/zsh
# lock.sh take|release|show [owner]: the Odin 3 is shared with other sessions (Eden, Vita3K, Cemu...). Nobody
# launches a game or measures without holding /data/local/tmp/odin3.lock. A refused take queues its owner in
# odin3.next: nobody else takes the lock while another owner's reservation is under 20 min old.
A=~/Library/Android/sdk/platform-tools/adb
L=/data/local/tmp/odin3.lock
N=/data/local/tmp/odin3.next
OWNER=${2:-dolphin}
queue() { $A shell "grep -q '^$OWNER ' $N 2>/dev/null || echo \"$OWNER \$(date +%s)\" >> $N"; echo "busy: $1"; exit 1; }
case $1 in
  take)
    [ "$($A shell cat $L 2>/dev/null | tr -d '\r' | cut -d' ' -f1)" = "$OWNER" ] && exit 0
    $A shell cat $L >/dev/null 2>&1 && queue "$($A shell cat $L | tr -d '\r')"
    # Fresh reservations of other owners queued before ours go first
    NOW=$(date +%s)
    for e in ${(f)"$($A shell cat $N 2>/dev/null | tr -d '\r')"}; do
      [ "${e% *}" = "$OWNER" ] && break
      [ $((NOW - ${e##* })) -lt 1200 ] && queue "queued after ${e% *}"
    done
    if $A shell "set -C; echo \"$OWNER \$(date +%s)\" > $L" 2>/dev/null; then
      $A shell "sed -i '/^$OWNER /d' $N 2>/dev/null"; exit 0
    fi
    queue "$($A shell cat $L | tr -d '\r')";;
  release) $A shell "grep -q '^$OWNER ' $L 2>/dev/null && rm $L"; exit 0;;
  show) $A shell cat $L $N 2>/dev/null | tr -d '\r';;
  *) echo "usage: lock.sh take|release|show [owner]" >&2; exit 2;;
esac
