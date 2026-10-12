#include "DJIMotorHandler.hpp"

#include "bsp_can.hpp" 
void DJIMotorHandler::registerMotor(DJIMotor *motor, CAN_HandleTypeDef *hcan, uint16_t canId)
{
    // TODO: 校验 CAN 句柄和 0x201~0x208 ID，再登记到对应总线与槽位。
    // TODO: 按 ID 设置对应的 0x200/0x1FF 报文存在标志。

      // ---------- ① 校验 ----------
    if (motor == nullptr || hcan == nullptr) return;
    if (canId < 0x201 || canId > 0x208) return;
    // ---------- ② 算总线 + 槽位 ----------
    int bus   = (hcan->Instance == CAN1) ? 0 : 1;
    int index = canId - 0x201;
    // ---------- ③ 存指针 ----------
    DJIMotorList[bus][index] = motor;
    motor->hcan  = hcan;
    motor->canId = canId;
    // ---------- ④ 设标志 ----------
    if (bus == 0) {
        if (index < 4) CAN1_0x200_Exist = true;
        else           CAN1_0x1FF_Exist = true;
    } else {
        if (index < 4) CAN2_0x200_Exist = true;
        else           CAN2_0x1FF_Exist = true;
    }
}

void DJIMotorHandler::sendControlData()
{
    // TODO: 将各电机 currentSet 按大端序放入正确的 8 字节控制帧。
    // TODO: 仅发送包含已注册电机的报文；未完成前不发送 CAN 电流命令。
       // ---------- CAN1 / 0x200(电机1~4) ----------
    if (CAN1_0x200_Exist)                     // ① 有电机才发
    {
        for (int i = 0; i < 4; i++)           // ② 遍历 4 个槽位
        {
            DJIMotor *m = DJIMotorList[0][i];                // ③ 取指针
            int16_t c = (m != nullptr) ? m->currentSet : 0;  // ④ 空槽位发 0
            can1_send_data_0[i * 2]     = (uint8_t)(c >> 8);     // ⑤ 高字节
            can1_send_data_0[i * 2 + 1] = (uint8_t)(c & 0xFF);   //    低字节
        }
        CAN_Transmit(&hcan1, 0x200, can1_send_data_0, 8);        // ⑥ 发
    }
    // ---------- CAN1 / 0x1FF(电机5~8) ----------
    if (CAN1_0x1FF_Exist)                     // ★ 独立的 if,不是 else if
    {
        for (int i = 0; i < 4; i++)
        {
            DJIMotor *m = DJIMotorList[0][i + 4];   // ★ 槽位 +4
            int16_t c = (m != nullptr) ? m->currentSet : 0;
            can1_send_data_1[i * 2]     = (uint8_t)(c >> 8);
            can1_send_data_1[i * 2 + 1] = (uint8_t)(c & 0xFF);
        }
        CAN_Transmit(&hcan1, 0x1FF, can1_send_data_1, 8);
    }
    // ---------- CAN2 / 0x200 ----------
    if (CAN2_0x200_Exist)
    {
        for (int i = 0; i < 4; i++)
        {
            DJIMotor *m = DJIMotorList[1][i];
            int16_t c = (m != nullptr) ? m->currentSet : 0;
            can2_send_data_0[i * 2]     = (uint8_t)(c >> 8);
            can2_send_data_0[i * 2 + 1] = (uint8_t)(c & 0xFF);
        }
        CAN_Transmit(&hcan2, 0x200, can2_send_data_0, 8);
    }
    // ---------- CAN2 / 0x1FF ----------
    if (CAN2_0x1FF_Exist)
    {
        for (int i = 0; i < 4; i++)
        {
            DJIMotor *m = DJIMotorList[1][i + 4];
            int16_t c = (m != nullptr) ? m->currentSet : 0;
            can2_send_data_1[i * 2]     = (uint8_t)(c >> 8);
            can2_send_data_1[i * 2 + 1] = (uint8_t)(c & 0xFF);
        }
        CAN_Transmit(&hcan2, 0x1FF, can2_send_data_1, 8);
    }
}

void DJIMotorHandler::updateFeedback(CAN_HandleTypeDef *hcan, uint8_t *rx_data, int index)
{
    // TODO: 校验总线、反馈数据和索引，再找到对应电机并调用 UpdateSensorData。
    // ① 参数校验
    if (rx_data == nullptr || index < 0 || index > 7) return;
    // ② 算出哪条总线
    int bus = (hcan->Instance == CAN1) ? 0 : 1;
    // ③ 查表找电机
    DJIMotor *motor = DJIMotorList[bus][index];
    if (motor == nullptr) return;              // 这个 ID 没登记过,丢掉
    // ④ 交给解析函数
    UpdateSensorData(motor, rx_data);
    // ⑤ 心跳计数(AliveCheck 靠它判在线)
    motor->AliveFlag++;
}

void DJIMotorHandler::UpdateSensorData(DJIMotor *motor, uint8_t *can_data)
{
    // TODO: 从 CAN 数据解析编码器、转速、电流和温度。
    // TODO: 处理编码器回绕，并按减速比换算输出轴位置和角速度。
    if (motor == nullptr || can_data == nullptr) return;
    //   ↑ 防御:两个指针都不能空
    // ========== ① 解析 4 个量 ==========
    motor->motorFeedback.last_ecd  = (int16_t)motor->motorFeedback.ecd;
    //   ↑ 先把【上一轮】的角度存起来,给第 ④ 步算 delta 用
    //     注意顺序:必须在更新 ecd 之前存
    motor->motorFeedback.ecd       = (uint16_t)(can_data[0] << 8 | can_data[1]);
    //   ↑ 角度:uint16,不用 (int16_t)
    motor->motorFeedback.speed_rpm = (int16_t) (can_data[2] << 8 | can_data[3]);
    //   ↑ 转速:int16,必须转有符号(可能是负的)
    motor->motorFeedback.currentFdb =
        (float)(int16_t)(can_data[4] << 8 | can_data[5]);
    //   ↑ 先按 int16 解析,再转成 float 存进结构体
    motor->motorFeedback.temperatureFdb = (float)can_data[6];
    //   ↑ 温度:单字节,直接转 float
    // ========== ② 查减速比 ==========
    float ratio = 1.0f;
    if (motor->gearBox == GearBox_M2006)      ratio = 36.0f;
    else if (motor->gearBox == GearBox_M3508) ratio = 3591.0f / 187.0f;
    // ========== ③ 转速换算 ==========
    motor->motorFeedback.lastSpeedFdb = motor->motorFeedback.speedFdb;
    //   ↑ 同样先存旧的
    motor->motorFeedback.speedFdb =
        (float)motor->motorFeedback.speed_rpm * 2.0f * 3.14159265f / 60.0f / ratio;
    //      │                                    └── rpm→rad/s ──┘   └转子→输出轴┘
    //      └ 先转成 float,否则整数运算会丢精度
    // ========== ④ 位置:回绕 + 累积 ==========
    int16_t delta = (int16_t)(motor->motorFeedback.ecd - motor->motorFeedback.last_ecd);
    if (delta >  4096) delta -= 8192;
    if (delta < -4096) delta += 8192;
    motor->motorFeedback.lastPositionFdb = motor->motorFeedback.positionFdb;
    motor->motorFeedback.positionFdb +=
        (float)delta / 8192.0f * 2.0f * 3.14159265f / ratio;
    //      └─ 刻度 → 弧度 ─┘              └ 转子→输出轴 ┘
    //   ↑ += 累加,不是 = 赋值!
}

void DJIMotorHandler::AllMotorAliveCheck()
{
    // TODO: 遍历已注册电机并调用 AliveCheck。
     for (int bus = 0; bus < 2; bus++)            // ① 两条总线
        for (int i = 0; i < 8; i++)              // ② 每条 8 个槽位
            if (DJIMotorList[bus][i] != nullptr) // ③ 空槽位跳过
                DJIMotorList[bus][i]->AliveCheck();  // ④ 调用
}
