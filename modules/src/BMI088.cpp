#include "BMI088.hpp"
#include "tx_api.h"
#include "bsp_pwm.hpp"
#include <cmath>
#include "spi.h"        // ← 加:hspi1
#include "main.h"       // ← 加:CS_ACC_Pin / CS_ACC_GPIO_Port

namespace BMI088
{
    /**
     * @brief BMI088 acc gyro 标定
     * @note 标定后的陀螺仪零偏存储在 Gyro_offset 中。
     * @attention 不管工作模式是blocking还是IT,标定时都是blocking模式,所以不用担心中断关闭后无法标定(RobotInit关闭了全局中断)
     * @attention 标定精度和等待时间有关。
     * @todo 将标定次数(等待时间)变为参数供设定
     * @section 整体流程为1.累加加速度数据计算gNrom()
     *                   2.累加陀螺仪数据计算零飘
     *                   3. 如果标定过程运动幅度过大,重新标定
     *                   4.保存标定参数
     */
    void cBMI088::Calibrate()
    {
        const int calib_samples = 4000; // 采样次数
        float gyro_sum[3] = {0.0f, 0.0f, 0.0f};
        gyro_data_t temp_gyro;

        // 1. 清除旧的 Offset，防止叠加
        Gyro_offset[0] = 0.0f;
        Gyro_offset[1] = 0.0f;
        Gyro_offset[2] = 0.0f;

        // 2. 循环采样
        for (int i = 0; i < calib_samples; i++)
        {
            ReadGyroData(&temp_gyro); // 这里读取的是原始值（因为Offset已清零）
            gyro_sum[0] += temp_gyro.x;
            gyro_sum[1] += temp_gyro.y;
            gyro_sum[2] += temp_gyro.z;
            
            tx_thread_sleep(1); // 等待一个 ThreadX tick；实际耗时由 tick 频率决定。
        }

        // 3. 计算平均值作为零偏
        Gyro_offset[0] = gyro_sum[0] / calib_samples;
        Gyro_offset[1] = gyro_sum[1] / calib_samples;
        Gyro_offset[2] = gyro_sum[2] / calib_samples;
        
        // 如果零偏过大（例如超过 0.1 rad/s），可能是在运动中标定的，应报错或丢弃
        if (fabs(Gyro_offset[0]) > 0.1f || fabs(Gyro_offset[1]) > 0.1f || fabs(Gyro_offset[2]) > 0.1f)
        {
            self_test.CALIBRATE_ERR = true;
            // 恢复默认值或保留上次值
            Gyro_offset[0] = BMI088_GYRO_PRE_CALI_OFFSET_X; 
            Gyro_offset[1] = BMI088_GYRO_PRE_CALI_OFFSET_Y;
            Gyro_offset[2] = BMI088_GYRO_PRE_CALI_OFFSET_Z;
        }
        else
        {
            self_test.CALIBRATE_ERR = false;
        }
    }

    void cBMI088::TemperatureControl(float target_temp)
    {
        // TODO: 温度读取完成后，用 TempPid 计算加热占空比并限制到 [0, 1]。
        // 未完成前保持加热关闭。
        (void)target_temp;
          // ① 读当前温度
    float temp = 0.0f;
    ReadAccTemperature(&temp);
    // ② 喂给 PID
    TempPid.ref = target_temp;     // 目标温度(比如 40℃)
    TempPid.fdb = temp;            // 当前温度
    TempPid.UpdateResult();        // 算一次
    // ③ PID 输出范围是 [-maxOut, +maxOut] = [-25000, +25000]
    //    要映射成占空比 [0, 1]
    float duty = TempPid.result / TempPid.maxOut;
    //           ↑              ↑ 25000.0f
    // ④ 限幅
    if (duty < 0.0f) duty = 0.0f;   // 温度超了 → 不加热
    if (duty > 1.0f) duty = 1.0f;
    // ⑤ 写进 PWM
        PWM_SetDutyRatio(&HEATING_RESISTANCE_TIM, 0.0f, HEATING_RESISTANCE_CHANNEL);
   
    }

    void cBMI088::VerifyAccChipID()
    {
        uint8_t pRxData[2]; //< 读取两个字节，第一个是需要丢弃的无效字节，第二个是芯片 ID

        ReadReg(BMI088_CS_ACC, ACC_CHIP_ID_ADDR, pRxData, 2); //< 读取加速度计chip id
        tx_thread_sleep(1);
        //< 如果chip id不等于预设值,则加速度计ID错误,初始化错误
        if (pRxData[1] != ACC_CHIP_ID_VAL)
        {
            self_test.ACC_CHIP_ID_ERR = true;
            self_test.INIT_ERR = true;
        }
        else if (pRxData[1] == ACC_CHIP_ID_VAL)
        {
            self_test.ACC_CHIP_ID_ERR = false;
        }
    }

    void cBMI088::VerifyGyroChipID()
    {
        uint8_t pRxData;                                                 //< 读取一个字节,chip id
        ReadReg(BMI088_CS_GYRO, GYRO_CHIP_ID_ADDR, &pRxData, 1); //< 读取陀螺仪chip id
        tx_thread_sleep(1);
        //< 如果chip id不等于预设值,则陀螺仪ID错误,初始化错误
        if (pRxData != GYRO_CHIP_ID_VAL)
        {
            self_test.GYRO_CHIP_ID_ERR = true;
            self_test.INIT_ERR = true;
        }
        else if (pRxData == GYRO_CHIP_ID_VAL)
        {
            self_test.GYRO_CHIP_ID_ERR = false;
        }
    }

    void cBMI088::VerifyAccData()
    {
        // TODO: 检查加速度数据是否有效，并更新 ACC_DATA_ERR。
             uint8_t buf[2];
    ReadReg(BMI088_CS_ACC, ACC_ERR_REG_ADDR, buf, 2);
    //                    ↑ 0x02
    //   为什么读 2 字节?
    //   因为【加速度计读操作】第一个字节是废字节
    //   buf[0] = 废   buf[1] = 真正的寄存器值
    //   (和 VerifyAccChipID 一模一样)
    if ((buf[1] & 0xC0) != 0)
    //            ↑ 0xC0 = 1100 0000
    //              同时检查 bit7 和 bit6
    //   & 之后不等于 0 → 说明有一位是 1 → 有错误
    {
        self_test.ACC_DATA_ERR = true;
    }
    else
    {
        self_test.ACC_DATA_ERR = false;
    }    
    }

    void cBMI088::VerifyGyroData()
    {
        // TODO: 检查角速度数据是否有效，并更新 GYRO_DATA_ERR。
            uint8_t buf;
    ReadReg(BMI088_CS_GYRO, GYRO_INT_STAT_1_ADDR, &buf, 1);
    //                       ↑ 0x0A
    //   陀螺仪读 1 字节,【没有废字节】
    //   buf 是单个变量,要取地址 &
    if ((buf & 0x80) != 0)
    //         ↑ 0x80 = 1000 0000,只看 bit7
    {
        self_test.GYRO_DATA_ERR = false;   // 有新数据 → 正常
    }
    else
    {
        self_test.GYRO_DATA_ERR = true;    // 没数据 → 异常
    }
    }

    void cBMI088::WriteReg(enum BMI088_SENSOR cs, uint8_t addr, uint8_t *data, uint8_t len)
    {
        // TODO: 根据 cs 拉低对应 GPIO 片选；按 SPI 写协议发送地址和数据，
        // 最后释放片选，并处理通信失败与必要的延时。
         // ---------------------------------------------------------
        // 【准备阶段】把要发的字节拼进一个缓冲区
        // ---------------------------------------------------------
        uint8_t tx[8] = {0};      // 发送缓冲区,8字节够用,先全填 0
        uint8_t n = 0;            // 计数器:记录往 tx 里放了几字节
        // ---- 第 1 个字节:命令字节(地址 + 写标志)----
        tx[n++] = addr & BMI088_SPI_WRITE_CODE;//要求bit7是0
        //       ↑         ↑
        //   放到 tx[0]   addr & 0x7F,把 bit7 清成 0 = "这是写操作"
        //   n 变成 1
        //   例:addr=0x7E → 0x7E & 0x7F = 0x7E
        // ---- 第 2 个字节(只有加速度计需要)----
        // if (cs == BMI088_CS_ACC)
        //     tx[n++] = 0x00;
        //           ↑
        //      这个 0x00 是【dummy 字节】
        //      BMI088 的加速度计在 SPI 写的时候,地址后面必须多跟一个字节
        //      这是芯片硬件特性,datasheet 里写的
        //      n 变成 2
        //      陀螺仪不需要,n 保持 1
        // ---- 第 3 个字节起:真正要写的数据 ----
        for (uint8_t i = 0; i < len; i++)
            tx[n++] = data[i];
        //  把调用者给的 data[0..len-1] 依次放进 tx
        //  例:len=1, data[0]=0xB6 → tx[2]=0xB6, n=3
        // ---------------------------------------------------------
        // 【发送阶段】拉低片选 → 发 → 拉高片选
        // ---------------------------------------------------------
        if (cs == BMI088_CS_ACC)
        {
            HAL_GPIO_WritePin(CS_ACC_GPIO_Port, CS_ACC_Pin, GPIO_PIN_RESET);
            //                 GPIOA             PA4        拉低(0V)
            //  作用:选中加速度计,"我要跟你说话了"
            //  此时陀螺仪的片选还是高 → 它保持安静
            HAL_SPI_Transmit(&hspi1, tx, n, 100);
            //                 ↑     ↑   ↑   ↑
            //             用SPI1  发tx 发n个 超时100ms
            //  硬件自动:在 SCK 上打 n×8 个节拍,把 tx 逐位发出去
            HAL_GPIO_WritePin(CS_ACC_GPIO_Port, CS_ACC_Pin, GPIO_PIN_SET);
            //                                            拉高(3.3V)
            //  作用:释放,"说完了"
        }
        else
        {
            // 陀螺仪,同理,只是换成 PB0
            HAL_GPIO_WritePin(CS_GYRO_GPIO_Port, CS_GYRO_Pin, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, tx, n, 100);
            HAL_GPIO_WritePin(CS_GYRO_GPIO_Port, CS_GYRO_Pin, GPIO_PIN_SET);
        }
    }

    void cBMI088::ReadReg(enum BMI088_SENSOR cs, uint8_t addr, uint8_t *data, uint8_t len)
    {
        // TODO: 根据 cs 拉低对应 GPIO 片选；按 SPI 读协议发送地址，
        // 丢弃加速度计返回的首个无效字节，读取 len 字节并释放片选。
        // 未实现前清零缓冲区，避免芯片 ID 校验读取未初始化数据。
        // ---------------------------------------------------------
        // 【防空指针】data 是调用者给的缓冲区,不能是空的
        // ---------------------------------------------------------
        if (data == nullptr || len == 0) return;
        //  len==0 就没必要读了,直接返回
        // ---------------------------------------------------------
        // 【第 1 步】拼命令字节
        // ---------------------------------------------------------
        uint8_t cmd = addr | BMI088_SPI_READ_CODE;
        //             ↑           ↑
        //        寄存器地址    0x80 = 1000 0000
        //        按位或之后,bit7 一定变成 1 → 表示"读操作"
        //  例:addr=0x00 → 0x00 | 0x80 = 0x80
        // ---------------------------------------------------------
        // 【第 2 步】拉片选 → 发地址 → 收数据 → 拉片选
        // ---------------------------------------------------------
        if (cs == BMI088_CS_ACC)
        {
            HAL_GPIO_WritePin(CS_ACC_GPIO_Port, CS_ACC_Pin, GPIO_PIN_RESET);
            //                 GPIOA             PA4        拉低
            //  选中加速度计
            HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
            //                 ↑      ↑   ↑   ↑
            //              用SPI1  发cmd 1个 超时100ms
            //  ⚠️ &cmd:cmd 是【单个变量】,要取地址
            //     (对比 WriteReg 的 tx 是数组,不用 &)
            //  作用:把命令字节推出去 → 芯片知道"你要读 0xXX"
            HAL_SPI_Receive(&hspi1, data, len, 100);
            //                ↑      ↑    ↑    ↑
            //             用SPI1  放哪  收len个 超时
            //  data 是调用者给的数组指针,不用 &
            //  作用:把芯片吐出来的 len 个字节收进 data
            HAL_GPIO_WritePin(CS_ACC_GPIO_Port, CS_ACC_Pin, GPIO_PIN_SET);
            //                                            拉高
            //  释放,通信结束
        }
        else
        {
            // 陀螺仪,同理,换成 PB0
            HAL_GPIO_WritePin(CS_GYRO_GPIO_Port, CS_GYRO_Pin, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
            HAL_SPI_Receive (&hspi1, data, len, 100);
            HAL_GPIO_WritePin(CS_GYRO_GPIO_Port, CS_GYRO_Pin, GPIO_PIN_SET);
        }
    }

    void cBMI088::Config()
    {
        tx_thread_sleep(10); //< 等待系统稳定

        /*-------------------------------------加速度计初始化-------------------------------------*/
        
        //< 先软重启，清空所有寄存器
        uint8_t pTxData;
        pTxData = ACC_SOFTRESET_VAL;
        WriteReg(BMI088_CS_ACC, ACC_SOFTRESET_ADDR, &pTxData, 1);
        tx_thread_sleep(100); //< 延时100ms,重启需要时间

        //< 打开加速度计电源
        pTxData = ACC_PWR_CTRL_ON;
        WriteReg(BMI088_CS_ACC, ACC_PWR_CTRL_ADDR, &pTxData, 1);
        tx_thread_sleep(150);

        //< 加速度计变成正常模式
        pTxData = ACC_PWR_CONF_ACT;
        WriteReg(BMI088_CS_ACC, ACC_PWR_CONF_ADDR, &pTxData, 1);
        tx_thread_sleep(10); //

        //< 测量范围
        pTxData = ACC_RANGE_6G;
        WriteReg(BMI088_CS_ACC, ACC_RANGE_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0xAB;
        WriteReg(BMI088_CS_ACC, ACC_CONF_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x08;
        WriteReg(BMI088_CS_ACC, INT1_IO_CTRL_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x04;
        WriteReg(BMI088_CS_ACC, INT_MAP_DATA_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        /*-------------------------------------陀螺仪初始化-------------------------------------*/
        //< 先软重启，清空所有寄存器
        pTxData = GYRO_SOFTRESET_VAL;
        WriteReg(BMI088_CS_GYRO, GYRO_SOFTRESET_ADDR, &pTxData, 1);
        tx_thread_sleep(100); //< 延时100ms,重启需要时间

        pTxData = GYRO_RANGE_2000_DEG_S;
        WriteReg(BMI088_CS_GYRO, GYRO_RANGE_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = GYRO_ODR_2000Hz_BANDWIDTH_230Hz | GYRO_LPM1_SUS;
        WriteReg(BMI088_CS_GYRO, GYRO_BANDWIDTH_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = GYRO_LPM1_NOR;
        WriteReg(BMI088_CS_GYRO, GYRO_LPM1_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = GYRO_DRDY_ON;
        WriteReg(BMI088_CS_GYRO, GYRO_INT_CTRL_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x00;
        WriteReg(BMI088_CS_GYRO, GYRO_INT3_INT4_IO_CONF_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x01;
        WriteReg(BMI088_CS_GYRO, GYRO_INT3_INT4_IO_MAP_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms
    }


    void cBMI088::ReadAccData(acc_data_t *data)
    {
        // TODO: 按 BMI088 加速度计 SPI 协议丢弃首个无效字节，拼接三轴有符号原始值，
        // 再按配置的量程换算为 m/s²，写入 data。
            // data 是调用者给的输出结构体,不能是空
    if (data == nullptr) return;
    // ---------------------------------------------------------
    // 【第 1 步】一次性读 6 个数据字节 + 1 个废字节 = 7 个
    // ---------------------------------------------------------
    uint8_t buf[7];
    ReadReg(BMI088_CS_ACC, ACC_X_LSB_ADDR, buf, 7);
    //                    ↑ 0x12    ↑ 从 X 轴低字节开始,连读 7 个
    // 收回来的是:
    //   buf[0] = 废字节(ACC 特有)
    //   buf[1] = X_L   buf[2] = X_H
    //   buf[3] = Y_L   buf[4] = Y_H
    //   buf[5] = Z_L   buf[6] = Z_H
    // ---------------------------------------------------------
    // 【第 2 步】两个字节拼成一个 int16
    // ---------------------------------------------------------
    int16_t rx = (int16_t)(buf[2] << 8 | buf[1]);
    //                   ↑        ↑
    //            高字节左移8位   低字节
    //   例:buf[2]=0x10, buf[1]=0x6C
    //       0x10 << 8 = 0x1000
    //       0x1000 | 0x6C = 0x106C = 4204
    //
    //   ⚠️ (int16_t) 必须加:加速度可能是负的(板子倒过来)
    //      不加的话 0xFFFF 会被当成 65535 而不是 -1
    int16_t ry = (int16_t)(buf[4] << 8 | buf[3]);
    int16_t rz = (int16_t)(buf[6] << 8 | buf[5]);
    // ---------------------------------------------------------
    // 【第 3 步】原始值 → m/s²
    // ---------------------------------------------------------
    data->x = rx * IMU_ACCEL_6G_SEN;
    //              ↑
    //        BMI088.hpp:166 定义的系数
    //        0.00179443359375
    //        量程 ±6g 时,1 个 LSB = 这么多 m/s²
    data->y = ry * IMU_ACCEL_6G_SEN;
    data->z = rz * IMU_ACCEL_6G_SEN;
    }

    void cBMI088::ReadGyroData(gyro_data_t *data)
    {
        // TODO: 读取并拼接陀螺仪三轴原始值，按配置量程换算为 rad/s，
        // 减去 Gyro_offset 后写入 data。
        if (data == nullptr) return;
    uint8_t buf[6];
    ReadReg(BMI088_CS_GYRO, GYRO_RATE_X_LSB_ADDR, buf, 6);
    // 无废字节:buf[0]=X_L, buf[1]=X_H, buf[2]=Y_L, ...
    int16_t rx = (int16_t)(buf[1] << 8 | buf[0]);
    int16_t ry = (int16_t)(buf[3] << 8 | buf[2]);
    int16_t rz = (int16_t)(buf[5] << 8 | buf[4]);
    data->x = rx * IMU_GYRO_2000_SEN - Gyro_offset[0];
    data->y = ry * IMU_GYRO_2000_SEN - Gyro_offset[1];
    data->z = rz * IMU_GYRO_2000_SEN - Gyro_offset[2];
    }

    void cBMI088::ReadAccTemperature(float *temp)
    {
        // TODO: 丢弃加速度计读取时的首个无效字节，解析 11 位有符号温度并换算为摄氏度。
        if (temp == nullptr) return;
    uint8_t buf[3];
    ReadReg(BMI088_CS_ACC, TEMP_MSB_ADDR, buf, 3);
    // buf[0]=废, buf[1]=TEMP_MSB, buf[2]=TEMP_LSB
    int16_t raw = (int16_t)((buf[1] << 8) | buf[2]) >> 5;   // 取高 11 位
    *temp = raw * TEMP_UNIT + TEMP_BIAS;   // ×0.125 + 23.0
    }

}

