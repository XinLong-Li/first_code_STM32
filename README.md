# first_code_STM32 — STM32 寄存器级闪烁灯

> 基于 STM32F103C8T6 的裸机寄存器编程入门项目。
> 不依赖任何 HAL / CMSIS 库，纯手写寄存器实现 GPIOC13 闪烁。

---

## 1. 项目概览

| 项目 | 详情 |
|------|------|
| **目标芯片** | STM32F103C8T6（Cortex-M3，Max 72 MHz） |
| **Flash / RAM** | 64 KB Flash / 20 KB RAM |
| **开发板** | Blue Pill（或其他搭载 STM32F103C8T6 的板子） |
| **目标引脚** | PC13（Blue Pill 板载 LED） |
| **实现方式** | 纯寄存器编程，零 HAL / CMSIS 依赖 |
| **编程语言** | C11 + ARM 汇编（启动文件） |
| **工具链** | GNU Tools for STM32（`arm-none-eabi-gcc`） |
| **构建系统** | CMake + Ninja |
| **IDE** | STM32CubeIDE for VS Code Extension |
| **调试/烧录器** | ST-Link V2（克隆版可用） |

---

## 2. 目录结构

```
first_code_STM32/
├── .clangd                      # clangd 配置（指向 build/Debug/compile_commands.json）
├── .gitignore                   # 忽略 build/ 输出、IDE 配置等
├── CMakeLists.txt               # 主 CMake 构建文件（用户可修改）
├── CMakePresets.json            # CMake 预设：Debug / Release
├── stm32f103x8_flash.ld         # 链接脚本（Flash/RAM 布局 + 段定义）
├── Inc/
│   └── stm32f103xb.h            # 自制最小化寄存器头文件（RCC + GPIO）
├── Src/
│   ├── main.c                   # 主程序：SystemInit + 两阶段闪烁
│   ├── startup_stm32f103xx.S    # 启动汇编：向量表、Reset_Handler、中断弱定义
│   ├── syscall.c                # 系统调用桩（_write/_read 等，对接半主机）
│   └── sysmem.c                 # 堆管理（_sbrk 实现）
├── cmake/
│   ├── gnu-tools-for-stm32.cmake  # 工具链配置（编译器路径、CPU 参数）
│   └── vscode_generated.cmake     # STM32CubeIDE 自动生成的源文件列表
└── build/Debug/                   # Debug 构建产物
    ├── first_code_STM32.elf       # ELF 可执行文件（含调试信息）
    ├── first_code_STM32.hex       # Intel HEX 格式（串口 ISP 烧录用）
    ├── first_code_STM32.bin       # 纯二进制镜像（ST-Link 烧录用）
    └── first_code_STM32.map       # 内存映射文件
```

---

## 3. 硬件连接

### 3.1 芯片：STM32F103C8T6

```
                      STM32F103C8T6 (Blue Pill)
                   ┌────┐              ┌────┐
              VBAT │1   │          ←  │48  │ VDD
          PC13 ANT │2   │  板载LED ←  │47  │ VSS
     PC14 OSC32_IN │3   │              │46  │ PB9
    PC15 OSC32_OUT │4   │              │45  │ PB8
            PD0 XO │5   │              │44  │ BOOT0
         PD1 XI X2 │6   │              │43  │ PB7
             NRST  │7   │              │42  │ PB6
              VSSA │8   │              │41  │ PB5
              VDDA │9   │              │40  │ PB4
             PA0-W │10  │              │39  │ PB3
              PA1  │11  │              │38  │ PA15
              PA2  │12  │              │37  │ PA12
                    └────┘              └────┘
              PCB 背面：LED 连接 PC13 — 限流电阻 — 3.3V
```

- **PC13**：板载 LED（低电平点亮），本项目的闪烁目标引脚
- **BOOT0**：正常运行时接 GND；ISP 串口烧录时接 3.3V

### 3.2 ST-Link 接线（SWD 模式）

```
  ST-Link V2           Blue Pill
 ┌──────────┐        ┌──────────┐
 │ SWDIO  ──┼───────►│ SWIO / DIO│
 │ SWCLK  ──┼───────►│ SWCLK / CLK│
 │ GND    ──┼───────►│ GND       │
 │ 3.3V   ──┼──(可选)►│ 3.3V     │
 └──────────┘        └──────────┘
```

> 如果用**串口 ISP** 烧录（USB-TTL 模块）：TX → PA10(RX)，RX → PA9(TX)，GND → GND。
> 烧录前 BOOT0 接 3.3V（BOOT1 接 GND），烧完恢复 BOOT0 接 GND。

---

## 4. 开发环境搭建

### 4.1 安装工具

#### macOS

```bash
# 编译工具链
brew install arm-none-eabi-gcc

# STM32CubeIDE VS Code 扩展
# 在 VS Code 中安装：STM32CubeIDE for VS Code（STMicroelectronics）

# 开源 ST-Link 烧录工具（完美兼容克隆版）
brew install stlink

# GitHub CLI（可选，提交代码用）
brew install gh
```

#### 扩展版本信息

| 工具 | 版本（本机已验证） |
|------|------|
| STM32CubeIDE Build CMake | 1.45.0 |
| STM32CubeIDE Core | 1.3.0 |
| stlink | 1.8.0 |
| arm-none-eabi-gcc | Homebrew latest |

### 4.2 验证安装

```bash
arm-none-eabi-gcc --version
st-info --version
cube-cmake --version
```

---

## 5. 编译

### 快速编译

```bash
# 配置（仅初次）
cube-cmake --preset Debug

# 编译
cube-cmake --build build/Debug
```

### 产物说明

编译成功后，`build/Debug/` 下生成：

| 文件 | 格式 | 大小（参考） | 用途 |
|------|------|------|------|
| `first_code_STM32.elf` | ELF | ~30 KB | 含调试符号，**debug 用** |
| `first_code_STM32.bin` | 纯二进制 | ~932 B | **ST-Link 烧录用** |
| `first_code_STM32.hex` | Intel HEX | ~2.6 KB | **串口 ISP 烧录用** |
| `first_code_STM32.map` | 文本 | ~103 KB | 内存布局报告 |

**为什么 `.bin` 只有 932 字节？**
- 代码 928 字节（`main` + 启动 + 系统调用）
- 已初始化数据 4 字节
- BSS 段（未初始化数据 + 堆 + 栈）在 RAM 中占 1972 字节，不写入 Flash

### 内存占用

```
Memory region    Used    Total    %
RAM              1976 B   20 KB   9.65%
FLASH             932 B   64 KB   1.42%
```

---

## 6. 烧录

### 6.1 方式一：ST-Link（推荐，克隆版也支持）

```bash
# 烧录
st-flash write build/Debug/first_code_STM32.bin 0x08000000

# 查看目标芯片信息
st-info --probe
```

`0x08000000` 是 STM32F103 Flash 的起始地址。

#### 关于克隆版 ST-Link

市面上 10 元左右的 ST-Link V2 几乎都是克隆版。ST 官方工具（STM32CubeProgrammer / CubeIDE）会检测并拒绝克隆版，提示 `ST-LINK firmware upgrade required`。

**解决方案**：用开源工具 `stlink`（`brew install stlink`），完美兼容克隆版，功能完全一样。

```bash
# st-flash 对克隆版输出示例：
st-info --probe
# Found 1 stlink programmers
#   version:    V2J37S7
#   flash:      65536 (pagesize: 1024)
#   sram:       20480
#   chipid:     0x410
#   dev-type:   STM32F1xx_MD
```

### 6.2 方式二：串口 ISP（USB-TTL 模块）

```bash
# 1. 接线：TX → PA10, RX → PA9, GND → GND
# 2. BOOT0 接 3.3V，BOOT1 接 GND，上电
brew install stm32flash
stm32flash -w build/Debug/first_code_STM32.hex -v -g 0x0 /dev/tty.usbserial-*
# 3. 烧完断开 BOOT0（接回 GND），按复位或重上电
```

### 6.3 一键编译 + 烧录

```bash
cube-cmake --build build/Debug && st-flash write build/Debug/first_code_STM32.bin 0x08000000
```

---

## 7. 代码详解

### 7.1 寄存器头文件 `Inc/stm32f103xb.h`

```c
// ↓ STM32F103 内存映射
PERIPH_BASE     = 0x40000000    // 外设基址
APB2PERIPH_BASE = 0x40010000    // APB2 总线（GPIOA~G 在这里）
AHBPERIPH_BASE  = 0x40020000    // AHB 总线（RCC、DMA 在这里）

RCC_BASE        = 0x40021000    // 时钟控制器基址
GPIOC_BASE      = 0x40011000    // GPIOC 寄存器基址
```

定义了两个寄存器结构体：

- `RCC_TypeDef`：时钟控制寄存器（CR、CFGR、APB2ENR…）
- `GPIO_TypeDef`：GPIO 寄存器（CRL、CRH、ODR、BSRR、BRR…）

> **踩坑记录**：初版把 AHB 基址写成了 `0x40018000`（STM32F4 的偏移），导致 RCC 写到错误地址，时钟未使能，LED 不亮。STM32F1 的 AHB 起始于 `0x40020000`。

### 7.2 启动流程 `Src/startup_stm32f103xx.S`

```
上电/复位
  │
  ▼
CPU 从 0x08000000 读取 SP（初始栈指针）
CPU 从 0x08000004 读取 PC（Reset_Handler 地址）
  │
  ▼
Reset_Handler:
  1. 设置栈指针 SP = _estack（RAM 顶部）
  2. 调用 SystemInit()           ← 我们需要提供
  3. 复制 .data 段（Flash → SRAM）
  4. 清零 .bss 段
  5. 调用 __libc_init_array()    ← 静态构造
  6. 调用 main()
  7. 死循环（main 不应返回）
```

**为什么必须提供 SystemInit？**

启动文件 `startup_stm32f103xx.S` 第 59 行：`bl SystemInit`，显式调用了它。而第 417 行 `SystemInit` 被弱定义为 `Default_Handler`（死循环）。如果不覆盖，程序会在启动时就卡住，永远到不了 `main`。

> `bl` = Branch with Link（ARM 调用指令，等价于函数调用）

### 7.3 主程序 `Src/main.c`

```c
void SystemInit(void)
{
    // 空实现。HSI = 8 MHz 是复位后的默认时钟，够用。
}

int main(void)
{
    // ① 使能 GPIOC 时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;   // APB2ENR bit4 = 1

    // ② 配置 PC13 为推挽输出
    GPIOC->CRH &= ~(0xFU << 20);   // 清除 PC13 的 4 个配置位
    GPIOC->CRH |=  (0x1U << 20);   // CNF[1:0]=00, MODE[1:0]=01

    // ③ 两阶段闪烁循环
    while (1) {
        // 慢闪 5 次（1s 间隔）
        // 快闪 20 次（100ms 间隔）
        // 循环往复
    }
}
```

#### GPIO 配置位详解（CRH 寄存器）

CRH 控制引脚 8 ~ 15。每个引脚占 4 位：`[CNF1:CNF0:MODE1:MODE0]`。

PC13 对应 CRH 的位 [23:20]：

| 字段 | 位 | 配置值 | 含义 |
|------|------|------|------|
| CNF[1:0] | [23:22] | `00` | 通用推挽输出（General purpose push-pull） |
| MODE[1:0] | [21:20] | `01` | 最大输出速度 10 MHz |

```
PC13 在 CRH 中的位置：      CNF  MODE
CRH: [31:28][27:24][23:22][21:20][19:16][15:12][11:8][7:4][3:0]
       PC15   PC14   ◄ PC13 ►  PC12   PC11   PC10  PC9   PC8
```

STM32 的 GPIO 默认是浮空输入（`CNF=01, MODE=00`），**必须手动配置为输出**引脚才能驱动 LED。

#### 为什么必须先开时钟？

STM32 为了**省电**，所有外设时钟默认关闭。如果不使能 GPIOC 的时钟（`RCC_APB2ENR_IOPCEN`），对 GPIOC 寄存器的读写可能无效或导致总线错误。这是一个非常常见的坑。

| 外设 | 总线 | 使能位 |
|------|------|------|
| GPIOA~GPIOC | APB2 | RCC_APB2ENR bit2~4 |
| USART1 | APB2 | RCC_APB2ENR bit14 |
| TIM2~TIM4 | APB1 | RCC_APB1ENR bit0~2 |

#### 引脚翻转

```c
GPIOC->BSRR = (1UL << 13);   // BSRR bit13 = 1 → ODR13 置 1（引脚高电平）
GPIOC->BRR  = (1UL << 13);   // BRR  bit13 = 1 → ODR13 清 0（引脚低电平）
```

- **BSRR**（Bit Set/Reset Register）：写 1 到低 16 位的某位 = 置位对应 ODR；写 1 到高 16 位 = 复位对应 ODR
- **BRR**（Bit Reset Register）：写 1 到某位 = 复位对应 ODR（BSRR 高 16 位的简化版）

#### 延时函数

```c
static void delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    while (ms--) {
        for (i = 0; i < 2000; ++i) __asm("nop");
    }
}
```

- 系统时钟 HSI = 8 MHz（125 ns / 周期）
- 内层循环约 4 个指令周期 → ~0.5 µs/次
- 2000 次 × 0.5 µs ≈ 1 ms
- `volatile` 防止编译器优化掉循环

> ⚠️ 这是一种**粗略估算**，实际延时会因编译优化等级、Flash 等待周期等因素偏差。精确延时请使用 SysTick 定时器。

### 7.4 其他源文件

| 文件 | 职责 |
|------|------|
| `sysmem.c` | 提供 `_sbrk()` — malloc/new 的堆内存分配 |
| `syscall.c` | 提供 `_write()`/`_read()` 等 — printf/scanf 的底层桩 |

---

## 8. STM32F103 内存映射速查

```
0x0800 0000 ─►┌─────────────┐
               │   Flash      │  64 KB（用户代码 + 常量）
0x0801 0000 ─►└─────────────┘

0x1FFF F000 ─►┌─────────────┐
               │   System     │  4 KB（出厂 Bootloader）
0x1FFF FFFF ─►└─────────────┘

0x2000 0000 ─►┌─────────────┐
               │   SRAM       │  20 KB（变量 + 堆 + 栈）
0x2000 5000 ─►└─────────────┘
               │   _estack    │  ← 栈顶（SP 初值）

0x4000 0000 ─►┌─────────────┐
               │   APB1       │  外设：TIM2~TIM7, UART2~5, I2C, SPI2/3…
0x4001 0000 ─►├─────────────┤
               │   APB2       │  外设：GPIOA~G, AFIO, EXTI, TIM1, USART1, SPI1, ADC…
0x4002 0000 ─►├─────────────┤
               │   AHB        │  外设：RCC, DMA, Flash 控制器, CRC
0xE000 E000 ─►├─────────────┤
               │   Cortex-M3  │  系统控制块（SCB）、SysTick、NVIC
               └─────────────┘
```

---

## 9. LED 不亮？常见排查

| 现象 | 可能原因 | 解决方案 |
|------|------|------|
| LED 完全不亮 | GPIOC 时钟未使能（RCC 基址错误） | 检查 `stm32f103xb.h` 中 `AHBPERIPH_BASE` 是否为 `0x40020000` |
| LED 常亮不灭 | 引脚仍为输入模式 | 检查 CRH bit[21:20] 是否配置为 01（输出） |
| LED 不亮 | LED 是低电平点亮 | 确认 `BSRR` 置位 / `BRR` 清位顺序正确 |
| ST-Link 无法连接 | 克隆版被官方工具拒绝 | 使用 `st-flash` 替代 STM32CubeProgrammer |
| 程序卡死不运行 | 缺少 SystemInit 定义 | 添加 `void SystemInit(void) {}` |

---

## 10. License

MIT

---

> 🛠 Built with STM32CubeIDE for VS Code Extension, GNU Tools for STM32, and open-source stlink tools.
