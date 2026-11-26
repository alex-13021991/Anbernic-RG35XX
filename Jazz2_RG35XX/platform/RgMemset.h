// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>

// GarlicOS uClibc's ARM memset fails to truncate negative int values before
// broadcasting them to words. Export an application-local replacement, also
// used by its shared libraries. Never change the system libc.
extern "C" __attribute__((visibility("default"), optimize("no-tree-loop-distribute-patterns")))
void* memset(void* destination, int value, std::size_t count) noexcept
{
    auto* p = static_cast<unsigned char*>(destination);
    const unsigned char byte = static_cast<unsigned char>(value);
    void* result = destination;
    while (count && (reinterpret_cast<std::uintptr_t>(p) & 3)) {
        *p++ = byte; --count;
    }
    using Word = std::uint32_t __attribute__((may_alias));
    const Word word = Word(byte) * 0x01010101u;
    auto* words = reinterpret_cast<Word*>(p);
    while (count >= 16) {
        words[0] = word; words[1] = word; words[2] = word; words[3] = word;
        words += 4; count -= 16;
    }
    while (count >= 4) { *words++ = word; count -= 4; }
    p = reinterpret_cast<unsigned char*>(words);
    while (count--) *p++ = byte;
    return result;
}
