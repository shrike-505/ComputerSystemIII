# ComputerSystemIII · Xpart

2024–2025 浙江大学《计算机系统 III》课程项目。本仓库自底向上贯通**硬件**与**软件**两个层次：在 FPGA / 仿真环境中实现一颗 64 位 RISC-V 处理器，并在其之上运行一个支持文件系统、进程管理与交互式 Shell 的操作系统内核。

---

## 项目总览

```
ComputerSystemIII/
├── submit/         硬件：五级流水线 RISC-V CPU（SystemVerilog）
├── kernel/         软件：基础内核（Sys III 实验框架）
├── kernel_XPart/   软件：扩展内核（VFS / FAT32 / ELF / Shell / 系统调用）
├── include/        共享头文件
├── slides/         结题展示幻灯片（reveal.js / Markdown）
├── Makefile        硬件仿真与 Vivado 综合的顶层构建脚本
└── README.md
```

整体设计分为两大部分：

| 部分 | 目录 | 关键能力 |
| --- | --- | --- |
| **硬件** | `submit/` | 64 位 RISC-V 处理器、Forwarding、分支预测、MMU（Sv39 三级页表） |
| **软件** | `kernel/`、`kernel_XPart/` | 11 个系统调用、缺页处理、ELF 加载、CoW `fork`、VirtIO + FAT32 文件系统、RIKEShell |

---

## 硬件部分

`submit/` 下为完整的五级流水线 RISC-V 处理器，采用 SystemVerilog 实现，可在 Verilator 仿真与 Vivado（Xilinx xc7a100t）综合两种流程下运行，并能直接启动软件部分的内核。

主要特性：

- **五级流水线**：取指 / 译码 / 执行 / 访存 / 写回。
- **Forwarding（数据前递）**：通过 `ForwardingUnit` / `ForwardingMux` 解决数据冒险（Sys II Lab2）。
- **分支预测**：在流水线中引入分支预测（Sys III Lab1）。
- **MMU**：实现 Sv39 三级页表地址翻译（Sys III Lab6），并处理「分支预测错误」与「页表遍历」之间的时序冲突——预测失败时等待 PC 恢复后再继续翻译。
- **CSR / 异常**：`CSRModule`、`CSRALU`、`ExceptReg`、`IDExceptExamine` 等模块支持机器态 CSR 与异常处理。
- **AXI 访存桥**：`Mem2Axi` 将核心访存请求桥接到 AXI 总线。

核心模块一览：

| 模块 | 职责 |
| --- | --- |
| `Core.sv` | 处理器顶层，组织各级流水线 |
| `Controller.sv` | 主控制器，生成各级控制信号 |
| `ALU.sv` / `Cmp.sv` | 算术逻辑运算与比较 |
| `RegFile.sv` | 通用寄存器堆 |
| `ImmGen.sv` | 立即数生成 |
| `ForwardingUnit.sv` / `ForwardingMux.sv` | 数据前递 |
| `HazardStall.sv` | 冒险检测与流水线暂停 |
| `MMU.sv` | Sv39 虚拟地址翻译 |
| `CSRModule.sv` / `CSRALU.sv` | CSR 读写与运算 |
| `ExceptReg.sv` / `IDExceptExamine.sv` | 异常寄存器与译码级异常检测 |
| `Mem2Axi.sv` | 访存到 AXI 的桥接 |

---

## 软件部分

软件部分包含两套内核：

- **`kernel/`** —— Sys III 实验基础框架，包含调度、虚拟内存、`printk` 等基础设施。
- **`kernel_XPart/`** —— 在基础内核之上扩展的完整内核，是软件部分的主体。

`kernel_XPart` 的主要能力：

#### 系统调用

共实现 **11 个系统调用**，采用与 Linux RISC-V 一致的调用号（`a7` 传号、`a0`–`a2` 传参，经 `ecall` 陷入 `syscall_handler` 统一分发，见 `trap.c`、`syscalls.h`）：

| 调用号 | 系统调用 | 说明 |
| --- | --- | --- |
| 23 | `dup3` | 复制文件描述符（用于 I/O 重定向，框架已就绪） |
| 56 | `open` | 打开 / 创建文件，返回 fd |
| 57 | `close` | 关闭文件描述符 |
| 62 | `lseek` | 调整读写偏移，支持 `SEEK_SET` / `SEEK_CUR` / `SEEK_END` |
| 63 | `read` | 从 fd 读取（fd 0 为标准输入，其余走文件系统） |
| 64 | `write` | 向 fd 写入（fd 1 为标准输出，其余走文件系统） |
| 93 | `exit` | 进程退出并回收资源 |
| 172 | `getpid` | 获取当前进程 PID |
| 174 | `waitpid` | 等待子进程退出并回收 |
| 220 | `clone` | `fork` 的底层实现（写时复制地址空间） |
| 221 | `execve` | 从文件系统加载并执行新程序 |

#### 进程与内存

- **缺页处理**：实现取指 / load / store 三类缺页（scause `0xc` / `0xd` / `0xf`）的处理，支持用户栈与代码段的按需调页。
- **`fork`（clone）**：基于写时复制（Copy-on-Write）复制父进程地址空间（`fork.c`）。
- **`execve`**：内建 **ELF 加载器**，从 FAT32 文件系统读入 ELF 可执行文件、建立用户地址空间映射并跳转执行（`execve.c`、`elf.h`，示例程序 `bin/hello.elf`）。
- **`waitpid` / `exit`**：父进程可阻塞等待指定子进程退出并回收其资源，构成进程从加载、运行到退出回收的完整生命周期。

#### 文件系统

自底向上三层结构：

- **VirtIO 块设备驱动**（`virtio.c`）：驱动 QEMU `virtio-blk-device`，并解析 MBR 分区表（`mbr.c`）。
- **FAT32 文件系统**（`fat32.c`）：支持文件的 **打开、读取、写入（可带偏移）、创建** 与目录项枚举（`fat32_open_file` / `fat32_read_file` / `fat32_write_file` / `fat32_create_file` / `get_fat32_filenames`）。
- **VFS 抽象层**（`fs.c`、`fs.h`）：以函数指针表（`lseek` / `read` / `write`）统一文件操作，每进程最多 **32 个打开文件**（`MAX_FILE_NUMBER`），路径长度上限 **32 字节**（`MAX_PATH_LENGTH`），支持 `F_READ` / `F_WRITE` / `F_EXEC` 权限位，写入数据**持久化**到磁盘镜像。

#### 用户态程序与 C 库

- **可运行真实用户程序**：用户程序通过 `user/src/main.c` 中的 `USER_MAIN` 宏选择，内置缺页测试（`PFH1`/`PFH2`）、`fork` / CoW / `malloc` 测试（`FORK1`–`FORK4`）、文件读写测试（`XPART`）以及默认的交互式 Shell（`SHELL`）；并可通过 `execve` 运行从文件系统加载的独立 ELF 程序。
- **RIKEShell**：交互式 Shell（致谢 ZJU `os24fall-stu`），支持 **6 条命令**：
  - `echo <content>` —— 输出文本（支持双引号包裹）
  - `cat <file>` —— 打印文件内容
  - `edit <file> <offset> <content>` —— 在指定偏移处写入文件
  - `run <elf>` —— 经 `fork` + `execve` 运行程序并 `waitpid` 等待其结束
  - `help` —— 显示命令帮助
  - `exit` —— 退出 Shell
- **用户态 C 库**：`malloc` / `free`（`malloc.c`）、`readline`（行编辑，`readline.c`）、`printf`、字符串函数等。

构建产物包括 `vmlinux`（ELF 内核）、`arch/riscv/boot/Image`（裸二进制）、`vmlinux.asm` 反汇编与 `System.map` 符号表。

---

## 构建与运行

### 软件：在 QEMU 上运行内核

依赖 `riscv64-linux-gnu-` 交叉工具链与 `qemu-system-riscv64`：

```bash
cd kernel_XPart

make all        # 交叉编译内核，生成 vmlinux / Image
make run        # 在 QEMU virt 平台上启动内核（挂载 disk.img 作为 VirtIO 磁盘）
make debug      # 同上，但开启 -S -s 等待 GDB 连接
make clean      # 清理构建产物
```

> 运行前请先将随仓提供的 `kernel_XPart/disk.img.zip` 解压为 `disk.img`（已在 `.gitignore` 中忽略），作为 FAT32 磁盘镜像。

此外还支持 **Spike** ISA 模拟器（`make spike_run` / `make spike_debug` / `make spike_bridge`）。

### 硬件：仿真与综合

顶层 `Makefile` 驱动硬件流程，依赖 **Verilator**（仿真）与 **Vivado**（综合 / 生成比特流）。其中 `DIR_UPSTREAM` 指向课程提供的 `sys-project` 工程（仿真平台、IP、测试用例等），请按实际路径配置。

```bash
make verilate        # Verilator 编译并运行测试用例（TESTCASE=sample）
make kernel          # 在仿真处理器上运行软件内核
make uart            # UART 外设测试
make conv            # 卷积测试用例
make wave            # 用 GtkWave 查看波形
make clean           # 清理仿真产物

make bitstream       # 调用 Vivado 综合生成比特流（需配置 VIVADO_SETUP 路径）
make vivado          # 打开 Vivado GUI
```

> 使用 Vivado 前，请在 `Makefile` 中将 `VIVADO_SETUP` 修改为本机 `settings64.bat` / `settings64.sh` 的实际路径；目标板默认为 `xc7a100tcsg324-1`。

---

## 展示幻灯片

`slides/` 为基于 reveal.js（Markdown 源 `main.md`）的结题展示，内容涵盖硬件与软件两部分的设计与运行结果。

```bash
cd slides
make            # 构建幻灯片
```

---

## 工具链一览

| 用途 | 工具 |
| --- | --- |
| 交叉编译 | `riscv64-linux-gnu-` GCC / LD / objcopy / objdump（`rv64ia_zicsr_zifencei`，`lp64`） |
| 软件运行 | QEMU（`qemu-system-riscv64`，virt 平台）、Spike |
| 硬件仿真 | Verilator、GtkWave |
| 硬件综合 | Vivado 2019.2+（Xilinx Artix-7 xc7a100t） |
| 幻灯片 | reveal.js / Markdown |