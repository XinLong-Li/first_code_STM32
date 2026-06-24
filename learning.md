# J-Link 烧录 STM32 命令详解

## 整体流程

整个脚本分为两部分：
1. **J-Link 命令脚本** (`/tmp/flash.jlink`)：告诉 J-Link 要做什么
2. **JLinkExe 命令行**：启动 J-Link 并执行脚本

---

## 第一部分：J-Link 命令脚本

```bash
cat > /tmp/flash.jlink << 'EOF'
...命令...
EOF
```

`cat > file << 'EOF'` 是 Bash 的 **heredoc（Here Document）** 语法：
- 将 `EOF` 之间的内容写入 `/tmp/flash.jlink` 文件
- `'EOF'`（加引号）阻止 shell 对内容做变量展开，确保 `$` 等字符原样写入

### 逐条命令解释

| 命令 | 含义 | 补充说明 |
|------|------|----------|
| `device STM32F103C8` | 告诉 J-Link 目标芯片型号 | 芯片手册中写的是 STM32F103**C8T6**，这里简写为 C8 即可。J-Link 根据此信息知道 Flash 大小（64KB）、RAM 大小（20KB）、内核类型（Cortex-M3）等参数 |
| `si SWD` | 选择 SWD 调试接口 | `si` = Select Interface。ARM 芯片有两种调试接口：**JTAG**（5 线）和 **SWD**（2 线：SWDIO + SWCLK）。SWD 更省引脚，STM32 最小系统板常用 |
| `speed 4000` | 设置 SWD 时钟频率为 4000 kHz (4 MHz) | 速度太快可能导致信号不稳定；SWD 一般 1–4 MHz 是安全值 |
| `connect` | 建立与目标芯片的连接 | 这一步会：① 通过 SWD 协议检测 CoreSight Debug Port；② 读取 CPUID 确认 Cortex-M3；③ 扫描内存映射 |
| `erase` | 擦除芯片 | 没有指定地址范围时，默认**全片擦除**。J-Link 会自动先复位并暂停内核，然后擦除整个 Flash |
| `loadfile <path>` | 将 hex 文件烧录到 Flash | J-Link 支持 `.hex`（Intel HEX）、`.bin`（纯二进制）、`.elf`（带调试信息）等格式。烧录前会自动复位暂停，烧录后自动校验 |
| `r` | 复位芯片 | `r` = Reset。让芯片从复位向量重新开始运行 |
| `g` | 启动运行 | `g` = Go。释放内核，让它开始执行程序（从复位向量 `0x08000000` 开始） |
| `exit` | 退出 J-Link Commander | 关闭连接，释放调试器 |

---

## 第二部分：JLinkExe 命令行

```bash
JLinkExe -commandfile /tmp/flash.jlink -autoconnect 1 -nogui 1
```

| 参数 | 含义 |
|------|------|
| `-commandfile /tmp/flash.jlink` | 指定要执行的命令脚本文件。JLinkExe 启动后会逐条执行其中的命令 |
| `-autoconnect 1` | 自动连接。`1` 表示连接第一个找到的 J-Link 调试器（如果你只接了一个 J-Link，这个参数可以让你免去交互式选择） |
| `-nogui 1` | 无 GUI 模式。不弹出 SEGGER 的图形界面，纯命令行运行，适合 CI/脚本自动化 |
| `2>&1` | Shell 重定向：把 stderr（标准错误）合并到 stdout（标准输出），这样所有输出都在一起，方便查看 |

### 为什么要 `2>&1`

J-Link 的一些诊断信息（如连接失败提示）可能输出到 stderr。`2>&1` 确保这些信息也能在终端看到，不会悄悄丢失。

---

## 烧录过程发生了什么

从 J-Link 的输出日志可以还原完整的硬件操作流程：

```
1. 连接 J-Link 硬件
   → Firmware: J-Link ARM-OB STM32  (这是板载 J-Link OB，集成在开发板上的)
   → VTref=3.300V                    (目标板供电正常，3.3V)

2. 建立 SWD 通信
   → Executing JTAG -> SWD switching sequence   (发送 SWD 切换序列)
   → Found SW-DP with ID 0x1BA01477            (发现 SW 调试端口)

3. 识别内核
   → CPUID: 0x411FC231 → Cortex-M3 r1p1        (确认是 Cortex-M3)

4. 擦除
   → Erasing device... Done                      (全片擦除)

5. 烧录
   → Bank 0 @ 0x08000000: 1 range affected (1024 bytes)   (烧到 Flash 起始地址)
   → Program & Verify speed: 23 KB/s                       (写入 + 校验速度)

6. 复位并运行
   → Reset: Reset device via AIRCR.SYSRESETREQ   (通过内核寄存器复位)
   → g                                           (启动运行)
```

---

## 常用变体

### 只烧录不复位（调试用）
```
connect
loadfile xxx.hex
exit
```

### 烧录 bin 文件（需要指定地址）
```
loadbin /path/to/firmware.bin 0x08000000
```

### 读出 Flash 内容
```
savebin /path/to/dump.bin 0x08000000 0x10000
```
这条命令将 `0x08000000` 开始的 `0x10000`（64KB）内容保存到文件。

### 解锁芯片（被读保护锁住时）
```
device STM32F103C8
connect
unlock STM32F1
```

---

## 编译命令（回顾）

```bash
cmake --build <build_dir>
```

本项目用的是 CMake + Ninja 构建系统：
- 预设配置在 `CMakePresets.json` 中，定义了 Debug 和 Release 两种配置
- 工具链文件 `cmake/gnu-tools-for-stm32.cmake` 指定了 ARM GCC 交叉编译器
- 构建输出在 `build/Debug/` 下：
  - `first_code_STM32.elf` — ELF 可执行文件（含调试信息）
  - `first_code_STM32.hex` — Intel HEX 格式（用于烧录）
  - `first_code_STM32.bin` — 纯二进制格式
  - `first_code_STM32.map` — 内存映射文件（查看函数/变量地址）

如果 ninja 输出 `no work to do`，说明源文件没有变化，不需要重新编译。如果想强制重新编译，可以：
```bash
cmake --build build/Debug --clean-first
```
