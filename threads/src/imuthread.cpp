#include "threads.hpp"
#include "BMI088.hpp"
#include "usart.h"
#include "tx_api.h"
#include <cstdio>

TX_THREAD imu_thread;
static UCHAR imu_stack[2048];

static void imu_thread_entry(ULONG input);

void ImuThread_Init()
{
    tx_thread_create(&imu_thread, (CHAR *)"imu", imu_thread_entry, 0,
                     imu_stack, sizeof(imu_stack),
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);
}

static void imu_thread_entry(ULONG input)
{
    (void)input;

    BMI088::cBMI088 imu;
    char buf[96];

    imu.Config();                 // 用 WriteReg 配一堆寄存器
   
    imu.VerifyAccChipID();        // 用 ReadReg 读 0x1E
    imu.VerifyGyroChipID();       // 用 ReadReg 读 0x0F
    // 改成判断两个 ID 标志(不用 INIT_ERR,它永远是 true)
    if (!imu.self_test.ACC_CHIP_ID_ERR && !imu.self_test.GYRO_CHIP_ID_ERR)
        tx_semaphore_put(&sem_init);        // ★ 初始化成功,只放一
    uint8_t w;
    w = 0x04;  imu.WriteReg(BMI088::BMI088_CS_ACC, 0x7D, &w, 1);   // PWR_CTRL ← 开电源
    tx_thread_sleep(50);
    w = 0x00;  imu.WriteReg(BMI088::BMI088_CS_ACC, 0x7C, &w, 1);   // PWR_CONF ← 正常模式
    tx_thread_sleep(50);
    uint8_t r1[2], r2[2], r3[2];
    imu.ReadReg(BMI088::BMI088_CS_ACC, 0x7D, r1, 2);   // PWR_CTRL → 应为 0x04
    imu.ReadReg(BMI088::BMI088_CS_ACC, 0x7C, r2, 2);   // PWR_CONF → 应为 0x00
    imu.ReadReg(BMI088::BMI088_CS_ACC, 0x41, r3, 2);   // RANGE    → 应为 0x01
    char ch[96];
    int k = snprintf(ch, sizeof(ch), "[CHK] PWR=0x%02X CONF=0x%02X RANGE=0x%02X\r\n",
                     r1[1], r2[1], r3[1]);
    HAL_UART_Transmit(&huart6, (uint8_t*)ch, k, 100);
    imu.Calibrate();        // ← 标定零偏,板子必须静止

    // 0 = 正确,1 = 错误
    int n = snprintf(buf, sizeof(buf), "ACC_ID_ERR=%d  GYRO_ID_ERR=%d\r\n",
                     imu.self_test.ACC_CHIP_ID_ERR,
                     imu.self_test.GYRO_CHIP_ID_ERR);
    HAL_UART_Transmit(&huart6, (uint8_t*)buf, n, 100);

     while (1)
    {
        acc_data_t  acc;
        gyro_data_t gyro;
        float       temp = 0.0f;
        imu.ReadAccData(&acc);
        imu.ReadGyroData(&gyro);
        imu.ReadAccTemperature(&temp);
        char b[128];
        int m = snprintf(b, sizeof(b),
                         "[IMU] a=%.2f,%.2f,%.2f  g=%.2f,%.2f,%.2f  t=%.1f\r\n",
                         acc.x, acc.y, acc.z,
                         gyro.x, gyro.y, gyro.z, temp);
        HAL_UART_Transmit(&huart6, (uint8_t*)b, m, 100);
        tx_semaphore_put(&sem_imu);          // ★ 加这行:我活着

        tx_thread_sleep(100);
    }
}