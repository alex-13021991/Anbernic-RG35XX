#ifndef RG_CORE_H
#define RG_CORE_H
#include <stddef.h>
typedef struct { int mtp, adb, host; } UsbChoice;
static inline void usb_toggle(UsbChoice *s, int row) {
    if (row == 0) { s->mtp = !s->mtp; if (s->mtp) s->host = 0; }
    if (row == 1) { s->adb = !s->adb; if (s->adb) s->host = 0; }
    if (row == 2) { s->host = !s->host; if (s->host) s->mtp = s->adb = 0; }
}
static inline int usb_equal(UsbChoice a, UsbChoice b) {
    return a.mtp == b.mtp && a.adb == b.adb && a.host == b.host;
}
static inline int usb_valid(UsbChoice s) {
    return (s.mtp == 0 || s.mtp == 1) && (s.adb == 0 || s.adb == 1)
        && (s.host == 0 || s.host == 1) && !(s.host && (s.mtp || s.adb));
}
/* Trim only at a UTF-8 codepoint boundary. Invalid filenames are replaced by
   the renderer; filenames used for I/O always retain their original bytes. */
static inline size_t utf8_previous(const char *s, size_t n) {
    if (!n) return 0;
    --n;
    while (n && (((unsigned char)s[n] & 0xc0) == 0x80)) --n;
    return n;
}
static inline int grid_move(int index, int count, int dx, int dy) {
    int n;
    if (!count) return 0;
    n = index + dx + 3 * dy;
    if (n < 0) n = 0;
    if (n >= count) n = count - 1;
    return n;
}
#endif
