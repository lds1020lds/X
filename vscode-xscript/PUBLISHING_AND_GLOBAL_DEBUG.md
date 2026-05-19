## XScript VS Code 插件发布与全局调试配置指南

本文说明如何打包、安装、发布 `xscript-language` VS Code 插件，以及如何配置**用户级全局调试配置**，避免每个 XScript 项目都重复创建 `.vscode/launch.json`。

## 一、插件当前能力

当前插件在 `package.json` 中已经注册了：

- **语言 ID**：`xscript`
- **文件扩展名**：`.xs`
- **语法高亮**：`syntaxes/xscript.tmLanguage.json`
- **代码片段**：`snippets/xscript.json`
- **断点支持**：`.xs` 文件可打断点
- **调试器类型**：`xscript`
- **调试请求类型**：`launch`、`attach`

因此，只要插件被安装，VS Code 就能识别下面这种调试配置：

```json
{
  "type": "xscript",
  "request": "launch",
  "name": "Launch XScript",
  "program": "${file}",
  "runtime": "C:/Projects/XScriptSVN/trunk/XScript/Debug/XScripter.exe",
  "debugPort": 5678
}
```

## 二、本地打包 VSIX

首次打包前安装依赖：

```bash
npm install
```

如果修改过 `src/extension.ts`、`src/debugAdapter.ts` 或 `server/src/server.ts`，先编译：

```bash
npm run compile
```

安装 VS Code 扩展打包工具：

```bash
npm install -g @vscode/vsce
```

在插件目录执行打包：

```bash
vsce package
```

打包成功后会生成类似文件：

```text
xscript-language-0.0.1.vsix
```

## 三、本地安装 VSIX

### 方式一：命令行安装

```bash
code --install-extension xscript-language-0.0.1.vsix --force
```

### 方式二：VS Code 界面安装

1. 打开 VS Code。
2. 进入扩展面板。
3. 点击右上角 `...`。
4. 选择 `Install from VSIX...`。
5. 选择生成的 `xscript-language-0.0.1.vsix`。

## 四、发布到 VS Code Marketplace

### 1. 准备发布者账号

`package.json` 当前配置为：

```json
{
  "publisher": "xscript"
}
```

正式发布前，需要确保 `publisher` 是你在 VS Code Marketplace 创建并拥有权限的发布者 ID。

如果实际发布者 ID 不是 `xscript`，需要先修改 `package.json`：

```json
{
  "publisher": "你的发布者ID"
}
```

### 2. 创建 Personal Access Token

在 Microsoft Azure DevOps 中创建 Personal Access Token，用于 `vsce` 发布。

通常需要 Marketplace 相关权限。创建后保存 token，因为后续登录会用到。

### 3. 登录发布者

```bash
vsce login xscript
```

如果你的发布者 ID 不是 `xscript`，请替换成实际 ID：

```bash
vsce login 你的发布者ID
```

### 4. 发布插件

```bash
vsce publish
```

也可以指定版本号递增方式：

```bash
vsce publish patch
vsce publish minor
vsce publish major
```

发布后，用户可以直接在 VS Code 扩展市场搜索并安装。

## 五、内部分发发布方式

如果暂时不发布到 Marketplace，可以直接分发 `.vsix` 文件：

- 将 `xscript-language-0.0.1.vsix` 放到内部共享目录。
- 用户通过 `code --install-extension` 安装。
- 或通过 VS Code 的 `Install from VSIX...` 安装。

这种方式适合公司内网、测试版或暂未公开发布的插件。

## 六、全局启动调试与 Attach 配置

### 目标

不希望每个 XScript 项目都创建：

```text
.vscode/launch.json
```

可以把调试配置写到 VS Code 的**用户级 `settings.json`** 中。

这样所有项目都能复用同一套 `Launch XScript` 和 `Attach XScript` 配置。

## 七、打开用户级 settings.json

在 VS Code 中执行：

1. 按 `Ctrl + Shift + P`。
2. 输入并选择 `Preferences: Open User Settings (JSON)`。
3. 在打开的用户级 `settings.json` 中加入 `launch` 配置。

## 八、推荐的全局调试配置

把下面内容加入用户级 `settings.json`：

```json
{
  "xscript.runtimePath": "C:/Projects/XScriptSVN/trunk/XScript/Debug/XScripter.exe",
  "xscript.diagnostics.enable": true,
  "xscript.diagnostics.debounceMs": 500,
  "launch": {
    "version": "0.2.0",
    "configurations": [
      {
        "type": "xscript",
        "request": "launch",
        "name": "XScript: 启动当前文件",
        "program": "${file}",
        "runtime": "C:/Projects/XScriptSVN/trunk/XScript/Debug/XScripter.exe",
        "debugPort": 5678
      },
      {
        "type": "xscript",
        "request": "attach",
        "name": "XScript: Attach 到本机调试服务",
        "host": "127.0.0.1",
        "port": 5678
      }
    ]
  }
}
```

注意：如果用户级 `settings.json` 里已经有其他配置，不要直接整段覆盖，只需要合并下面几个字段：

- `xscript.runtimePath`
- `xscript.diagnostics.enable`
- `xscript.diagnostics.debounceMs`
- `launch`

## 九、全局 Launch 配置说明

```json
{
  "type": "xscript",
  "request": "launch",
  "name": "XScript: 启动当前文件",
  "program": "${file}",
  "runtime": "C:/Projects/XScriptSVN/trunk/XScript/Debug/XScripter.exe",
  "debugPort": 5678
}
```

字段说明：

- **`type`**：必须是 `xscript`，对应插件注册的调试器类型。
- **`request`**：`launch` 表示由 VS Code 启动 `XScripter.exe`。
- **`program`**：`${file}` 表示调试当前打开的 `.xs` 文件。
- **`runtime`**：`XScripter.exe` 的绝对路径。全局配置里推荐使用绝对路径，不依赖项目目录结构。
- **`debugPort`**：调试服务端口，默认使用 `5678`。

使用方式：

1. 打开任意 `.xs` 文件。
2. 在需要的位置打断点。
3. 进入运行和调试面板。
4. 选择 `XScript: 启动当前文件`。
5. 按 `F5` 启动调试。

## 十、全局 Attach 配置说明

```json
{
  "type": "xscript",
  "request": "attach",
  "name": "XScript: Attach 到本机调试服务",
  "host": "127.0.0.1",
  "port": 5678
}
```

字段说明：

- **`type`**：必须是 `xscript`。
- **`request`**：`attach` 表示连接到已经运行中的 XScript 调试服务。
- **`host`**：调试服务地址，本机一般是 `127.0.0.1`。
- **`port`**：调试服务端口，需要和 `XScripter.exe` 启动时监听的端口一致。

使用方式：

1. 先用外部方式启动支持调试协议的 XScript 程序。
2. 确认调试服务正在监听 `5678` 端口。
3. 打开任意 `.xs` 文件。
4. 在 VS Code 调试面板选择 `XScript: Attach 到本机调试服务`。
5. 按 `F5` 连接。

## 十一、工作区配置与全局配置的关系

VS Code 调试配置优先级通常可以理解为：

1. 当前工作区的 `.vscode/launch.json`
2. 用户级 `settings.json` 里的 `launch`
3. 插件 `package.json` 里提供的 `initialConfigurations`

如果某个项目有特殊的运行参数，可以只在该项目创建 `.vscode/launch.json` 覆盖全局配置。

如果大多数项目都使用同一个 `XScripter.exe` 和同一个端口，推荐使用用户级全局配置。

## 十二、常见问题

### 1. 调试配置列表里看不到 XScript 配置

检查插件是否已经安装并启用。

可以通过命令行确认：

```bash
code --list-extensions | findstr xscript
```

如果是本地 VSIX 安装，重新安装：

```bash
code --install-extension xscript-language-0.0.1.vsix --force
```

### 2. 启动调试时报找不到 XScripter.exe

检查用户级 `settings.json` 中的路径：

```json
{
  "xscript.runtimePath": "C:/Projects/XScriptSVN/trunk/XScript/Debug/XScripter.exe"
}
```

同时检查全局 `launch.configurations[].runtime` 是否也是正确路径。

### 3. Attach 连接失败

检查：

- `XScripter.exe` 是否已经启动调试服务。
- 端口是否是 `5678`。
- 防火墙是否拦截本机连接。
- `host` 是否应该从 `127.0.0.1` 改成实际调试主机 IP。

### 4. 修改插件后安装没有变化

建议执行完整流程：

```bash
npm run compile
vsce package
code --install-extension xscript-language-0.0.1.vsix --force
```

然后重启 VS Code，避免旧扩展进程仍在运行。
