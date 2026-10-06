
## 10-06 开发环境搭建
**目标**:在 macOS + VSCode 上打通"编译 → 烧录"。
**遇到并解决的问题**:
1. **VSCode 找不到工具链**(ninja / arm-none-eabi-gcc)
   原因:macOS 从 Dock 启动的 GUI 程序不继承终端的 PATH。
   解决:安装 `code` 命令,改为从终端 `code .` 启动 VSCode。
2. **CMake 在错误的目录找工程**
   原因:仓库根目录残留一份坏的 CMakeLists.txt(课程作者把工程
   下沉到 board/ 时的遗留副本),`${sourceDir}` 解析成仓库根,
   导致 toolchainFile 路径不存在。
   解决:建 .vscode/settings.json,指定 "cmake.sourceDirectory"
   为 ${workspaceFolder}/board。
3. **settings.json 文件名写错**
   写成 setting.json(少一个 s),VSCode 静默忽略,不报任何错。
   教训:VSCode 配置文件名字是硬性的。
4. **没有 launch.json**
   解决:建 .vscode/launch.json,配置 Cortex-Debug + OpenOCD。
**结果**:VSCode 内可一键编译、F5 烧录并停在 main()。
首次烧录输出 `** Verified OK **`,MSP = 0x20020000 与链接脚本
的 RAM 顶端(128KB)一致。
