# OfflineAIStudio

一个基于 Qt 6 的离线 AI 开发工作室，支持多 Agent 协作、代码生成、项目管理和系统操作。

## 功能特性

### 🏠 核心功能
- **多 Agent 架构**：FileAgent、ComputerAgent、SearchAgent、CodeAgent 协同工作
- **对话记忆系统**：支持多轮对话历史，限制最大 20 轮
- **流式输出**：SSE 实时滚动显示 LLM 响应和 Agent 调用日志
- **Markdown 渲染**：支持代码高亮和富文本显示
- **多会话管理**：支持切换会话时加载对应历史记录
- **配置持久化**：自动保存和加载 API 配置、模型选择等

### 📁 FileAgent - 文件操作
- 文件读写（支持自动创建目录）
- 目录创建、删除、列出
- 文件复制、移动、重命名
- 批量文件写入
- 文件搜索（支持通配符）
- 文件详细信息查询

### 💻 ComputerAgent - 系统操作
- 系统命令执行（白名单限制）
- 系统信息获取（OS、CPU、内存、磁盘、网络）
- 进程管理（列出、终止）
- 环境变量管理（获取、设置、列出）
- 服务管理（列出、启动、停止）

### 🔍 SearchAgent - 搜索功能
- 本地文件搜索（支持通配符）
- 文件内容搜索（支持正则表达式）
- 代码符号搜索（类、函数、变量）
- 网络搜索（在线模式）

### 🧑‍💻 CodeAgent - 代码开发
- 项目创建（CMake、Qt、Python）
- 代码生成和编译
- 程序运行和调试
- 代码分析和统计
- 依赖检查和项目清理
- 代码格式化

## 技术栈

- **框架**: Qt 6.11+
- **编译器**: MinGW 13.1+
- **构建系统**: CMake
- **语言**: C++20

## 快速开始

### 环境要求

- Qt 6.11+（包含 MinGW 工具链）
- CMake 3.20+
- Git

### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/Old-schoolprogramming/OfflineAIStudio.git
cd OfflineAIStudio

# 创建构建目录
mkdir build
cd build

# 配置 CMake（Windows）
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<your_qt_path>\6.11.1\mingw_64

# 编译
mingw32-make

# 运行
./OfflineAIStudio.exe
```

### 使用方法

1. **配置模型**：点击设置按钮，输入 API 地址和模型名称
2. **测试连接**：点击"测试连接"验证 API 是否可用
3. **开始对话**：在输入框中输入问题，按 Enter 发送
4. **多 Agent 任务**：系统会自动选择合适的 Agent 完成复杂任务

## 项目结构

```
OfflineAIStudio/
├── src/
│   ├── core/          # 核心模块
│   │   ├── agent.h/cpp         # Agent 基类
│   │   ├── tool.h/cpp          # 工具类
│   │   ├── llmclient.h/cpp     # LLM 客户端（流式输出）
│   │   ├── orchestrator.h/cpp  # 任务编排器
│   │   ├── planner.h/cpp       # 计划解析器
│   │   ├── taskscheduler.h/cpp # 任务调度器
│   │   ├── conversationmanager.h/cpp  # 对话管理器
│   │   ├── environmentdetector.h/cpp  # 环境检测器
│   │   └── securitymanager.h/cpp      # 安全管理器
│   ├── agents/        # Agent 实现
│   │   ├── fileagent.h/cpp     # 文件操作 Agent
│   │   ├── computeragent.h/cpp # 系统操作 Agent
│   │   ├── searchagent.h/cpp   # 搜索 Agent
│   │   └── codeagent.h/cpp     # 代码开发 Agent
│   ├── ui/            # UI 组件
│   │   ├── mainwindow.h/cpp    # 主窗口
│   │   ├── chatpage.h/cpp      # 聊天页面
│   │   ├── thememanager.h/cpp  # 主题管理
│   │   └── ...                 # 其他 UI 组件
│   └── main.cpp       # 入口文件
├── resources/         # 资源文件
│   └── styles.qrc     # 样式资源
├── CMakeLists.txt     # CMake 配置
└── README.md          # 项目说明
```

## 安全特性

- **命令白名单**：仅允许执行安全的系统命令
- **危险操作过滤**：禁止格式化磁盘、删除系统文件等危险操作
- **路径安全检查**：防止路径遍历攻击
- **参数验证**：所有工具调用都经过严格的参数校验

## 离线模式

本应用设计为离线优先，所有功能都可以在无网络环境下使用：
- 网络搜索功能在离线时返回友好提示
- 代码开发不依赖网络包管理器
- 环境检测在启动时完成，无需联网

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！