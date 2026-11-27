// SPDX-License-Identifier: GPL-3.0-or-later
#include "../platform/RgRumble.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
static int value(const char* path) {FILE* f=fopen(path,"r");assert(f);int n=-1;assert(fscanf(f,"%d",&n)==1);fclose(f);return n;}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    setenv("JAZZ2_RUMBLE_PATH",argv[1],1);
    FILE* f=fopen(argv[1],"w");assert(f);fputs("0\n",f);fclose(f);
    Rg35xx::RumbleEnabled=true;Rg35xx::RumbleIntensity=2;
    auto& motor=Rg35xx::GetMotor();motor.SetStrength(.5f);assert(value(argv[1])==50);
    // Simulate kernel forgetting an unchanged active command: heartbeat must reapply it.
    f=fopen(argv[1],"w");assert(f);fputs("0\n",f);fclose(f);
    usleep(130000);motor.SetStrength(.5f);assert(value(argv[1])==50);
    // Temporarily disappear, then return with exactly the same requested strength.
    assert(unlink(argv[1])==0);motor.SetStrength(.8f);
    f=fopen(argv[1],"w");assert(f);fputs("0\n",f);fclose(f);
    usleep(300000);motor.SetStrength(.8f);assert(value(argv[1])==80);
    // Recover from write error as well as open error, using /dev/full.
    assert(unlink(argv[1])==0);assert(symlink("/dev/full",argv[1])==0);
    motor.SetStrength(.7f);assert(unlink(argv[1])==0);
    f=fopen(argv[1],"w");assert(f);fputs("0\n",f);fclose(f);
    usleep(300000);motor.SetStrength(.7f);assert(value(argv[1])==70);
    motor.Stop();assert(value(argv[1])==0);
    puts("Native motor: active heartbeat, recovery from open/write errors, unchanged-strength retry, stop OK");
}
