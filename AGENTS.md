如何用ai看懂代码和架构
1.询问目函数的目的，参数
2.询问处于架构的位置：调用了谁，被谁调用了
3.逐行行解析代码
4.同时在每行代码后面备注：枚举名称，宏例如CHNANNEL_1 ,NO_WAIT
5.最后了解信息流，即信息是如何从board->bsp->modules->threads

以下是ai总结踩过的坑和问题：
# AGENTS.md

## 1. 项目

RoboMaster C 板框架大作业。基于 STM32F407 + ThreadX，
补全 bsp / libs / modules / threads 四层框架，
实现单电机闭环控制与 BMI088 数据读取。

## 2. 架构分层

    第 3 层  threads/    什么时候做（线程调度）
    第 2 层  modules/    做什么（设备逻辑）
    第 1 层  libs/       怎么算（纯算法）
    第 0 层  bsp/        怎么和硬件说话
    -1 层    board/      硬件配置（CubeMX）

依赖方向只能【自上而下】。唯一例外：bsp_can 的接收回调
按框架设计需要回调 modules 层的 DJIMotorHandler。

## 3. 编码规范

- 每层只做一件事：线程管节奏、modules 管逻辑、bsp 管搬运
- 上层不碰寄存器，下层不认识业务
- 所有接收指针的函数先判 nullptr
- 数组下标使用前判范围
- 中断里只做「取数据 + 转交」，不做耗时运算
- 线程必须有 sleep，不能占死 CPU
- 参数尽量带单位（如 speedSet 为 rad/s，period_s 为秒）

## 4. 执行计划

    [x] CubeMX 配置（时钟/GPIO/PWM/UART/CAN/SPI/I2C/TIM/ThreadX）
    [x] CMake 接入 C++，加入 bsp/libs/modules/threads 源码
    [x] libs：PID 位置式 + 增量式 + 双限幅（电脑端验证）
    [x] bsp：CAN 收发（真机验证）
    [x] modules：DJIMotorHandler + M2006（电机闭环真机验证）
    [x] threads：control（1ms 速度环）
    [x] modules：BMI088 SPI 读写 + 三轴数据（真机验证）
    [x] threads：imu + alive（信号量监控 + 灯效）
    [x] bsp：PWM / USART 封装
    [ ] docs：chassis / spi / iic / logs / references
    [ ] 整体复验

## 5. 调试方法

- 从下往上验证：先让 bsp 通，再往上搭
- 一次只验证一个功能，改动后立即编译烧录
- 关键变量用串口打印实际值，不靠猜
- 出问题先缩小范围：CAN 通不通？数据是 0 还是乱？
- 每次调试记录到 docs/logs.md

## 6. 与 agent 的协作方式

### 我提要求的方式
- 先说明"我现在在做什么、卡在哪"，而不是直接要代码
- 要求解释原理，不接受"照抄就能跑"的答案
- 涉及新概念时要求先讲清楚，再给操作步骤

### agent 的使用边界
- 原理讲解、方案对比、错误定位：交给 agent
- 核心逻辑（PID、控制帧打包、SPI 协议）：自己写，
  写完让 agent 检查
- 文档：自己组织，agent 只做校对

### 有效的做法
- 让 agent 先给出"整体架构图 + 数据流"，再进入细节
- 每写完一个模块就让 agent 编译验证
- 要求 agent 指出「调用关系」和「谁调用了谁」，
  避免只看到局部

### 踩过的坑
- 参数在两处重复设置，后者静默覆盖前者
- maxIOut = 0 导致积分项被压死，Ki 设了等于没设
- 加速度计 SPI 读的第一个字节是无效的
- 调试时先怀疑硬件，实际上多数是软件配置问题