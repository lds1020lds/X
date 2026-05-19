import {
    createConnection,
    TextDocuments,
    ProposedFeatures,
    InitializeParams,
    InitializeResult,
    TextDocumentSyncKind,
    Diagnostic,
    DiagnosticSeverity,
    DidChangeConfigurationNotification
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import * as cp from 'child_process';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

type XScriptSettings = {
    runtimePath: string;
    diagnostics: {
        enable: boolean;
        debounceMs: number;
    };
};

const defaultSettings: XScriptSettings = {
    runtimePath: '',
    diagnostics: {
        enable: true,
        debounceMs: 500
    }
};

let globalSettings: XScriptSettings = defaultSettings;
let hasConfigurationCapability = false;
let defaultRuntimePath = '';
const pendingTimers = new Map<string, NodeJS.Timeout>();

connection.onInitialize((params: InitializeParams) => {
    hasConfigurationCapability = !!params.capabilities.workspace?.configuration;
    const initOptions = params.initializationOptions as { defaultRuntimePath?: string } | undefined;
    defaultRuntimePath = initOptions?.defaultRuntimePath || '';

    const result: InitializeResult = {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Incremental
        }
    };
    return result;
});

connection.onInitialized(() => {
    if (hasConfigurationCapability) {
        connection.client.register(DidChangeConfigurationNotification.type, undefined);
        refreshSettings().catch(error => {
            connection.console.error(String(error));
        });
    }
});

connection.onDidChangeConfiguration(() => {
    refreshSettings().catch(error => {
        connection.console.error(String(error));
    });
});

async function refreshSettings(): Promise<void> {
    if (!hasConfigurationCapability) {
        return;
    }

    const settings = await connection.workspace.getConfiguration('xscript');
    globalSettings = normalizeSettings(settings);
    for (const document of documents.all()) {
        scheduleValidation(document);
    }
}

documents.onDidOpen(event => scheduleValidation(event.document));
documents.onDidChangeContent(event => scheduleValidation(event.document));
documents.onDidClose(event => {
    const timer = pendingTimers.get(event.document.uri);
    if (timer) {
        clearTimeout(timer);
        pendingTimers.delete(event.document.uri);
    }
    connection.sendDiagnostics({ uri: event.document.uri, diagnostics: [] });
});

function normalizeSettings(raw: any): XScriptSettings {
    return {
        runtimePath: typeof raw?.runtimePath === 'string' ? raw.runtimePath : defaultSettings.runtimePath,
        diagnostics: {
            enable: typeof raw?.diagnostics?.enable === 'boolean' ? raw.diagnostics.enable : defaultSettings.diagnostics.enable,
            debounceMs: typeof raw?.diagnostics?.debounceMs === 'number' ? raw.diagnostics.debounceMs : defaultSettings.diagnostics.debounceMs
        }
    };
}

function scheduleValidation(document: TextDocument) {
    const oldTimer = pendingTimers.get(document.uri);
    if (oldTimer) {
        clearTimeout(oldTimer);
    }

    const delay = Math.max(0, globalSettings.diagnostics.debounceMs || 0);
    const timer = setTimeout(() => {
        pendingTimers.delete(document.uri);
        validateTextDocument(document).catch(error => {
            connection.console.error(String(error));
        });
    }, delay);
    pendingTimers.set(document.uri, timer);
}

async function validateTextDocument(document: TextDocument): Promise<void> {
    if (!globalSettings.diagnostics.enable) {
        connection.sendDiagnostics({ uri: document.uri, diagnostics: [] });
        return;
    }

    const runtimePath = resolveRuntimePath();
    if (!runtimePath || !fs.existsSync(runtimePath)) {
        connection.sendDiagnostics({
            uri: document.uri,
            diagnostics: [createWholeFileDiagnostic(document, `找不到 XScripter.exe。请设置 xscript.runtimePath，当前路径：${runtimePath || '(empty)'}`)]
        });
        return;
    }

    const tempFile = writeTempDocument(document);
    try {
        const result = runSyntaxCheck(runtimePath, tempFile);
        const diagnostics = parseDiagnostics(document, result.stdout + '\n' + result.stderr);
        connection.sendDiagnostics({ uri: document.uri, diagnostics });
    } finally {
        try {
            fs.unlinkSync(tempFile);
        } catch {
            // 忽略临时文件删除失败
        }
    }
}

function resolveRuntimePath(): string {
    if (globalSettings.runtimePath && globalSettings.runtimePath.trim().length > 0) {
        return globalSettings.runtimePath;
    }
    return defaultRuntimePath;
}

function writeTempDocument(document: TextDocument): string {
    const dir = path.join(os.tmpdir(), 'xscript-lsp');
    fs.mkdirSync(dir, { recursive: true });
    const fileName = `check-${process.pid}-${Date.now()}-${Math.random().toString(16).slice(2)}.xs`;
    const filePath = path.join(dir, fileName);
    fs.writeFileSync(filePath, document.getText(), 'utf8');
    return filePath;
}

function runSyntaxCheck(runtimePath: string, filePath: string): { stdout: string; stderr: string; status: number | null } {
    const proc = cp.spawnSync(runtimePath, ['--syntax-only', '--recover', filePath], {
        encoding: 'utf8',
        cwd: path.dirname(runtimePath),
        windowsHide: true,
        timeout: 10000
    });

    if (proc.error) {
        return {
            stdout: '',
            stderr: proc.error.message,
            status: proc.status ?? 1
        };
    }

    return {
        stdout: proc.stdout || '',
        stderr: proc.stderr || '',
        status: proc.status
    };
}

function parseDiagnostics(document: TextDocument, output: string): Diagnostic[] {
    const diagnostics: Diagnostic[] = [];
    const syntaxPattern = /Syntax Error:(.*?) at line\((\d+)\),char\((\d+)\)/g;
    let match: RegExpExecArray | null;
    while ((match = syntaxPattern.exec(output)) !== null) {
        diagnostics.push(createPointDiagnostic(document, Number(match[2]), Number(match[3]), match[1].trim()));
    }

    if (diagnostics.length === 0) {
        const lexPattern = /Lex Error:(.*?) at line\((\d+)\),char\((\d+)\)/g;
        while ((match = lexPattern.exec(output)) !== null) {
            diagnostics.push(createPointDiagnostic(document, Number(match[2]), Number(match[3]), match[1].trim()));
        }
    }

    return diagnostics;
}

function createPointDiagnostic(document: TextDocument, parserLine: number, parserChar: number, message: string): Diagnostic {
    const line = clamp(parserLine - 1, 0, Math.max(0, document.lineCount - 1));
    const lineText = document.getText({ start: { line, character: 0 }, end: { line: line + 1, character: 0 } }).replace(/\r?\n$/, '');
    const character = clamp(parserChar, 0, Math.max(0, lineText.length));
    const endCharacter = Math.min(character + 1, Math.max(character, lineText.length));
    return {
        severity: DiagnosticSeverity.Error,
        range: {
            start: { line, character },
            end: { line, character: endCharacter }
        },
        message,
        source: 'xscript-parser'
    };
}

function createWholeFileDiagnostic(document: TextDocument, message: string): Diagnostic {
    const line = 0;
    const lineText = document.lineCount > 0
        ? document.getText({ start: { line: 0, character: 0 }, end: { line: 1, character: 0 } }).replace(/\r?\n$/, '')
        : '';
    return {
        severity: DiagnosticSeverity.Warning,
        range: {
            start: { line, character: 0 },
            end: { line, character: Math.max(1, lineText.length) }
        },
        message,
        source: 'xscript-lsp'
    };
}

function clamp(value: number, min: number, max: number): number {
    return Math.max(min, Math.min(max, value));
}

documents.listen(connection);
connection.listen();
