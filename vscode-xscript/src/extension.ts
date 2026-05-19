import * as path from 'path';
import * as vscode from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

type FunctionInfo = {
    name: string;
    range: vscode.Range;
    selectionRange: vscode.Range;
    detail: string;
};

const languageSelector: vscode.DocumentSelector = { language: 'xscript', scheme: 'file' };

const keywords = [
    'var', 'function', 'lambda', 'if', 'else', 'switch', 'case', 'default', 'while', 'for', 'foreach', 'in',
    'return', 'break', 'continue', 'nil', 'true', 'false', 'or', 'async', 'await', 'try', 'catch', 'finally', 'throw',
    'defer', 'iterator', 'generator', 'emit'
];

const builtinFunctions = [
    'require', 'printf', 'assert', 'assertEqual', 'type', 'toString', 'toNumber',
    'pcall', 'xpcall', 'loadstring', 'loadfile', 'setmetatable', 'getmetatable', 'pairs'
];

const builtinTables = [
    'string', 'math', 'list', 'coroutine', 'package', 'os', 'io', 'debug'
];

const stringMembers = [
    'len', 'sub', 'find', 'atof', 'atoi', 'format', 'lower', 'upper', 'split', 'trim'
];

const mathMembers = [
    'abs', 'sqrt', 'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'exp', 'log', 'log10', 'floor', 'ceil', 'random'
];

const listMembers = [
    'size', 'append', 'insert', 'remove', 'sort', 'reverse', 'count'
];

const coroutineMembers = [
    'create', 'resume', 'yield', 'status', 'wrap'
];

let lspClient: LanguageClient | undefined;

export function activate(context: vscode.ExtensionContext) {
    const serverModule = context.asAbsolutePath(path.join('server', 'out', 'server.js'));
    const serverOptions: ServerOptions = {
        run: { module: serverModule, transport: TransportKind.ipc },
        debug: { module: serverModule, transport: TransportKind.ipc, options: { execArgv: ['--nolazy', '--inspect=6009'] } }
    };
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ language: 'xscript', scheme: 'file' }],
        synchronize: {
            configurationSection: 'xscript'
        },
        initializationOptions: {
            defaultRuntimePath: context.asAbsolutePath(path.join('..', 'Debug', 'XScripter.exe'))
        }
    };
    lspClient = new LanguageClient('xscriptLanguageServer', 'XScript Language Server', serverOptions, clientOptions);
    context.subscriptions.push(lspClient);
    lspClient.start();

    context.subscriptions.push(vscode.languages.registerCompletionItemProvider(
        languageSelector,
        new XScriptCompletionProvider(),
        '.', ':'
    ));

    context.subscriptions.push(vscode.languages.registerDefinitionProvider(
        languageSelector,
        new XScriptDefinitionProvider()
    ));

    context.subscriptions.push(vscode.languages.registerDocumentSymbolProvider(
        languageSelector,
        new XScriptDocumentSymbolProvider()
    ));

}

export function deactivate() {
    return lspClient?.stop();
}

class XScriptCompletionProvider implements vscode.CompletionItemProvider {
    provideCompletionItems(document: vscode.TextDocument, position: vscode.Position): vscode.CompletionItem[] {
        const linePrefix = document.lineAt(position).text.slice(0, position.character);
        const memberMatch = linePrefix.match(/([A-Za-z_\u0080-\uFFFF][A-Za-z0-9_\u0080-\uFFFF]*)\s*[\.:]\s*([A-Za-z0-9_\u0080-\uFFFF]*)$/);
        if (memberMatch) {
            return createMemberCompletions(memberMatch[1]);
        }

        const completions: vscode.CompletionItem[] = [];
        for (const keyword of keywords) {
            const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
            item.detail = 'XScript 关键字';
            completions.push(item);
        }

        for (const name of builtinFunctions) {
            const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
            item.detail = 'XScript 内置函数';
            item.insertText = new vscode.SnippetString(`${name}($0)`);
            completions.push(item);
        }

        for (const name of builtinTables) {
            const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Module);
            item.detail = 'XScript 内置库';
            completions.push(item);
        }

        for (const func of collectFunctions(document)) {
            const item = new vscode.CompletionItem(func.name, vscode.CompletionItemKind.Function);
            item.detail = func.detail;
            item.insertText = new vscode.SnippetString(`${func.name}($0)`);
            completions.push(item);
        }

        return completions;
    }
}

class XScriptDefinitionProvider implements vscode.DefinitionProvider {
    provideDefinition(document: vscode.TextDocument, position: vscode.Position): vscode.Definition | undefined {
        const range = document.getWordRangeAtPosition(position, /[A-Za-z_\u0080-\uFFFF][A-Za-z0-9_\u0080-\uFFFF]*/);
        if (!range) {
            return undefined;
        }

        const word = document.getText(range);
        for (const func of collectFunctions(document)) {
            if (func.name === word || func.name.endsWith(`.${word}`) || func.name.endsWith(`:${word}`)) {
                return new vscode.Location(document.uri, func.selectionRange);
            }
        }

        return undefined;
    }
}

class XScriptDocumentSymbolProvider implements vscode.DocumentSymbolProvider {
    provideDocumentSymbols(document: vscode.TextDocument): vscode.DocumentSymbol[] {
        return collectFunctions(document).map(func => new vscode.DocumentSymbol(
            func.name,
            func.detail,
            vscode.SymbolKind.Function,
            func.range,
            func.selectionRange
        ));
    }
}

function createMemberCompletions(receiver: string): vscode.CompletionItem[] {
    const lower = receiver.toLowerCase();
    let members: string[] = [];
    if (lower === 'string') {
        members = stringMembers;
    } else if (lower === 'math') {
        members = mathMembers;
    } else if (lower === 'list') {
        members = listMembers;
    } else if (lower === 'coroutine') {
        members = coroutineMembers;
    }

    return members.map(member => {
        const item = new vscode.CompletionItem(member, vscode.CompletionItemKind.Method);
        item.detail = `${receiver} 成员`;
        item.insertText = new vscode.SnippetString(`${member}($0)`);
        return item;
    });
}

function collectFunctions(document: vscode.TextDocument): FunctionInfo[] {
    const result: FunctionInfo[] = [];
    const text = stripCommentsAndStrings(document.getText());
    const pattern = /\b(?:(?:async|generator)\s+)?function\s+([A-Za-z_\u0080-\uFFFF][A-Za-z0-9_\u0080-\uFFFF]*(?:\s*(?:\.|:)\s*[A-Za-z_\u0080-\uFFFF][A-Za-z0-9_\u0080-\uFFFF]*)*)\s*\(/g;
    let match: RegExpExecArray | null;
    while ((match = pattern.exec(text)) !== null) {
        const rawName = match[1];
        const name = rawName.replace(/\s+/g, '');
        const start = document.positionAt(match.index);
        const nameStart = document.positionAt(match.index + match[0].indexOf(rawName));
        const nameEnd = document.positionAt(match.index + match[0].indexOf(rawName) + rawName.length);
        const end = findFunctionEnd(document, start);
        result.push({
            name,
            range: new vscode.Range(start, end),
            selectionRange: new vscode.Range(nameStart, nameEnd),
            detail: 'XScript 函数'
        });
    }
    return result;
}

function collectDiagnostics(document: vscode.TextDocument): vscode.Diagnostic[] {
    const diagnostics: vscode.Diagnostic[] = [];
    collectBracketDiagnostics(document, diagnostics);
    collectInternalIdentifierDiagnostics(document, diagnostics);
    return diagnostics;
}

function collectBracketDiagnostics(document: vscode.TextDocument, diagnostics: vscode.Diagnostic[]) {
    const stack: Array<{ char: string; position: vscode.Position }> = [];
    const pairs: Record<string, string> = { ')': '(', ']': '[', '}': '{' };
    const text = document.getText();
    let state: 'normal' | 'string' | 'tripleString' | 'lineComment' | 'blockComment' = 'normal';

    for (let i = 0; i < text.length; i++) {
        const ch = text[i];
        const next = text[i + 1];
        const next2 = text[i + 2];
        const position = document.positionAt(i);

        if (state === 'lineComment') {
            if (ch === '\n') {
                state = 'normal';
            }
            continue;
        }

        if (state === 'blockComment') {
            if (ch === '*' && next === '/') {
                state = 'normal';
                i++;
            }
            continue;
        }

        if (state === 'string') {
            if (ch === '\\') {
                i++;
                continue;
            }
            if (ch === '"') {
                state = 'normal';
            }
            continue;
        }

        if (state === 'tripleString') {
            if (ch === '\\') {
                i++;
                continue;
            }
            if (ch === '"' && next === '"' && next2 === '"') {
                state = 'normal';
                i += 2;
            }
            continue;
        }

        if (ch === '/' && next === '/') {
            state = 'lineComment';
            i++;
            continue;
        }
        if (ch === '/' && next === '*') {
            state = 'blockComment';
            i++;
            continue;
        }
        if (ch === '"' && next === '"' && next2 === '"') {
            state = 'tripleString';
            i += 2;
            continue;
        }
        if (ch === '"') {
            state = 'string';
            continue;
        }

        if (ch === '(' || ch === '[' || ch === '{') {
            stack.push({ char: ch, position });
        } else if (ch === ')' || ch === ']' || ch === '}') {
            const expected = pairs[ch];
            const top = stack.pop();
            if (!top || top.char !== expected) {
                diagnostics.push(new vscode.Diagnostic(
                    new vscode.Range(position, position.translate(0, 1)),
                    `不匹配的闭合括号 '${ch}'`,
                    vscode.DiagnosticSeverity.Error
                ));
            }
        }
    }

    for (const item of stack) {
        diagnostics.push(new vscode.Diagnostic(
            new vscode.Range(item.position, item.position.translate(0, 1)),
            `缺少 '${matchingClose(item.char)}'`,
            vscode.DiagnosticSeverity.Warning
        ));
    }
}

function collectInternalIdentifierDiagnostics(document: vscode.TextDocument, diagnostics: vscode.Diagnostic[]) {
    const text = stripCommentsAndStrings(document.getText());
    const pattern = /\b__xscript_internal_[A-Za-z0-9_]*\b/g;
    let match: RegExpExecArray | null;
    while ((match = pattern.exec(text)) !== null) {
        const start = document.positionAt(match.index);
        const end = document.positionAt(match.index + match[0].length);
        diagnostics.push(new vscode.Diagnostic(
            new vscode.Range(start, end),
            '该前缀保留给 XScript 编译器内部使用',
            vscode.DiagnosticSeverity.Error
        ));
    }
}

function findFunctionEnd(document: vscode.TextDocument, start: vscode.Position): vscode.Position {
    for (let line = start.line; line < document.lineCount; line++) {
        if (line > start.line && /^\s*function\b/.test(document.lineAt(line).text)) {
            return new vscode.Position(line - 1, document.lineAt(line - 1).text.length);
        }
    }
    const lastLine = document.lineCount - 1;
    return new vscode.Position(lastLine, document.lineAt(lastLine).text.length);
}

function matchingClose(open: string): string {
    if (open === '(') {
        return ')';
    }
    if (open === '[') {
        return ']';
    }
    return '}';
}

function stripCommentsAndStrings(text: string): string {
    let result = '';
    let state: 'normal' | 'string' | 'tripleString' | 'lineComment' | 'blockComment' = 'normal';

    for (let i = 0; i < text.length; i++) {
        const ch = text[i];
        const next = text[i + 1];
        const next2 = text[i + 2];

        if (state === 'lineComment') {
            if (ch === '\n') {
                state = 'normal';
                result += '\n';
            } else {
                result += ' ';
            }
            continue;
        }

        if (state === 'blockComment') {
            if (ch === '*' && next === '/') {
                result += '  ';
                state = 'normal';
                i++;
            } else {
                result += ch === '\n' ? '\n' : ' ';
            }
            continue;
        }

        if (state === 'string') {
            if (ch === '\\') {
                result += '  ';
                i++;
                continue;
            }
            if (ch === '"') {
                state = 'normal';
            }
            result += ch === '\n' ? '\n' : ' ';
            continue;
        }

        if (state === 'tripleString') {
            if (ch === '\\') {
                result += '  ';
                i++;
                continue;
            }
            if (ch === '"' && next === '"' && next2 === '"') {
                result += '   ';
                state = 'normal';
                i += 2;
                continue;
            }
            result += ch === '\n' ? '\n' : ' ';
            continue;
        }

        if (ch === '/' && next === '/') {
            result += '  ';
            state = 'lineComment';
            i++;
            continue;
        }
        if (ch === '/' && next === '*') {
            result += '  ';
            state = 'blockComment';
            i++;
            continue;
        }
        if (ch === '"' && next === '"' && next2 === '"') {
            result += '   ';
            state = 'tripleString';
            i += 2;
            continue;
        }
        if (ch === '"') {
            result += ' ';
            state = 'string';
            continue;
        }

        result += ch;
    }

    return result;
}
