// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "RgSettings.h"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

namespace Rg35xx {
// Native motor bridge for the engine's rumble timelines. Failures are recoverable.
class Motor {
    int last = -1, target = 0;
    std::uint64_t lastWrite = 0, retryAt = 0, lastReport = 0;
    std::uint64_t effects = 0, writes = 0, errors = 0;
    bool recovering = false;
    static std::uint64_t nowMs() {
        timespec t{}; ::clock_gettime(CLOCK_MONOTONIC, &t);
        return std::uint64_t(t.tv_sec)*1000 + std::uint64_t(t.tv_nsec)/1000000;
    }
    const char* path() const {
        const char* overridePath = std::getenv("JAZZ2_RUMBLE_PATH");
        return overridePath && *overridePath ? overridePath : "/sys/class/power_supply/battery/moto";
    }
    void report(std::uint64_t now) {
        if (now-lastReport < 10000) return;
        std::fprintf(stderr,"RG35XX: rumble effects=%llu target=%d applied=%d writes=%llu errors=%llu\n",
                     (unsigned long long)effects,target,last,(unsigned long long)writes,(unsigned long long)errors);
        lastReport=now;
    }
    void fail(std::uint64_t now,int error) {
        ++errors;
        if (!recovering) std::fprintf(stderr,"RG35XX: motor I/O error (%s, errno=%d); retrying\n",path(),error);
        recovering=true;
        last=-1; // A failed command must never poison the strength cache.
        retryAt=now+250;
    }
    void writeStrength(int value) {
        target=value;
        const auto now=nowMs();
        report(now);
        if (now<retryAt) return;
        // Refresh an active command periodically; a cached value alone is not
        // proof the kernel motor is still running. Idle zero needs no heartbeat.
        if (value==last && (value==0 || now-lastWrite<100)) return;
        int fd=::open(path(),O_WRONLY|O_CLOEXEC);
        if (fd<0) { fail(now,errno); return; }
        char buffer[8];
        int count=std::snprintf(buffer,sizeof(buffer),"%d\n",value);
        ssize_t result;
        do { result=::write(fd,buffer,count); } while(result<0 && errno==EINTR);
        int error=result<0 ? errno : EIO;
        ::close(fd);
        if(result!=count) { fail(now,error); return; }
        if(recovering) std::fprintf(stderr,"RG35XX: motor I/O recovered\n");
        recovering=false;retryAt=0;last=value;lastWrite=now;++writes;
    }
public:
    Motor() { lastReport=nowMs();writeStrength(0); }
    ~Motor() { Stop(); }
    Motor(const Motor&)=delete;
    Motor& operator=(const Motor&)=delete;
    void NoteEffect() { ++effects; }
    void SetStrength(float gain) {
        const float scales[]={0.35f,0.65f,1.0f};
        const int index=std::clamp(RumbleIntensity,0,2);
        const int value=RumbleEnabled ? int(std::clamp(gain,0.0f,1.0f)*scales[index]*100.0f+0.5f) : 0;
        writeStrength(value);
    }
    void Stop() { writeStrength(0); }
};
inline Motor& GetMotor() { static Motor motor;return motor; }
}
