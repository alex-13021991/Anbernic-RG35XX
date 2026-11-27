// SPDX-License-Identifier: GPL-3.0-or-later
#include "../platform/RgRumble.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
static int readMotor(const char* path) {
    FILE* f=std::fopen(path,"r"); assert(f);
    int value=-1; assert(std::fscanf(f,"%d",&value)==1); std::fclose(f); return value;
}
int main(int argc,char** argv) {
    if(argc!=2) return 2; // Tests require an explicit fake writable motor file.
    setenv("JAZZ2_RUMBLE_PATH",argv[1],1);
    FILE* f=std::fopen(argv[1],"w");assert(f);std::fputs("0\n",f);std::fclose(f);
    auto& motor=Rg35xx::GetMotor();
    const int expected[]={35,65,100};
    for(int i=0;i<3;++i) {
        Rg35xx::RumbleIntensity=i;motor.SetStrength(1.0f);
        assert(readMotor(argv[1])==expected[i]);
        motor.Stop();assert(readMotor(argv[1])==0);
    }
    Rg35xx::RumbleEnabled=false;motor.SetStrength(1.0f);assert(readMotor(argv[1])==0);
    Rg35xx::RumbleEnabled=true;motor.SetStrength(2.0f);assert(readMotor(argv[1])==100);
    motor.SetStrength(-1.0f);assert(readMotor(argv[1])==0);
    motor.SetStrength(.5f);assert(readMotor(argv[1])==50);
    { Rg35xx::Motor temporary;temporary.SetStrength(1.0f); }
    assert(readMotor(argv[1])==0);
    std::puts("RG35XX motor: 35/65/100 intensity, clamping, disable, stop, destructor cleanup OK");
}
