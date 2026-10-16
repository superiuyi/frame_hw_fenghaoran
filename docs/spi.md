我的思考：

双工
    全双工：双方可以同时收发
    半双工：同一时刻只能一个方向传
    UART:RXTX
    SPI:MISO MOSI
    CAN:CANH CANL只有一条主线
    IIC：
理解：
    CS片选：通道，拉低就开启->一主多从
    CLK：同步
为啥用？
    快：全双工，同步，具体机理
    IMU要灵敏

AI总结：
# SPI 学习记录

## 1. 全双工与半双工

### 1.1 定义
    全双工：双方可以【同时】收发
    半双工：同一时刻只能一个方向传

### 1.2 判断方法
    看有几套独立的数据通道

### 1.3 对比表
    | 类型   | 数据线 | 能否同时收发 | 例子 |
    | 全双工 | 2 根   | 能           | UART / SPI |
    | 半双工 | 1 根   | 不能         | I²C / CAN |

## 2. SPI 是什么

### 2.1 全称
    Serial Peripheral Interface（串行外设接口）

### 2.2 四根线
    SCK   时钟（主机产生）
    MOSI  主机 → 从机
    MISO  从机 → 主机
    CS    片选

### 2.3 工作原理
    主机打一拍 SCK，双方各交换一位
    打 8 拍 = 交换一个字节

### 2.4 特点
    同步（有专门的时钟线）
    全双工
    一主多从，靠片选区分
    速度高（可达几十 MHz）
    没有应答机制（不知道对方收到没）
    占引脚多

## 3. SPI 的四种模式

    CPOL：时钟空闲时的电平（0=低，1=高）
    CPHA：在第几个边沿采样（0=第一个，1=第二个）

    | Mode | CPOL | CPHA |
    | 0    | 0    | 0    |
    | 1    | 0    | 1    |
    | 2    | 1    | 0    |
    | 3    | 1    | 1    |

## 4. 本项目的配置过程

### 4.1 为什么用 SPI
    BMI088 支持 SPI 和 I²C
    → 陀螺仪要 2000Hz 采样，需要 112kbps
    → SPI 10MHz 余量大；且片选能把 ACC/GYRO 分开

### 4.2 CubeMX 配置
    Connectivity → SPI1 → Mode: Full-Duplex Master
    参数：
        Data Size       = 8 Bits
        Clock Polarity  = High      （CPOL=1）
        Clock Phase     = 2 Edge    （CPHA=1）→ Mode 3
        NSS             = Software
        Prescaler       = 16        → 84MHz/16 = 5.25MHz
        First Bit       = MSB First

    GPIO：
        PB3 → SPI1_SCK
        PA7 → SPI1_MOSI
        PB4 → SPI1_MISO
        PA4 → GPIO_Output（CS_ACC，初始高）
        PB0 → GPIO_Output（CS_GYRO，初始高）

### 4.3 验证
    读 BMI088 芯片 ID：
        ACC  → 0x1E ✅
        GYRO → 0x0F ✅

### 4.4 踩的坑
    · 加速度计读回来的第 1 个字节是【无效的】，要丢掉
    · 片选必须手动拉低/拉高（NSS 配成了 Software）