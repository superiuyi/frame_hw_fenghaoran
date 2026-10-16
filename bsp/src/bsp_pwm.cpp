//
// Created by cosmosmount on 2025/9/2.
//

#include "bsp_pwm.hpp"
#include "tim.h"                    // htim10

void PWM_Init(void)
{
    // TODO: 根据实际使用的定时器和通道完成 PWM 初始化。
    HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);

}

void PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    // TODO: 调用 HAL 启动指定定时器通道的 PWM，并检查返回状态。
    if (htim == nullptr) return;    // 防空指针
    HAL_TIM_PWM_Start(htim, Channel);
}

void PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    // TODO: 调用 HAL 停止指定定时器通道的 PWM。
    if (htim == nullptr) return;
    HAL_TIM_PWM_Stop(htim, Channel);
}

void PWM_SetPeriod(TIM_HandleTypeDef *htim, float period_s)
{
    // TODO: 根据定时器时钟和预分频值，将秒转换为 ARR 并更新周期。
    if (htim == nullptr) return;
    uint32_t psc     = htim->Init.Prescaler;   // 读预分频值
    //     ↑            ↑        ↑
    //  32位无符号   句柄里的   初始化时配的
    //               Init结构   PSC(=167)
    uint32_t tim_clk = 168000000;              // 168MHz,和 board.ioc 一致
    uint32_t arr = (uint32_t)(period_s * tim_clk / (psc + 1)) - 1;
    //                                            ↑
    //                                     PSC 是从 0 开始数的
    //                                     所以实际分频数 = PSC+1
    __HAL_TIM_SET_AUTORELOAD(htim, arr);       // 写进 ARR 寄存器
}

void PWM_SetDutyRatio(TIM_HandleTypeDef *htim, float dutyratio, uint32_t channel)
{
    // TODO: 将 [0, 1] 占空比换算为比较值，写入指定通道。
    if (htim == nullptr) return;
    // ① 限幅:必须是 0~1
    if (dutyratio < 0.0f) dutyratio = 0.0f;
    if (dutyratio > 1.0f) dutyratio = 1.0f;
    // ② 读当前周期
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
    //   定时器数到 arr 就归零,所以一共 arr+1 个刻度
    // ③ 占空比 × 总刻度 = 比较值
    uint32_t ccr = (uint32_t)(dutyratio * (arr + 1));
    //   例:arr=99(共100个刻度),duty=0.3 → ccr = 30
    // ④ 写进 CCR
    __HAL_TIM_SET_COMPARE(htim, channel, ccr);
    //   效果:CNT 从 0 数到 30 → 这段时间输出高电平
    //        从 30 数到 99 → 输出低电平
    //   → 占空比 = 30/100 = 30% ✅
}
