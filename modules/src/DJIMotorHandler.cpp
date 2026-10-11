#include "DJIMotorHandler.hpp"

void DJIMotorHandler::registerMotor(DJIMotor *motor, CAN_HandleTypeDef *hcan, uint16_t canId)
{
    // TODO: 校验 CAN 句柄和 0x201~0x208 ID，再登记到对应总线与槽位。
    // TODO: 按 ID 设置对应的 0x200/0x1FF 报文存在标志。
    (void)motor;
    (void)hcan;
    (void)canId;
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
    (void)hcan;
    (void)rx_data;
    (void)index;
}

void DJIMotorHandler::UpdateSensorData(DJIMotor *motor, uint8_t *can_data)
{
    // TODO: 从 CAN 数据解析编码器、转速、电流和温度。
    // TODO: 处理编码器回绕，并按减速比换算输出轴位置和角速度。
    (void)motor;
    (void)can_data;
}

void DJIMotorHandler::AllMotorAliveCheck()
{
    // TODO: 遍历已注册电机并调用 AliveCheck。
}
