
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
## 10-13 BMI088 驱动调试
坑 1  三轴数据全是 0
      原因:ReadReg() 末尾残留在框架里的"清零缓冲区"占位代码,
            把刚读到的数据又清成了 0
      解法:删掉那两行

坑 2  陀螺仪正常,加速度计恒为 0
      原因:① WriteReg 里照框架注释多写了一个 dummy 字节,
               但 BMI088 的写操作【不需要】dummy(只有读才要)
            ② Config() 里陀螺仪软重启的延时把加速度计的
               电源控制寄存器冲掉了
      解法:删掉 dummy 字节
            Config() 之后补写 PWR_CTRL(0x7D)=0x04 和 PWR_CONF(0x7C)=0x00

坑 3  if (!imu.self_test.INIT_ERR) 永远不成立,初始化信号量放不出去
      原因:框架只在【失败】时把 INIT_ERR 置 true,成功时【不清零】
      解法:改用 ACC_CHIP_ID_ERR 和 GYRO_CHIP_ID_ERR 两个标志判断

坑 4  启动要等约 8 秒
      原因:Calibrate() 被调了两次,每次 4000 次采样 × 1ms
      解法:只调一次

## 10-14 三个线程
坑 1  红灯绿灯都不亮
      原因:AliveThread_Init() 忘了在 App_ThreadX_Init() 里调用
      解法:补上调用

坑 2  以为 alive 在闪蓝灯,其实是别的线程在闪
      原因:app_threadx.c 里框架自带的 test_thread 也在闪 PH10
      解法:注释掉 test_thread 的创建

坑 3  alive 主循环不执行
      原因:测试代码里留了一个 while(1) 死循环
      解法:删掉

坑 4  tx_semaphore_get 用 TX_NO_WAIT 取走后没放回
      → 计数被吃掉,下一次判断永远失败
      解法:取走后再 put 回去,保持计数不变

## 10-15 bsp_pwm / bsp_usart 封装
坑 1  PWM_SetPeriod 算出来的 ARR 差 1
      原因:PSC 从 0 开始数,实际分频倍数 = PSC + 1
      解法:arr = period_s * tim_clk / (psc + 1) - 1

坑 2  PWM_SetDutyRatio 占空比总差一点
      原因:定时器从 0 数到 ARR,一共 ARR + 1 个刻度
      解法:ccr = dutyratio * (arr + 1)

## 10-16 底盘解算
坑 1  把"投影到【辊子轴】方向"错说成"投影到【垂直于辊子】的方向"
      原因:约束的物理来源是"辊子不能沿自己的轴滑动",
            所以【接触点沿辊子轴方向的速度必须为 0】
      解法:投影方向 = 辊子轴方向 û

      用三个物理情形验算公式:
          纯前进 → 四轮同速
          纯自转 → 左轮反、右轮正
          纯左移 → 对角同向(FL/RR 同向,FR/RL 同向)

坑 2  √2 的来历
      辊子轴 û 与轮毂滚动方向 ê 差 45°
      → ê·û = 1/√2,在约束方程里出现
      → 分子分母各一次,最终公式里抵消

      麦轮:  ω·r = (v·û)/cos45° = √2(v·û) → 抵消,结果无 √2
      全向轮:ω·r = (v·û)/cos0°  = v·ê    → 不抵消,结果多一个 1/√2

坑 3  ω 的正负号
      本工程约定:ω > 0 = 逆时针(俯视)
      但 RM 官方代码里 yaw 是顺时针增大,两者相反
      解法:全项目统一约定,并在注释里写死