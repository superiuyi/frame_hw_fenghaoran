#include "threads.hpp"
#include "M2006.hpp"
#include "DJIMotorHandler.hpp"
#include "can.h"
#include "tx_api.h"
#include <cstdio>
#include "usart.h"
M2006 motor;
TX_THREAD control_thread;
static UCHAR control_stack[1024];
//从main.c threadx_init开始，然后进入control_thread_entry
static void control_thread_entry(ULONG input)
{
    (void)input;

    motor.controlMode = DJIMotor::SPD_MODE;   // ★ 第一步先松开!
    motor.speedSet    = 5.0f;

    while (1)
    {
        motor.setOutput();
        DJIMotorHandler::Instance()->sendControlData();

        static int cnt = 0;
        if (++cnt >= 100)
        {
            cnt = 0;
            char buf[64];
            int n = snprintf(buf, sizeof(buf), "rpm=%d  rad/s=%.2f  out=%d\r\n",
                         motor.motorFeedback.speed_rpm,
                         motor.motorFeedback.speedFdb,
                         motor.currentSet);          // ★ 加上这个
            HAL_UART_Transmit(&huart6, (uint8_t*)buf, n, 100);
        }

        tx_thread_sleep(1);
    }
}

void ControlThread_Init()
{
    DJIMotorHandler::Instance()->registerMotor(&motor, &hcan1, 0x201);//在这个地方记录了把M2006 motor传进了Instance

    tx_thread_create(&control_thread, (CHAR *)"control", control_thread_entry, 0,
                     control_stack, sizeof(control_stack),
                     10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
}