/**
 * @file agent.cpp
 * @brief Agent 抽象基类实现
 *
 * @details
 * Agent 基类的实现非常简单，因为大部分逻辑由子类实现。
 * 基类只提供 QObject 的初始化和虚析构函数的实现。
 */

#include "agent.h"  // 【引入】自己的头文件

/**
 * @brief 构造函数
 * @param parent 父 QObject
 *
 * @implementation
 * 调用 QObject 的构造函数，将 parent 传递给基类。
 * 这使得 Agent 可以加入 Qt 对象树，享受自动内存管理。
 */
Agent::Agent(QObject *parent)
    : QObject(parent)  // 【初始化】调用QObject构造函数，建立对象树关系
{
    // 【说明】基类构造函数体为空，所有初始化在初始化列表完成
}

/**
 * @brief 析构函数
 *
 * @implementation
 * 虚析构函数的实现，确保通过基类指针删除派生类对象时，
 * 能正确调用派生类的析构函数，避免内存泄漏。
 */
Agent::~Agent()
{
    // 【说明】基类析构函数体为空
    // 如果子类在堆上分配了资源，应在子类析构函数中释放
}
