# Installing on Windows (MSYS2)

Windows native builds use **MSYS2 UCRT64** (MinGW-w64), not Cygwin/Qt4.

See the full guide: [WINDOWS_BUILD.md](../../WINDOWS_BUILD.md)

Quick start (in **MSYS2 UCRT64** bash):

```bash
mkdir -p /tmp
pacman -Syu
# 重开 UCRT64 后：
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-toolchain
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-tools
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gsl mingw-w64-ucrt-x86_64-python
ln -sf /ucrt64/bin/python.exe /ucrt64/bin/python3.exe

cd /c/path/to/polyworld
./configure --os msys2
make app -j$(nproc)
export PATH="$PWD/lib:$PATH"
./Polyworld.exe worldfiles/tests/low-spec-pc/minitest.wf
```

WSL2 Linux build remains an alternative; see [Installing-on-Linux](Installing-on-Linux.md).
