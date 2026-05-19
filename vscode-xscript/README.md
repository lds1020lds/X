# XScript Language Support

这是 XScript 的 VSCode 语言扩展，当前提供：

- `.xs` 文件语法高亮
- 注释、括号、字符串自动闭合配置
- 常用代码片段
- 关键字、内置函数、当前文件函数补全
- LSP 语法诊断：调用 `XScripter.exe --syntax-only --recover`
- 当前文件函数定义跳转
- 当前文件函数 Outline 符号

## 开发调试

用 VSCode 打开本目录，按 `F5` 启动 Extension Development Host。

## 本地安装

```bash
npm install -g @vscode/vsce
vsce package
code --install-extension xscript-language-0.0.1.vsix --force
```

## TypeScript 编译

仓库里已经包含 `out/extension.js`，可直接调试运行。

如果修改 `src/extension.ts` 或 `server/src/server.ts`，可以安装依赖后重新编译：

```bash
npm install
npm run compile
```

## 语法诊断配置

扩展会启动 LSP server，并在文本变更后延迟调用：

```bash
XScripter.exe --syntax-only --recover <临时文件>
```

默认 runtime 路径按当前目录推导为 `../Debug/XScripter.exe`。如果移动扩展目录，请在 VSCode 设置里配置：

```json
{
  "xscript.runtimePath": "C:/Projects/XScriptSVN/trunk/XScript/Debug/XScripter.exe",
  "xscript.diagnostics.enable": true,
  "xscript.diagnostics.debounceMs": 500
}
```
