# Offline AI Studio

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6.11-blue?style=flat-square&logo=qt" />
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square" />
  <img src="https://img.shields.io/badge/CMake-3.16%2B-green?style=flat-square&logo=cmake" />
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=flat-square" />
</p>

> 基于 C++ / Qt6 构建的本地大模型交互客户端，支持多 Agent 协作任务执行、深色/浅色主题切换与技能扩展，完全离线运行。

---

## 项目架构

```
OfflineAIStudio/
├── src/
│   ├── main.cpp                    # 应用入口
│   ├── mainwindow.h/.cpp           # 主窗口（导航 + 页面容器）
│   ├── core/                       # 核心引擎层
│   │   ├── agent.h/.cpp            # Agent 抽象基类
│   │   ├── llmclient.h/.cpp        # LLM HTTP 客户端
│   │   ├── orchestrator.h/.cpp     # 总控协调器（规划 + 调度）
│   │   ├── planner.h/.cpp          # JSON 计划解析器
│   │   ├── promptbuilder.h/.cpp    # Prompt 构造器
│   │   ├── taskscheduler.h/.cpp    # 任务调度引擎
│   │   ├── task.h                  # 任务数据结构
│   │   └── tool.h/.cpp             # 工具抽象基类
│   ├── agents/                     # 具体 Agent 实现
│   │   ├── fileagent.h/.cpp        # 文件操作 Agent
│   │   └── computeragent.h/.cpp    # 系统命令 Agent
│   └── ui/                         # 用户界面层
│       ├── thememanager.h/.cpp     # 主题管理器（单例）
│       ├── iconhelper.h/.cpp       # 图标绘制辅助
│       ├── chatpage.h/.cpp         # 对话页面
│       ├── modelconfigpage.h/.cpp  # 模型配置页面
│       └── skillimportpage.h/.cpp  # 技能导入页面
├── CMakeLists.txt                  # CMake 构建配置
└── README.md                       # 本文档
```

### 架构分层

| 层级 | 职责 | 核心组件 |
|------|------|----------|
| **UI 层** | 用户交互与视觉呈现 | ChatPage、ModelConfigPage、SkillImportPage、ThemeManager |
| **调度层** | 任务规划与 Agent 路由 | Orchestrator、TaskScheduler、Planner |
| **Agent 层** | 领域特定工具执行 | FileAgent、ComputerAgent |
| **通信层** | LLM API 网络通信 | LlmClient（OpenAI 兼容格式） |

### 执行流

```
用户输入
    ↓
Orchestrator::processQuery()
    ↓
PromptBuilder 构造规划 Prompt
    ↓
LlmClient 发送 HTTP 请求
    ↓
Planner 解析 JSON 响应 → TaskPlan
    ↓
TaskScheduler 逐步骤调度 Agent
    ↓
Agent 执行具体工具 → 输出结果
    ↓
Orchestrator 汇总 → UI 展示
```

---

## 技术特性

- **C++17 / Qt6.11** —— 原生跨平台 GUI 框架
- **CMake 构建** —— 现代化构建系统
- **OpenAI 兼容 API** —— 支持任意本地部署的大模型（Ollama、llama.cpp、vLLM 等）
- **Multi-Agent 架构** —— 大模型生成计划，C++ 层全权执行
- **双主题系统** —— 深色/浅色模式，QSettings 本地持久化
- **QPainter 矢量图标** —— 无外部图标字体依赖，完全离线
- **圆角设计语言** —— 一致的 8–16px 圆角现代 UI

---

## 构建

### 环境要求

- Qt 6.11+（MinGW 或 MSVC）
- CMake 3.16+
- C++17 兼容编译器

### Windows (MinGW)

```powershell
# 配置
cmake -B build -S . -G "MinGW Makefiles"

# 编译
cmake --build build --parallel

# 运行
.\build\OfflineAIStudio.exe
```

---

## 界面预览

| 对话页面 | 模型配置 | 技能导入 |
|---------|---------|---------|
| 左侧对话历史 + 右侧聊天区域，消息左右分栏 | 连接状态、API 配置、高级参数、已安装模型 | 拖拽上传、技能卡片网格、启用开关 |

---

## 核心类说明

| 类名 | 文件 | 职责 |
|------|------|------|
| `MainWindow` | `mainwindow.h` | 顶层窗口容器，导航栏 + QStackedWidget 页面管理 |
| `ChatPage` | `ui/chatpage.h` | 对话交互页面，消息渲染、计划展示、输入控制 |
| `Orchestrator` | `core/orchestrator.h` | 两阶段执行模式：规划（LLM）→ 执行（TaskScheduler）|
| `TaskScheduler` | `core/taskscheduler.h` | 按 TaskPlan 逐步骤路由到对应 Agent 执行 |
| `ThemeManager` | `ui/thememanager.h` | 单例主题管理器，QSettings 持久化，全局样式表生成 |
| `IconHelper` | `ui/iconhelper.h` | QPainter 矢量图标绘制，零外部依赖 |

---

## License

MIT License
