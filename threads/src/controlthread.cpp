#include "threads.hpp"
#include "M2006.hpp"
#include "DJIMotorHandler.hpp"
#include "can.h"
#include "tx_api.h"

// 电机对象(文件作用域,不占线程栈)
M2006 motor;

// 线程控制块 + 栈
TX_THREAD control_thread;
static UCHAR control_stack[1024];

// 线程主体
static void control_thread_entry(ULONG input)
{
    (void)input;

    motor.controlMode = DJIMotor::SPD_MODE;   // 速度模式
    motor.speedSet    = 100.0f;               // 目标 100 rad/s

    while (1)
    {
        motor.setOutput();                                   // 算
        DJIMotorHandler::Instance()->sendControlData();      // 发
        tx_thread_sleep(1);                                  // 1ms
    }
    
}

// 创建线程(供 App_ThreadX_Init 调用)
void ControlThread_Init()
{
    DJIMotorHandler::Instance()->registerMotor(&motor, &hcan1, 0x201);

    tx_thread_create(&control_thread, "control", control_thread_entry, 0,
                     control_stack, sizeof(control_stack),
                     10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
}