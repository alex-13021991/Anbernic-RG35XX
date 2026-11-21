#!/bin/sh
# Save boot flags only. Never start/stop services or reboot from this helper.
# Exit codes documented in README. Test harness substitutes mount/sync only.
MISC=/misc
MMC=/mnt/mmc
LOCK=/tmp/rg35xx-usb-manager.lock
case "$1:$2:$3" in
  0:0:0|1:0:0|0:1:0|1:1:0|0:0:1) ;;
  *) exit 10 ;;
esac
mtp=$1; adb=$2; host=$3
if [ "$mtp" = 1 ]; then
  [ -f "$MISC/usb.sh" ] && [ -f "$MMC/CFW/autostart/mtp.sh" ] &&
  [ -f "$MMC/CFW/mtp/run.sh" ] && [ -x "$MMC/CFW/mtp/mtp-server" ] &&
  grep -q '^[[:space:]]*/misc/usb\.sh[[:space:]]*$' "$MISC/dmenu.bin" || exit 11
fi
# Do not destroy nonstandard files using these names.
for flag in enableMTP enableADB waitForUSB; do
  [ ! -L "$MISC/$flag" ] || exit 10
  [ ! -e "$MISC/$flag" ] || [ -f "$MISC/$flag" ] || exit 10
done
mkdir "$LOCK" 2>/dev/null || exit 12
mounted=0; transaction=0
backup="$MISC/.usb-manager-backup"
restore() {
  failed=0
  # First remove new flags, then restore the original set and contents.
  for flag in enableMTP enableADB waitForUSB; do
    rm -f "$MISC/$flag" || failed=1
  done
  for flag in enableMTP enableADB waitForUSB; do
    if [ -f "$backup/$flag" ]; then cp -p "$backup/$flag" "$MISC/$flag" || failed=1; fi
  done
  sync
  [ "$failed" = 0 ]
}
clear_backup() {
  for flag in enableMTP enableADB waitForUSB ready; do rm -f "$backup/$flag" || return 1; done
  rmdir "$backup"
}
cleanup() {
  status=$?
  trap - EXIT HUP INT TERM
  if [ "$transaction" = 1 ]; then
    if restore; then clear_backup || status=15; else status=15; fi
  fi
  if [ "$mounted" = 1 ]; then
    sync
    mount -o remount,ro "$MISC" || status=16
  fi
  rmdir "$LOCK" 2>/dev/null
  exit "$status"
}
trap cleanup EXIT
trap 'exit 14' HUP INT TERM
mount -o remount,rw "$MISC" || exit 13
mounted=1
# A journal left by interrupted Apply is never silently overwritten.
if [ -e "$backup" ]; then
  [ -d "$backup" ] && [ ! -L "$backup" ] || exit 15
  if [ -f "$backup/ready" ]; then restore || exit 15; fi
  clear_backup || exit 15
  # Require the UI to reread recovered settings before a new Apply.
  exit 17
fi
mkdir "$backup" || exit 14
for flag in enableMTP enableADB waitForUSB; do
  if [ -f "$MISC/$flag" ]; then
    cp -p "$MISC/$flag" "$backup/$flag" || { clear_backup; exit 14; }
  fi
done
touch "$backup/ready" || { clear_backup; exit 14; }
sync
transaction=1
# Remove incompatible modes before setting new flags. No live USB changes.
if [ "$host" = 1 ]; then
  rm -f "$MISC/enableMTP" "$MISC/enableADB" || exit 14
  touch "$MISC/waitForUSB" || exit 14
else
  rm -f "$MISC/waitForUSB" || exit 14
  if [ "$mtp" = 1 ]; then touch "$MISC/enableMTP"; else rm -f "$MISC/enableMTP"; fi || exit 14
  if [ "$adb" = 1 ]; then touch "$MISC/enableADB"; else rm -f "$MISC/enableADB"; fi || exit 14
fi
sync
for pair in "enableMTP:$mtp" "enableADB:$adb" "waitForUSB:$host"; do
  flag=${pair%:*}; expected=${pair#*:}; actual=0
  [ ! -f "$MISC/$flag" ] || actual=1
  [ "$actual" = "$expected" ] || exit 14
done
# Deleting ready is the commit point. A crash after this keeps the new flags.
rm -f "$backup/ready" || exit 14
sync
transaction=0
clear_backup || exit 15
exit 0
