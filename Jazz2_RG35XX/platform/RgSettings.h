// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace Rg35xx {
inline constexpr std::int32_t Widths[] = {304, 320, 384, 480, 640};
inline constexpr std::int32_t Heights[] = {228, 240, 288, 360, 480};
inline std::int32_t ResolutionIndex = 1;
inline bool RumbleEnabled = true;
inline std::int32_t RumbleIntensity = 1; // low / medium / high
// Active dimensions stay stable until restart; menus edit ResolutionIndex only.
inline std::int32_t RenderWidth = 320, RenderHeight = 240;
}
