# Polyworld 原生 Windows 编译（MSYS2）

在 **MSYS2 UCRT64** 终端中构建原生 Windows 版 `Polyworld.exe`（不用 WSL）。

## 1. 安装 MSYS2

从 https://www.msys2.org/ 安装后，打开 **「MSYS2 UCRT64」** 终端（不要用 MSYS 或 MINGW32）。

若提示 `could not find /tmp`：

```bash
mkdir -p /tmp
```

## 2. 安装依赖

**不要用** Ubuntu 包名 `python-is-python3`（MSYS2 中不存在）。粘贴时注意行尾不要多出 `~` 等字符。

若 `pacman` 报 `bfsu` 镜像 404，编辑 **`/etc/pacman.d/mirrorlist.mingw`**（不是 `mirrorlist.ucrt64`），注释 `bfsu` 行，文件顶部已有 `mirror.msys2.org` 时可只用官方源，然后 `pacman -Syy`。

```bash
pacman -Syu
```

重开 UCRT64 后：

```bash
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-toolchain
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-tools
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gsl mingw-w64-ucrt-x86_64-python
```

若只有 `python` 没有 `python3`：

```bash
ln -sf /ucrt64/bin/python.exe /ucrt64/bin/python3.exe
```

确认：

```bash
which g++ qmake python3
```

## 3. 编译

```bash
cd /c/path/to/polyworld
sed -i 's/\r$//' configure
./configure --os msys2
make app -j$(nproc)
```

## 4. 运行

```bash
export PATH="$PWD/lib:$PATH"
./Polyworld.exe worldfiles/tests/low-spec-pc/minitest.wf
```

## 5. 常见问题

| 问题 | 处理 |
|------|------|
| `python-is-python3` not found | 改用 `mingw-w64-ucrt-x86_64-python` |
| `$'\E[200~pacman'` | 手打命令，避免错误粘贴 |
| Qt 交互选包 | 用 `qt5-base` 而非整组 `qt5` |
| `fork` 链接/运行错误 | 见第 6 节 |

## 6. 已知限制

`interpreter.cc` 使用 `fork()` + `pipe()`，MinGW 上需后续改为 `CreateProcess`。编译/运行失败时可暂用 WSL。

## 7. 中文步骤

见项目根目录 **`Windows安装和编译.md`**。
