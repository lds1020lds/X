"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.XScriptDebugSession = void 0;
const debugadapter_1 = require("@vscode/debugadapter");
const net = require("net");
const cp = require("child_process");
const path = require("path");
class XScriptDebugSession extends debugadapter_1.LoggingDebugSession {
    constructor() {
        super('xscript-debug.txt');
        this.seq = 1;
        this.pending = new Map();
        this.recvBuffer = '';
        this.variableHandles = new Map();
        this.nextVariableHandle = 1000;
        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerColumnsStartAt1(true);
    }
    initializeRequest(response, args) {
        response.body = response.body || {};
        response.body.supportsConfigurationDoneRequest = true;
        response.body.supportsEvaluateForHovers = true;
        response.body.supportsSetVariable = true;
        response.body.supportsConditionalBreakpoints = true;
        response.body.supportsStepBack = false;
        response.body.supportsDataBreakpoints = false;
        response.body.supportsCompletionsRequest = false;
        response.body.supportsCancelRequest = false;
        this.sendResponse(response);
    }
    launchRequest(response, args) {
        const program = args.program;
        const runtime = args.runtime;
        const port = Number(args.debugPort || 5678);
        if (!program || !runtime) {
            response.success = false;
            response.message = 'launch 需要 program 和 runtime';
            this.sendResponse(response);
            return;
        }
        this.sendEvent(new debugadapter_1.OutputEvent(`XScript launch: runtime=${runtime}, program=${program}, port=${port}\n`, 'console'));
        this.runtimeProcess = cp.spawn(runtime, ['--debug-port', String(port), '--wait-debugger', program], {
            cwd: path.dirname(program),
            windowsHide: true
        });
        this.runtimeProcess.stdout?.on('data', data => this.sendEvent(new debugadapter_1.OutputEvent(data.toString(), 'stdout')));
        this.runtimeProcess.stderr?.on('data', data => this.sendEvent(new debugadapter_1.OutputEvent(data.toString(), 'stderr')));
        this.runtimeProcess.on('exit', () => this.sendEvent(new debugadapter_1.TerminatedEvent()));
        this.connectWithRetry('127.0.0.1', port, 30000).then(() => {
            this.sendEvent(new debugadapter_1.OutputEvent(`XScript debug server connected: 127.0.0.1:${port}\n`, 'console'));
            this.sendResponse(response);
            this.sendEvent(new debugadapter_1.InitializedEvent());
        }).catch(error => {
            response.success = false;
            response.message = String(error);
            this.sendResponse(response);
        });
    }
    attachRequest(response, args) {
        const host = args.host || '127.0.0.1';
        const port = Number(args.port || 5678);
        this.connect(host, port).then(() => {
            this.sendResponse(response);
            this.sendEvent(new debugadapter_1.InitializedEvent());
        }).catch(error => {
            response.success = false;
            response.message = String(error);
            this.sendResponse(response);
        });
    }
    disconnectRequest(response, args) {
        this.socket?.destroy();
        this.runtimeProcess?.kill();
        this.sendResponse(response);
    }
    setBreakPointsRequest(response, args) {
        const file = args.source.path || '';
        const breakpoints = args.breakpoints || [];
        const lines = breakpoints.map(bp => Number(bp.line));
        const conditions = breakpoints.map(bp => bp.condition || '');
        this.sendVmCommand('setBreakpoints', { file, lines, conditions }).then(() => {
            response.body = {
                breakpoints: breakpoints.map(bp => ({ verified: true, line: bp.line }))
            };
            this.sendResponse(response);
        }).catch(error => {
            response.success = false;
            response.message = String(error);
            this.sendResponse(response);
        });
    }
    configurationDoneRequest(response, args) {
        this.sendVmCommand('configurationDone', {}).then(() => this.sendResponse(response)).catch(() => this.sendResponse(response));
    }
    threadsRequest(response) {
        response.body = { threads: [new debugadapter_1.Thread(XScriptDebugSession.threadId, 'XScript Main Thread')] };
        this.sendResponse(response);
    }
    continueRequest(response, args) {
        this.simpleContinueCommand(response, 'continue');
    }
    nextRequest(response, args) {
        this.simpleContinueCommand(response, 'next');
    }
    stepInRequest(response, args) {
        this.simpleContinueCommand(response, 'stepIn');
    }
    stepOutRequest(response, args) {
        this.simpleContinueCommand(response, 'stepOut');
    }
    stackTraceRequest(response, args) {
        this.sendVmCommand('stackTrace', {}).then(message => {
            const frames = message.body?.frames || [];
            response.body = {
                stackFrames: frames.map((frame) => new debugadapter_1.StackFrame(Number(frame.id || 0), frame.name || '<anonymous>', frame.file ? new debugadapter_1.Source(path.basename(frame.file), frame.file) : undefined, Number(frame.line || 1), Number(frame.column || 1))),
                totalFrames: frames.length
            };
            this.sendResponse(response);
        }).catch(error => this.failResponse(response, error));
    }
    scopesRequest(response, args) {
        const localsRef = this.createHandle({ kind: 'locals', frameId: args.frameId });
        const upvaluesRef = this.createHandle({ kind: 'upvalues', frameId: args.frameId });
        const globalsRef = this.createHandle({ kind: 'globals' });
        response.body = {
            scopes: [
                new debugadapter_1.Scope('Locals', localsRef, false),
                new debugadapter_1.Scope('Upvalues', upvaluesRef, false),
                new debugadapter_1.Scope('Globals', globalsRef, true)
            ]
        };
        this.sendResponse(response);
    }
    variablesRequest(response, args) {
        const handle = this.variableHandles.get(args.variablesReference);
        this.sendVmCommand('variables', handle || { objectId: args.variablesReference }).then(message => {
            const vars = message.body?.variables || [];
            response.body = {
                variables: vars.map((item) => ({
                    name: item.name || '',
                    value: item.value || '',
                    type: item.type || '',
                    variablesReference: item.variablesReference ? this.createHandle({ objectId: item.variablesReference }) : 0
                }))
            };
            this.sendResponse(response);
        }).catch(error => this.failResponse(response, error));
    }
    evaluateRequest(response, args) {
        this.sendVmCommand('evaluate', { expression: args.expression, frameId: args.frameId || 0, context: args.context || '' }).then(message => {
            const result = message.body?.result || {};
            response.body = {
                result: result.value || '',
                type: result.type || '',
                variablesReference: result.variablesReference ? this.createHandle({ objectId: result.variablesReference }) : 0
            };
            this.sendResponse(response);
        }).catch(error => this.failResponse(response, error));
    }
    setVariableRequest(response, args) {
        const handle = this.variableHandles.get(args.variablesReference) || {};
        this.sendVmCommand('setVariable', {
            name: args.name,
            value: args.value,
            kind: handle.kind || '',
            frameId: handle.frameId || 0,
            objectId: handle.objectId || 0
        }).then(message => {
            response.body = {
                value: message.body?.value || args.value,
                type: message.body?.type || '',
                variablesReference: message.body?.variablesReference ? this.createHandle({ objectId: message.body.variablesReference }) : 0
            };
            this.sendResponse(response);
        }).catch(error => this.failResponse(response, error));
    }
    simpleContinueCommand(response, command) {
        this.sendVmCommand(command, {}).then(() => this.sendResponse(response)).catch(error => this.failResponse(response, error));
    }
    connectWithRetry(host, port, timeoutMs) {
        const startTime = Date.now();
        let lastError;
        let reportedWaiting = false;
        return new Promise((resolve, reject) => {
            const tryConnect = () => {
                this.connect(host, port, false).then(resolve).catch(error => {
                    lastError = error;
                    if (!reportedWaiting && Date.now() - startTime > 1000) {
                        reportedWaiting = true;
                        this.sendEvent(new debugadapter_1.OutputEvent(`Waiting for XScript debug server: ${host}:${port}\n`, 'console'));
                    }
                    if (this.runtimeProcess?.exitCode !== null) {
                        reject(new Error(`XScript runtime exited before debugger connected: ${this.runtimeProcess?.exitCode}`));
                        return;
                    }
                    if (Date.now() - startTime >= timeoutMs) {
                        reject(lastError);
                        return;
                    }
                    setTimeout(tryConnect, 100);
                });
            };
            tryConnect();
        });
    }
    connect(host, port, emitTerminateOnClose = true) {
        return new Promise((resolve, reject) => {
            let connected = false;
            const socket = net.createConnection({ host, port }, () => {
                connected = true;
                this.socket = socket;
                resolve();
            });
            socket.setEncoding('utf8');
            socket.on('data', data => this.onVmData(data.toString()));
            socket.on('error', error => {
                if (!connected) {
                    socket.destroy();
                    reject(error);
                }
            });
            socket.on('close', () => {
                if (connected && emitTerminateOnClose) {
                    this.sendEvent(new debugadapter_1.TerminatedEvent());
                }
            });
        });
    }
    sendVmCommand(command, payload) {
        if (!this.socket) {
            return Promise.reject(new Error('未连接 XScript Debug Server'));
        }
        const seq = String(this.seq++);
        const msg = { seq, cmd: command, ...payload };
        return new Promise((resolve, reject) => {
            this.pending.set(seq, message => {
                if (message.success === false) {
                    reject(new Error(message.body?.message || `${command} failed`));
                    return;
                }
                resolve(message);
            });
            this.socket.write(JSON.stringify(msg) + '\n');
        });
    }
    onVmData(data) {
        this.recvBuffer += data;
        let index;
        while ((index = this.recvBuffer.indexOf('\n')) >= 0) {
            const line = this.recvBuffer.slice(0, index).trim();
            this.recvBuffer = this.recvBuffer.slice(index + 1);
            if (line.length > 0) {
                this.handleVmMessage(line);
            }
        }
    }
    handleVmMessage(line) {
        let msg;
        try {
            msg = JSON.parse(line);
        }
        catch {
            this.sendEvent(new debugadapter_1.OutputEvent(line + '\n', 'stderr'));
            return;
        }
        if (msg.type === 'event' && msg.event === 'stopped') {
            this.sendEvent(new debugadapter_1.StoppedEvent(msg.reason || 'breakpoint', XScriptDebugSession.threadId));
            return;
        }
        if (msg.type === 'response' && msg.seq) {
            const resolver = this.pending.get(msg.seq);
            if (resolver) {
                this.pending.delete(msg.seq);
                resolver(msg);
            }
        }
    }
    createHandle(value) {
        const id = this.nextVariableHandle++;
        this.variableHandles.set(id, value);
        return id;
    }
    failResponse(response, error) {
        response.success = false;
        response.message = String(error);
        this.sendResponse(response);
    }
}
exports.XScriptDebugSession = XScriptDebugSession;
XScriptDebugSession.threadId = 1;
XScriptDebugSession.run(XScriptDebugSession);
//# sourceMappingURL=debugAdapter.js.map