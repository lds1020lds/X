import { LoggingDebugSession, InitializedEvent, StoppedEvent, TerminatedEvent, OutputEvent, Thread, StackFrame, Scope, Source } from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import * as net from 'net';
import * as cp from 'child_process';
import * as path from 'path';

interface VmMessage {
    type?: string;
    event?: string;
    reason?: string;
    seq?: string;
    cmd?: string;
    success?: boolean;
    body?: any;
    file?: string;
    line?: number;
}

export class XScriptDebugSession extends LoggingDebugSession {
    private static threadId = 1;
    private socket?: net.Socket;
    private runtimeProcess?: cp.ChildProcess;
    private seq = 1;
    private pending = new Map<string, (message: VmMessage) => void>();
    private recvBuffer = '';
    private variableHandles = new Map<number, any>();
    private nextVariableHandle = 1000;

    public constructor() {
        super('xscript-debug.txt');
        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerColumnsStartAt1(true);
    }

    protected initializeRequest(response: DebugProtocol.InitializeResponse, args: DebugProtocol.InitializeRequestArguments): void {
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

    protected launchRequest(response: DebugProtocol.LaunchResponse, args: any): void {
        const program = args.program;
        const runtime = args.runtime;
        const port = Number(args.debugPort || 5678);
        if (!program || !runtime) {
            response.success = false;
            response.message = 'launch 需要 program 和 runtime';
            this.sendResponse(response);
            return;
        }

        this.sendEvent(new OutputEvent(`XScript launch: runtime=${runtime}, program=${program}, port=${port}\n`, 'console'));
        this.runtimeProcess = cp.spawn(runtime, ['--debug-port', String(port), '--wait-debugger', program], {
            cwd: path.dirname(program),
            windowsHide: true
        });
        this.runtimeProcess.stdout?.on('data', data => this.sendEvent(new OutputEvent(data.toString(), 'stdout')));
        this.runtimeProcess.stderr?.on('data', data => this.sendEvent(new OutputEvent(data.toString(), 'stderr')));
        this.runtimeProcess.on('exit', () => this.sendEvent(new TerminatedEvent()));

        this.connectWithRetry('127.0.0.1', port, 30000).then(() => {
            this.sendEvent(new OutputEvent(`XScript debug server connected: 127.0.0.1:${port}\n`, 'console'));
            this.sendResponse(response);
            this.sendEvent(new InitializedEvent());
        }).catch(error => {
            response.success = false;
            response.message = String(error);
            this.sendResponse(response);
        });
    }

    protected attachRequest(response: DebugProtocol.AttachResponse, args: any): void {
        const host = args.host || '127.0.0.1';
        const port = Number(args.port || 5678);
        this.connect(host, port).then(() => {
            this.sendResponse(response);
            this.sendEvent(new InitializedEvent());
        }).catch(error => {
            response.success = false;
            response.message = String(error);
            this.sendResponse(response);
        });
    }

    protected disconnectRequest(response: DebugProtocol.DisconnectResponse, args: DebugProtocol.DisconnectArguments): void {
        this.socket?.destroy();
        this.runtimeProcess?.kill();
        this.sendResponse(response);
    }

    protected setBreakPointsRequest(response: DebugProtocol.SetBreakpointsResponse, args: DebugProtocol.SetBreakpointsArguments): void {
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

    protected configurationDoneRequest(response: DebugProtocol.ConfigurationDoneResponse, args: DebugProtocol.ConfigurationDoneArguments): void {
        this.sendVmCommand('configurationDone', {}).then(() => this.sendResponse(response)).catch(() => this.sendResponse(response));
    }

    protected threadsRequest(response: DebugProtocol.ThreadsResponse): void {
        response.body = { threads: [new Thread(XScriptDebugSession.threadId, 'XScript Main Thread')] };
        this.sendResponse(response);
    }

    protected continueRequest(response: DebugProtocol.ContinueResponse, args: DebugProtocol.ContinueArguments): void {
        this.simpleContinueCommand(response, 'continue');
    }

    protected nextRequest(response: DebugProtocol.NextResponse, args: DebugProtocol.NextArguments): void {
        this.simpleContinueCommand(response, 'next');
    }

    protected stepInRequest(response: DebugProtocol.StepInResponse, args: DebugProtocol.StepInArguments): void {
        this.simpleContinueCommand(response, 'stepIn');
    }

    protected stepOutRequest(response: DebugProtocol.StepOutResponse, args: DebugProtocol.StepOutArguments): void {
        this.simpleContinueCommand(response, 'stepOut');
    }

    protected stackTraceRequest(response: DebugProtocol.StackTraceResponse, args: DebugProtocol.StackTraceArguments): void {
        this.sendVmCommand('stackTrace', {}).then(message => {
            const frames = message.body?.frames || [];
            response.body = {
                stackFrames: frames.map((frame: any) => new StackFrame(
                    Number(frame.id || 0),
                    frame.name || '<anonymous>',
                    frame.file ? new Source(path.basename(frame.file), frame.file) : undefined,
                    Number(frame.line || 1),
                    Number(frame.column || 1)
                )),
                totalFrames: frames.length
            };
            this.sendResponse(response);
        }).catch(error => this.failResponse(response, error));
    }

    protected scopesRequest(response: DebugProtocol.ScopesResponse, args: DebugProtocol.ScopesArguments): void {
        const localsRef = this.createHandle({ kind: 'locals', frameId: args.frameId });
        const upvaluesRef = this.createHandle({ kind: 'upvalues', frameId: args.frameId });
        const globalsRef = this.createHandle({ kind: 'globals' });
        response.body = {
            scopes: [
                new Scope('Locals', localsRef, false),
                new Scope('Upvalues', upvaluesRef, false),
                new Scope('Globals', globalsRef, true)
            ]
        };
        this.sendResponse(response);
    }

    protected variablesRequest(response: DebugProtocol.VariablesResponse, args: DebugProtocol.VariablesArguments): void {
        const handle = this.variableHandles.get(args.variablesReference);
        this.sendVmCommand('variables', handle || { objectId: args.variablesReference }).then(message => {
            const vars = message.body?.variables || [];
            response.body = {
                variables: vars.map((item: any) => ({
                    name: item.name || '',
                    value: item.value || '',
                    type: item.type || '',
                    variablesReference: item.variablesReference ? this.createHandle({ objectId: item.variablesReference }) : 0
                }))
            };
            this.sendResponse(response);
        }).catch(error => this.failResponse(response, error));
    }

    protected evaluateRequest(response: DebugProtocol.EvaluateResponse, args: DebugProtocol.EvaluateArguments): void {
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

    protected setVariableRequest(response: DebugProtocol.SetVariableResponse, args: DebugProtocol.SetVariableArguments): void {
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

    private simpleContinueCommand(response: DebugProtocol.Response, command: string): void {
        this.sendVmCommand(command, {}).then(() => this.sendResponse(response)).catch(error => this.failResponse(response, error));
    }

    private connectWithRetry(host: string, port: number, timeoutMs: number): Promise<void> {
        const startTime = Date.now();
        let lastError: any;
        let reportedWaiting = false;

        return new Promise((resolve, reject) => {
            const tryConnect = () => {
                this.connect(host, port, false).then(resolve).catch(error => {
                    lastError = error;
                    if (!reportedWaiting && Date.now() - startTime > 1000) {
                        reportedWaiting = true;
                        this.sendEvent(new OutputEvent(`Waiting for XScript debug server: ${host}:${port}\n`, 'console'));
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

    private connect(host: string, port: number, emitTerminateOnClose = true): Promise<void> {
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
                    this.sendEvent(new TerminatedEvent());
                }
            });
        });
    }

    private sendVmCommand(command: string, payload: any): Promise<VmMessage> {
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
            this.socket!.write(JSON.stringify(msg) + '\n');
        });
    }

    private onVmData(data: string): void {
        this.recvBuffer += data;
        let index: number;
        while ((index = this.recvBuffer.indexOf('\n')) >= 0) {
            const line = this.recvBuffer.slice(0, index).trim();
            this.recvBuffer = this.recvBuffer.slice(index + 1);
            if (line.length > 0) {
                this.handleVmMessage(line);
            }
        }
    }

    private handleVmMessage(line: string): void {
        let msg: VmMessage;
        try {
            msg = JSON.parse(line);
        } catch {
            this.sendEvent(new OutputEvent(line + '\n', 'stderr'));
            return;
        }

        if (msg.type === 'event' && msg.event === 'stopped') {
            this.sendEvent(new StoppedEvent(msg.reason || 'breakpoint', XScriptDebugSession.threadId));
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

    private createHandle(value: any): number {
        const id = this.nextVariableHandle++;
        this.variableHandles.set(id, value);
        return id;
    }

    private failResponse(response: DebugProtocol.Response, error: any): void {
        response.success = false;
        response.message = String(error);
        this.sendResponse(response);
    }
}

XScriptDebugSession.run(XScriptDebugSession);
