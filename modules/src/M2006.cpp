#include "M2006.hpp"

M2006::M2006()
{
    // TODO: 根据 M2006 手册设置减速比、允许电流和速度/位置环 PID 参数。
    // 基类初始化为松开模式、零电流；完成配置前不允许驱动电机。
    gearBox    = GearBox_M2006;   // 减速比 36:1
    maxCurrent = 10000;           // ⚠️ 查 C610 手册确认
    // PID 参数(kp/ki/kd 也可以直接写进 DJIMotor.hpp:65-66)
    //speedPid.Tuning(3.0f, 0.3f, 0.0f);
  //  positionPid.Tuning(5.0f, 0.0f, 20.0f);
}

void M2006::setOutput()
{
    // TODO: 分别实现松开、速度和位置模式；位置模式需要串联位置环与速度环。
    // TODO: 将 PID 输出限制在 maxCurrent 范围内后写入 currentSet。
     switch (controlMode)
    {
        case RELAX_MODE:
            currentSet = 0;
            speedPid.Clear();
            positionPid.Clear();
            break;
        case SPD_MODE:
            speedPid.ref = speedSet;
            speedPid.fdb = motorFeedback.speedFdb;
            speedPid.UpdateResult();
            currentSet = (int16_t)speedPid.result;
            break;
        case POS_MODE:
            // 外环:位置 → 速度
            positionPid.ref = positionSet;
            positionPid.fdb = motorFeedback.positionFdb;
            positionPid.UpdateResult();
            // 内环:速度 → 电流
            speedPid.ref = positionPid.result;
            speedPid.fdb = motorFeedback.speedFdb;
            speedPid.UpdateResult();
            currentSet = (int16_t)speedPid.result;
            break;
    }
    // 最后一道限幅
    if (currentSet >  (int16_t)maxCurrent) currentSet =  (int16_t)maxCurrent;
    if (currentSet < -(int16_t)maxCurrent) currentSet = -(int16_t)maxCurrent;
}

M2006::MotorStateTypedef M2006::AliveCheck()
{
    // TODO: 比较 AliveFlag 与 Pre_AliveFlag，更新在线状态并保存本次计数。
    MotorState = (AliveFlag == Pre_AliveFlag) ? MOTOR_OFFLINE : MOTOR_ONLINE;
    Pre_AliveFlag = AliveFlag;
    return MotorState;
}
