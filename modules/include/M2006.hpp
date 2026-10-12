//
// Created by cosmosmount on 2025/8/30.
//

#ifndef RM26_M2006_HPP
#define RM26_M2006_HPP

#include "DJIMotor.hpp"

/**
 * @class M2006
 * @brief M2006电机的控制类，继承自Motor类。
 *
 * 这个类实现了M2006电机的控制方法，包括输出设置等。
 */
 //djiMOTOR中储存了电机的具体信息！！！
 //而M2006是电机型号，可以用来创建具体的对象，同时继承djimotor的具体信息
 //为什么这么做？方便！！！！！
class M2006 final : public DJIMotor
{
public:
    /**
     * @brief 构造函数，初始化M2006电机控制类。
     */
    M2006();

    /**
     * @brief 析构函数。
     */
    ~M2006() override = default;

    /**
     * @brief 实现电机输出设置。
     * 根据当前的控制模式和PID反馈调整电机输出。
     * 必须在派生类中具体实现此方法以适应具体电机。
     */
    void setOutput() override;

    MotorStateTypedef AliveCheck() override;
};

#endif //RM26_M2006_HPP
