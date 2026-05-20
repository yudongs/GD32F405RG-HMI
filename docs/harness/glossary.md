# Glossary（通用语言 / Ubiquitous language）

> 领域驱动设计中的 **ubiquitous language**：需求、代码命名、评审与 AI 提示应使用同一套术语。术语变更须 **先改本表，再改实现与文档**。
> 对应约束：`ubiquitous-language`。

## 原则

- 每个业务词在本表中有唯一主条目；禁止在同一概念上用多套中英文混称。
- PR / 需求中出现新词时，先在本表登记或显式引用已有条目。
- `forbidden_synonyms` 下列出的词不得再在对外 API、用户文案与持久化字段中引入。

## 术语表

| `BSP` | Board Support Package，板级支持包，提供芯片/开发板级初始化和硬件抽象 | 架构主程 | N/A | `Board Support Library`、`板级支持` |
| `CMSIS` | Cortex Microcontroller Software Interface Standard，ARM 提供的 Cortex-M 微控制器软件接口标准，包含内核头文件和启动文件 | ARM/GigaDevice | N/A | — |
| `StdPeriph Driver` | GD32 标准外设驱动库，位于 `GD32_StdPeriph_Driver/`，封装了 MCU 外设（ADC/CAN/GPIO 等）的寄存器操作 | 固件主程 | 驱动层模块 | `Peripheral Library`、`外设库` |
| `RCU` | Reset and Clock Unit，复位与时钟单元，负责 MCU 各外设的时钟门控和复位控制 | 固件主程 | 驱动层模块 | `Clock Control`、`时钟控制` |
| `FMC` | Flash Memory Controller，Flash 存储器控制器，管理内部 Flash 的读写和选项字节 | 固件主程 | 驱动层模块 | `Flash Controller` |
| `GPIO` | General Purpose Input/Output，通用输入输出端口，用于配置引脚功能（输入/输出/复用/中断） | 固件主程 | 驱动层模块 | — |
| `ADC` | Analog-to-Digital Converter，模数转换器，用于采集模拟信号 | 固件主程 | 驱动层模块 | `A/D Converter` |
| `USART` | Universal Synchronous/Asynchronous Receiver/Transmitter，通用同步/异步收发器，串口通信外设 | 固件主程 | 驱动层模块 | `串口`、`UART` |
| `SPI` | Serial Peripheral Interface，串行外设接口，用于高速板级通信 | 固件主程 | 驱动层模块 | — |
| `I2C` | Inter-Integrated Circuit，双线串行总线，用于连接传感器和外围芯片 | 固件主程 | 驱动层模块 | `IIC` |
| `DMA` | Direct Memory Access，直接内存访问，用于在外设和内存之间高速传输数据无需 CPU 介入 | 固件主程 | 驱动层模块 | — |
| `中断向量表` | Interrupt Vector Table，存放各中断服务程序入口地址的表，位于 Flash 起始地址 | 固件主程 | 驱动层模块 | `Vector Table`、`中断表` |
| `FLM` | Flash Layout Manager，Keil Flash 算法文件，描述目标芯片的 Flash 擦写算法 | Keil/GigaDevice | N/A | `Flash Algorithm` |
| `HEX` | Intel HEX 格式文件，编译产物之一，包含可执行的机器码和地址信息 | 编译输出 | N/A | `Hex File` |
| `AXF` | ARM Executable File，Keil 编译输出的 ELF/DWARF 格式调试文件，含符号表 | 编译输出 | N/A | `AXF File`、`ELF File` |
| `JTAG/SWD` | Joint Test Action Group / Serial Wire Debug，芯片调试接口，用于下载程序和实时调试 | 硬件/调试 | N/A | `调试接口`、`调试器` |
| `MKLink` | 烧录调试工具，通过 SWD 接口实现烧录、RTT、寄存器读写等功能 | 工具 owner | N/A | — |

## 维护者与变更流程

- **Owner**：项目主程（架构和固件负责人）
- **变更流程**：
  1. 先更新本 glossary。
  2. 同 PR 更新代码命名、对外 API 与需求文档。
  3. 通知依赖方或更新 `decision_log.md`。

## 已知术语漂移（待消除）

- 暂无记录。发现口语/文案与代码不一致时在此登记。
