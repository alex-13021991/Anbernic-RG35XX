# CMake toolchain for the Miyoo Mini: SigmaStar SSD202D, 2x Cortex-A7, NEON + VFPv4.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# -mcpu=cortex-a7 tunes scheduling for the in-order pipeline; -mfpu=neon-vfpv4 is what the A7
# actually has and is required for the __ARM_NEON paths in SwRaster to compile in at all.
set(A7_FLAGS "-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -mthumb")
set(CMAKE_C_FLAGS_INIT   "${A7_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${A7_FLAGS}")

# Debian multiarch keeps armhf headers in the shared /usr/include and armhf libraries under
# /usr/lib/arm-linux-gnueabihf. CMAKE_LIBRARY_ARCHITECTURE is what steers find_library() into
# the right per-arch subdirectory; the root-path modes stay BOTH so the shared headers are
# still visible (ONLY would hide all of /usr/include and nothing would be found).
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf /usr/lib/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/share/pkgconfig")
