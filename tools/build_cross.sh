#!/usr/bin/env bash
# Cross-compile DeathAxe from Linux/macOS using Zig as a drop-in C++ cross compiler.
#   pip install ziglang        (or any zig >= 0.11 on PATH)
#   ./tools/build_cross.sh <win64|linux64> [path/to/vst3sdk]
# Output: dist/<target>/DeathAxe.vst3   (Windows: a plain DLL renamed .vst3)
set -euo pipefail
TARGET="${1:-win64}"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${2:-${VST3_SDK_ROOT:-$HERE/vst3sdk}}"
ZIG="${ZIG:-$(command -v zig || python3 -c 'import ziglang,os;print(os.path.join(os.path.dirname(ziglang.__file__),"zig"))')}"
[ -d "$SDK/pluginterfaces" ] || { echo "VST3 SDK not found at $SDK (git clone --recurse-submodules https://github.com/steinbergmedia/vst3sdk)"; exit 1; }

case "$TARGET" in
  win64)   ZT=x86_64-windows-gnu; OUT=DeathAxe.vst3; EXTRA="-lole32 -luuid -lshell32 -ladvapi32 -lgdi32 -lcomdlg32 -luser32"; PLAT=win ;;
  linux64) ZT=x86_64-linux-gnu;   OUT=DeathAxe.so;   EXTRA="-ldl -fPIC"; PLAT=linux ;;
  *) echo "unknown target $TARGET"; exit 1 ;;
esac

SRC=(
  "$SDK/base/source/baseiids.cpp" "$SDK/base/source/fbuffer.cpp" "$SDK/base/source/fdebug.cpp"
  "$SDK/base/source/fobject.cpp" "$SDK/base/source/fstreamer.cpp" "$SDK/base/source/fstring.cpp"
  "$SDK/base/source/updatehandler.cpp" "$SDK/base/thread/source/fcondition.cpp" "$SDK/base/thread/source/flock.cpp"
  "$SDK/pluginterfaces/base/conststringtable.cpp" "$SDK/pluginterfaces/base/coreiids.cpp"
  "$SDK/pluginterfaces/base/funknown.cpp" "$SDK/pluginterfaces/base/ustring.cpp"
  "$SDK/public.sdk/source/common/commoniids.cpp" "$SDK/public.sdk/source/common/pluginview.cpp"
  "$SDK/public.sdk/source/main/pluginfactory.cpp"
  "$SDK/public.sdk/source/vst/vstaudioeffect.cpp" "$SDK/public.sdk/source/vst/vstbus.cpp"
  "$SDK/public.sdk/source/vst/vstcomponent.cpp" "$SDK/public.sdk/source/vst/vstcomponentbase.cpp"
  "$SDK/public.sdk/source/vst/vsteditcontroller.cpp" "$SDK/public.sdk/source/vst/vstinitiids.cpp"
  "$SDK/public.sdk/source/vst/vstparameters.cpp"
  "$HERE/src/vst3/deathaxe_vst3.cpp"
)
if [ "$PLAT" = win ]; then SRC+=("$SDK/public.sdk/source/main/dllmain.cpp"); else SRC+=("$SDK/public.sdk/source/main/linuxmain.cpp" "$SDK/base/source/timer.cpp"); fi

BUILD="$HERE/build/$TARGET"; DIST="$HERE/dist/$TARGET"
mkdir -p "$BUILD" "$DIST"
export ZIG_GLOBAL_CACHE_DIR="${ZIG_GLOBAL_CACHE_DIR:-$HERE/build/.zigcache}" ZIG_LOCAL_CACHE_DIR="$HERE/build/.zigcache-local"
CXXFLAGS="-std=c++17 -O2 -DRELEASE=1 -DNDEBUG -I$SDK -fno-strict-aliasing -Wno-deprecated-declarations -Wno-attributes"

# compile objects in parallel
printf '%s\n' "${SRC[@]}" | xargs -P "$(nproc 2>/dev/null || echo 4)" -I{} sh -c \
  "'$ZIG' c++ -target $ZT $CXXFLAGS -c '{}' -o '$BUILD/'\$(basename '{}' .cpp).o"
"$ZIG" c++ -target "$ZT" -shared -O2 "$BUILD"/*.o -o "$DIST/$OUT" $EXTRA
echo "built: $DIST/$OUT"
