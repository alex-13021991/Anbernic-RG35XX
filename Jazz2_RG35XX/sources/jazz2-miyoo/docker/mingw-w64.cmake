# CMake toolchain for cross-building the Windows AssetPacker from Linux.
set(CMAKE_SYSTEM_NAME Windows)
# AMD64, not x86_64: ncine_imported_targets.cmake picks the prebuilt Windows library directory with
# `if(MINGW AND CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")`. With x86_64 it silently falls back to the
# 32-bit set and the link fails on every zlib symbol.
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# The -posix variants matter: Debian defaults mingw to the win32 threading model, where libstdc++
# ships no std::mutex or std::thread, and the shared code uses both.
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 /opt/sdl2-mingw)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# -mconsole: the tool prints its progress and is run from a .bat, so it must be a console app.
# Static: the .exe is then a single file a player can double-click with nothing installed.
# (AssetPacker's wmain() entry point needs -municode, but that is added to the target itself - putting
# it here would also reach CMake's compiler probes, which define a plain main() and would fail to link.)
set(CMAKE_EXE_LINKER_FLAGS "-mconsole -static -static-libgcc -static-libstdc++")
