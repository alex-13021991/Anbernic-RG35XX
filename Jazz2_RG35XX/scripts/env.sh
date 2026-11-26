# Общие пути к рабочим файлам в Linux. Подключается из сценариев bash.
PORT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
WORK=${JAZZ2_WORK:-"$HOME/.cache/jazz2-rg35xx"}
export RG_SYSROOT="${RG_SYSROOT:-$WORK/sysroot}" RG_TOOLCHAIN="${RG_TOOLCHAIN:-$WORK/miyoo}"
VENDOR="$PORT/vendor"
TRIPLE=arm-miyoo-linux-uclibcgnueabi
export PATH="$PATH:$RG_TOOLCHAIN/bin" LC_ALL=C
CPU="-marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp"
JOBS=${JOBS:-8}
