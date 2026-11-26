# CMake toolchain for the real Miyoo Mini device (OnionOS), adapted from the captain-claw-miyoo
# port's miyoo_toolchain.cmake. Targets the device sysroot (glibc 2.28) so the binary actually
# starts there - unlike docker/armhf-a7.cmake, which is Debian glibc 2.36 and qemu-only.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_ROOT /opt/miyoomini-toolchain/usr)
set(CMAKE_SYSROOT ${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/sysroot)

set(CMAKE_C_COMPILER ${TOOLCHAIN_ROOT}/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/bin/arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Cortex-A7 (SigmaStar SSD202D). -march=armv7ve + neon-vfpv4 is what the core actually is;
# this is also what enables the __ARM_NEON paths in SwRaster.
# Logical (internal render) resolution is 304x228, not the panel's 640x480. The rasterizer is fill
# bound - a fifth off each axis roughly halved the frame time - and 304x228 is the point where a
# Carrotus level clears the vblank budget while costing only 5% of the view. See NOTES step 17.
#
# -mthumb (the Captain Claw toolchain used -marm): Thumb-2 is roughly a quarter smaller, and this
# core has only 32 KB of L1 instruction cache, so density tends to matter more than the odd extra
# instruction. Being measured against the -marm baseline of ~19.5 ms of work per frame.
# JAZZ2_LOGICAL_* belongs here rather than on the configure command line: this toolchain file
# assigns CMAKE_CXX_FLAGS unconditionally, which shadows any -D CMAKE_CXX_FLAGS= passed to
# cmake, so defines given there are silently dropped (that is how the first device build ended
# up rendering at the desktop default 720x405 instead of 320x240).
set(CMAKE_C_FLAGS "-DJAZZ2_LOGICAL_WIDTH=304 -DJAZZ2_LOGICAL_HEIGHT=228 -mthumb -mtune=cortex-a7 -march=armv7ve -mfpu=neon-vfpv4 -mfloat-abi=hard -I${CMAKE_SYSROOT}/usr/include/SDL2")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# The device's libSDL2.so declares NEEDED on libEGL.so.1/libGLESv2.so.2, which exist on-device
# under Miyoo's MI GPU stack but not in this sysroot. The software renderer never touches them.
set(CMAKE_EXE_LINKER_FLAGS "-L${CMAKE_SYSROOT}/usr/lib -lpthread -Wl,--allow-shlib-undefined -Wl,--unresolved-symbols=ignore-in-shared-libs")

set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_SKIP_RPATH ON)
