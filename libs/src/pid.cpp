#include "pid.hpp"

PID::PID(float kp, float ki, float kd, float maxOut, float maxIOut, int mode)
    : mode(mode), kp(kp), ki(ki), kd(kd), maxOut(maxOut), maxIOut(maxIOut)
{
    Clear();
}

void PID::Tuning(float tuning_kp, float tuning_ki, float tuning_kd)
{
    kp = tuning_kp;
    ki = tuning_ki;
    kd = tuning_kd;
}

void PID::UpdateResult()
{
    err[2] = err[1];
    err[1] = err[0];
    err[0] = ref - fdb;
    if (mode == PID_POSITION)
        {
            pResult = kp * err[0];
            iResult += ki * err[0];
            if (iResult >  maxIOut) iResult =  maxIOut;
            if (iResult < -maxIOut) iResult = -maxIOut;
            dResult = kd * (err[0] - err[1]);
            result = pResult + iResult + dResult;
        }
        else
        {
            float delta = kp * (err[0] - err[1])
                        + ki * err[0]
                        + kd * (err[0] - 2.0f * err[1] + err[2]);
            result += delta;
        }
        if (result >  maxOut) result =  maxOut;
        if (result < -maxOut) result = -maxOut;
    // 完成前始终保持零输出。
}

void PID::Clear()
{
    ref = fdb = 0.0f;
    err[0] = err[1] = err[2] = 0.0f;
    pResult = iResult = dResult = result = 0.0f;
}
