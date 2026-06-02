# Polyworld 原生 Windows 编译（MSYS2 UCRT64）

在 **「MSYS2 UCRT64」** 终端里操作（开始菜单里选 UCRT64，不要用 PowerShell、不要用普通 MSYS）。

---

## 0. 先处理两个小问题

### `/tmp` 警告

若启动时出现 `could not find /tmp`，在 UCRT64 里执行一次：

```bash
mkdir -p /tmp
```

### 粘贴命令出现 `$'\E[200~pacman'` 或行尾多了 `~`

这是 **带标记的粘贴**（Bracketed Paste）或从网页/文档复制了多余字符。

- 请 **手打** `pacman`，或从本文 **一行一行** 复制（不要带 `~`）。
- 出现 `Enter a selection (default=all):` 时，直接按 **Enter**（装全部）即可。

---

## 1. 安装依赖（复制下面整块，或分四行执行）

**不要** 安装 `python-is-python3`——那是 Ubuntu 的包名，MSYS2 里没有，你之前的 `python-is-python3~` 末尾 `~` 也是粘贴错误。

```bash
mkdir -p /tmp
pacman -Syu
```

若提示关闭终端重开，关掉 UCRT64 再打开一次，然后：

```bash
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-toolchain
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-tools
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gsl mingw-w64-ucrt-x86_64-python
```

说明：

| 包 | 作用 |
|----|------|
| `toolchain` | g++、make、链接器 |
| `qt5-base` + `qt5-tools` | Qt5 与 **qmake**（不必装整个 `qt5` 组 38 项，省空间） |
| `gsl` | 数值库 |
| `python` | 解析 worldfile；提供 `/ucrt64/bin/python.exe` |

检查：

```bash
which g++ qmake-qt5 python3
g++ --version
```

MSYS2 上 Qt 提供的是 **`qmake-qt5`**，不是 `qmake`。任选其一：

```bash
ln -sf /ucrt64/bin/qmake-qt5.exe /ucrt64/bin/qmake.exe
```

或编译时：`./configure --os msys2 --qmake qmake-qt5`

若只有 `python` 没有 `python3`：

```bash
ln -sf /ucrt64/bin/python.exe /ucrt64/bin/python3.exe
```

---

## 2. 编译 Polyworld

每次打开 UCRT64 建议先保证工具在 PATH 里：

```bash
export PATH="/ucrt64/bin:$PATH"
ln -sf /ucrt64/bin/mingw32-make.exe /ucrt64/bin/make.exe
which g++ make qmake python3
```

若 `make: command not found`：MSYS2 装的是 **`mingw32-make`**，不是 `make`，需软链：

```bash
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-make
ln -sf /ucrt64/bin/mingw32-make.exe /ucrt64/bin/make.exe
```

```bash
cd /c/Users/glsa-/Documents/GitHub/neat_thinking_unit/Polyworld/polyworld

sed -i 's/\r$//' configure
./configure --os msys2
make app -j$(nproc)
```

**不要**在已建好 `qmake` 软链后再加 `--qmake qmake-qt5`；第一次 `./configure --os msys2` 成功即可。若需指定 Qt，用 `--qmake qmake`（指向软链后的名字）。

或：

```bash
bash scripts/build-msys2.sh
```

成功后会得到 `Polyworld.exe`、`lib/libpolyworld.dll` 等。

---

## 3. 运行

**编译**必须在 UCRT64（或同等 MinGW 环境）里完成。**运行**不强制 UCRT64：在 **cmd / PowerShell** 里也可以，只要：

1. 当前目录是 **polyworld 根目录**（含 `etc/`、`worldfiles/`、`lib/`）
2. `PATH` 同时包含 **`polyworld/lib`** 与 **`/ucrt64/bin`**（或 `C:\msys64\ucrt64\bin`）：前者有 `libpolyworld.dll`、`pwqtrenderer.dll`；后者有 Qt、Python、GSL 等
3. 从 UCRT64 启动最省事（PATH、Python 已配好）

**首次启动**会在 `runs/run/.cppprops/` 里根据 worldfile **自动编译** `libcppprops.dll`（约 10–20 秒），属正常，不要中断。

必须在 **polyworld 根目录**（有 `etc/`、`worldfiles/` 的目录）：

```bash
cd /c/Users/glsa-/Documents/GitHub/neat_thinking_unit/Polyworld/polyworld
export PATH="/ucrt64/bin:$PATH"
export PATH="$PWD/lib:$PATH"
./Polyworld.exe worldfiles/tests/low-spec-pc/minitest.wf
```

也可一行：

```bash
cd /c/Users/glsa-/Documents/GitHub/neat_thinking_unit/Polyworld/polyworld
export PATH="$PWD/lib:/ucrt64/bin:$PATH"
./Polyworld.exe worldfiles/tests/low-spec-pc/minitest.wf
```

在 **PowerShell** 中示例：

```powershell
cd C:\Users\...\neat_thinking_unit\Polyworld\polyworld
$env:PATH = "$PWD\lib;C:\msys64\ucrt64\bin;$env:PATH"
.\Polyworld.exe worldfiles\tests\low-spec-pc\minitest.wf
```

GUI：

```bash
./Polyworld.exe worldfiles/hello.wf
```

---

## 4. 常见错误

### pacman 镜像 404（`mirrors.bfsu.edu.cn` / `failed retrieving file`）

国内镜像偶发 **包已更新、签名文件未同步**，会报：

```text
error: failed retrieving file '...pkg.tar.zst.sig' ... 404
error: failed to commit transaction
```

**处理（在 UCRT64 里按顺序试）：**

**办法 A — 换官方源（推荐）**

新版 MSYS2 的 UCRT64 镜像在 **`/etc/pacman.d/mirrorlist.mingw`**（没有 `mirrorlist.ucrt64`）：

```bash
sed -i 's|^Server.*bfsu|# &|' /etc/pacman.d/mirrorlist.mingw
sed -i '1i Server = https://mirrors2.msys2.org/mingw/$repo/' /etc/pacman.d/mirrorlist.mingw
pacman -Syy
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-toolchain
```

**办法 B — 仍用清华/中科大，先刷新密钥环**

```bash
pacman -Sy archlinux-keyring msys2-keyring mingw-w64-ucrt-x86_64-keyring
pacman -Syy
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-toolchain
```

**办法 C — 稍后重试**

过几小时再执行同一条 `pacman -S`（镜像同步滞后时常自行恢复）。

出现 `Enter a selection (default=all):` 时 **直接按 Enter**。

---

| 现象 | 处理 |
|------|------|
| `target not found: python-is-python3` | 删掉该行；MSYS2 用 `mingw-w64-ucrt-x86_64-python` |
| `command not found: $'\E[200~pacman'` | 重新手打命令，勿从富文本复制 |
| Qt 组让你选 1–38 | 改用上面的 **`qt5-base` + `qt5-tools`**，或两次都按 Enter |
| `Unsupported OS (windows)` | 必须在 **UCRT64** bash 里 `./configure --os msys2` |
| `make: command not found` | `ln -sf /ucrt64/bin/mingw32-make.exe /ucrt64/bin/make.exe` |
| `fatal error: gl.h: No such file` | `OPENGL_CXXFLAGS` 用 `C:/msys64/ucrt64/include/GL`；qtrenderer 需 `make clean` 后重编 |
| `'drand48' was not declared` | 已加 `utils/pw_rand48.cc`（MinGW 无 POSIX `drand48`）；重新 `make app` |
| `'M_PI' was not declared` | 已在 `misc.h` 为 MinGW 补充 `M_PI`/`M_PI_2`；重新 `make app` |
| 指针转 `long` 精度丢失 (`gcamera.cc`) | 已改为 `intptr_t`；重新 `make app` |
| `sys/errno.h: No such file` | 已改为标准 `<errno.h>`（`Simulation.cc`、`misc.cc`） |
| `pipe` / `fork` / `setenv`（`interpreter.cc`） | Windows 下用 `CreateProcess` + 管道；需 `python3` 在 PATH |
| `::link` 未声明（`AbstractFile.cc`） | Windows 下用 `CreateHardLinkA` |
| `byteswap.h: No such file`（`PwMovieUtils.cc`） | Windows 用 `_byteswap_*` + `winsock2`；链接 `-lws2_32` |
| `cannot find -ldl`（链接 libpolyworld.dll） | 已修 `Makefile.conf`：msys2 不再链接 `-ldl` |
| `undefined reference to SceneRenderer::create` | 已加 `pw_renderer_win.cc` + Qt 插件 C 导出；先链 `libpolyworld.dll` 再编 `qtrenderer` |
| 链接 `Polyworld.exe` 大量 `undefined reference`（仅 moc/*.o） | `.qt.pro` 的 `SOURCES` 未含子目录；已改 `wildcard`；`make -C src/app clean` 后重编 |
| `QtSceneRenderer::copyTo` 等未定义 | `libpwqtrenderer.dll` 需 `-Wl,--export-all-symbols` |
| `sys/select.h`（`termio.cc`） | Windows 下改用 `conio.h` / 控制台 API |
| Python `AssertionError`（`header` 为 `''` 或 `'<expr>\\r'`） | 须用 `MSYS2_PREFIX/bin/python3.exe` 起子进程；`interpreter.py` 已 `strip`；重编 `libpolyworld.dll` |
| 启动时乱码 `Ŀ¼... -p ... runs ...` 后立刻退出 | Windows 上勿用 `mkdir -p`（会走 cmd）；已改 `makeDirs` 用 `_mkdir`；`make -C src/library` 后重跑 |
| `Failed executing command 'mkdir -p runs/run/.cppprops'` | 勿用 cmd 的 `mkdir -p`；已改 `makeDirs` + MSYS `bash` 编 `libcppprops.dll`；**必须**重编 `libpolyworld.dll`（`make` 显示 up to date 时先 `rm .bld/library/utils/misc.o .bld/library/proplib/cppprops.o` 再 `make -C src/library`） |
| `Failed opening ... libcppprops.dll` | 确认已用**新** `libpolyworld.dll`；`PATH` 含 `$PWD/lib`；见上节强制重编 |
| `Failed opening ... libpwqtrenderer.dll` / `pwqtrenderer.dll` | 确认 `lib/pwqtrenderer.dll` 存在（`make app` 会生成）；`PATH` 含 `$PWD/lib` |
| `make` 显示 up to date 但运行仍报旧错误 | 强制重编库：`rm -f .bld/library/utils/misc.o .bld/library/proplib/cppprops.o .bld/library/utils/pw_renderer_win.o && make -C src/library` |
| `Cannot locate qmake` 第二次 configure | 已有 `qmake` 软链时用 `./configure --os msys2` 即可，勿重复 `--qmake qmake-qt5` |

---

## 5. 已知限制

- **完整构建**需 `make app`（含 `qtrenderer`）；仅 `make -C src/library` 不够运行 GUI。
- **cppprops** 每次新 `runs/run` 可能重新 `make`；依赖 MSYS2 的 `bash` 与 `/ucrt64/bin` 里的 `g++`、`make`（程序内通过 `pwSystem` 调用）。
- 若 worldfile / Python 子进程仍异常，可暂时用 **WSL** 对照；多数启动问题已按 Windows 路径修过。

更完整说明见 `docs/WINDOWS_BUILD.md`（若存在）。
