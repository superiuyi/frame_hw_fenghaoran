
## 10-06 开发环境搭建
**目标**:在 macOS + VSCode 上打通"编译 → 烧录"。
**遇到并解决的问题**:
1. **VSCode 找不到工具链**(ninja / arm-none-eabi-gcc)
   原因:macOS 从 Dock 启动的 GUI 程序不继承终端的 PATH。
   解决:安装 `code` 命令,改为从终端 `code .` 启动 VSCode。
2. **CMake 在错误的目录找工程**
   原因:仓库根目录残留一份坏的 CMakeLists.txt(课程作者把工程
   下沉到 board/ 时的遗留副本),`${sourceDir}` 解析成仓库根,
   导致 toolchainFile 路径不存在。
   解决:建 .vscode/settings.json,指定 "cmake.sourceDirectory"
   为 ${workspaceFolder}/board。
3. **settings.json 文件名写错**
   写成 setting.json(少一个 s),VSCode 静默忽略,不报任何错。
   教训:VSCode 配置文件名字是硬性的。
4. **没有 launch.json**
   解决:建 .vscode/launch.json,配置 Cortex-Debug + OpenOCD。
**结果**:VSCode 内可一键编译、F5 烧录并停在 main()。
首次烧录输出 `** Verified OK **`,MSP = 0x20020000 与链接脚本
的 RAM 顶端(128KB)一致。

## 10-06 串口通信出现的问题以及解决方案
C板上的晶振频率是12MHz我误以为是25MHz
导致验证usart的时候串口打印乱码

## 10-06 今日小结
理解了spi iic 和 usart的内在机理
解决了 vscode 环境配置 vscode. settings.json launch.json的问题 PATH的配置问题
实现了串口的验证
通过串口打印检查出

## 10-12 今日小结
坑 1  CAN_Transmit 缺 include(卡了两次)

坑 2  残留 openocd 占着 ST-Link → VSCode F5 报
      "GDB Server Quit Unexpectedly"
      解法:pkill -f openocd

坑 3  串口没输出 → 丝印 UART1/UART2 是反的

坑 4  rad/s 打印空白 → newlib-nano 默认不支持浮点 printf
      解法:链接加 -u _printf_float

坑 5  ★ PID 参数设了两处
      M2006.cpp 的 Tuning() 覆盖了 DJIMotor.hpp 的值

坑 6  ★ maxIOut = 0 让积分【静默失效】
      Ki 设了也白设,输出等于纯 P
      ← 踩了两次

坑 7  Kp = 0.5 / 3.0 太小
      err=5 → 输出 2 / 15,推不动电机
      改成 200 才行

坑 8  200f 不是合法浮点字面量,要写 200.0f