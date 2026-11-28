// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <cstdint>
enum class GameSpeedMode : int
{
    SPEED_HALF = 0, SPEED_1X = 1, SPEED_1_5X = 2, SPEED_2X = 3
};
inline GameSpeedMode NextGameSpeed(GameSpeedMode speed)
{
    if (speed == GameSpeedMode::SPEED_1X) return GameSpeedMode::SPEED_HALF;
    if (speed == GameSpeedMode::SPEED_HALF) return GameSpeedMode::SPEED_2X;
    return GameSpeedMode::SPEED_1X;
}
inline int GameSpeedUpdates(GameSpeedMode speed, uint32_t& counter)
{
    if (speed == GameSpeedMode::SPEED_HALF) return (++counter & 1) ? 1 : 0;
    if (speed == GameSpeedMode::SPEED_2X) return 2;
    if (speed == GameSpeedMode::SPEED_1_5X) return (++counter & 1) ? 2 : 1;
    return 1;
}
