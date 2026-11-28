#include "GameSpeed.h"
#include <cassert>
#include <cstdio>
int main() {
 auto speed=GameSpeedMode::SPEED_1X;
 for(int round=0;round<4;++round){
  speed=NextGameSpeed(speed);assert(speed==GameSpeedMode::SPEED_HALF);
  uint32_t counter=0;int updates=0;
  for(int i=0;i<100;++i) updates+=GameSpeedUpdates(speed,counter);
  assert(updates==50);
  speed=NextGameSpeed(speed);assert(speed==GameSpeedMode::SPEED_2X);
  counter=0;updates=0;for(int i=0;i<100;++i)updates+=GameSpeedUpdates(speed,counter);assert(updates==200);
  speed=NextGameSpeed(speed);assert(speed==GameSpeedMode::SPEED_1X);
  counter=0;updates=0;for(int i=0;i<100;++i)updates+=GameSpeedUpdates(speed,counter);assert(updates==100);
 }
 puts("PASS: SELECT cycle 1 -> 0.5 -> 2 -> 1; 50/200/100 simulation updates per 100 ticks");
}
