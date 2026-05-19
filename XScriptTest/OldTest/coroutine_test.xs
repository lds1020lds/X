// XScript 协程测试代码
// 展示协程的各种使用场景和API功能

require("debug.xs");

// ==================== 基础协程示例 ====================

// 示例1: 简单的生产者-消费者模式
function producer_consumer()
{
    var count = 0;
    while (count < 5) {
        printf("生产者生成数据:", count);
        var received = coroutine.yield(count);  // 生产数据并暂停
        printf("生产者收到反馈:", received);
        count++;
    }
    return "生产完成";
}

// 示例2: 斐波那契数列生成器
function fibonacci_generator()
{
    var a = 0, b = 1;
    while (true) {
        var next_value = coroutine.yield(a);
        if (next_value == "stop") {
            return "生成器停止";
        }
        var temp = a;
        a = b;
        b = temp + b;
    }
}

// 示例3: 带状态的状态机
function state_machine()
{
    var state = "开始";
    
    while (state != "结束") {
        printf("当前状态:", state);
        var input = coroutine.yield(state);
        
        if (state == "开始" && input == "下一步") {
            state = "处理中";
        } else if (state == "处理中" && input == "完成") {
            state = "结束";
        } else if (input == "重置") {
            state = "开始";
        }
    }
    return "状态机完成";
}

// ==================== 协程API测试 ====================

function test_coroutine_apis()
{
    printf("=== 开始协程API测试 ===");
    
    // 测试1: coroutine.create 和 coroutine.resume
    printf("\n1. 测试 coroutine.create 和 coroutine.resume:");
    var co1 = coroutine.create(producer_consumer);
    
    var status1 = coroutine.status(co1);
    printf("协程状态:", status1);
    
    var success, value = coroutine.resume(co1);
    printf("第一次resume - 成功:", success, "返回值:", value);
    
    success, value = coroutine.resume(co1, "确认收到");
    printf("第二次resume - 成功:", success, "返回值:", value);
    
    // 测试2: coroutine.wrap (更方便的API)
    printf("\n2. 测试 coroutine.wrap:");
    var wrapped_fib = coroutine.wrap(fibonacci_generator);
    
    for (i = 0; i < 10; i++) {
        var fib_value = wrapped_fib();
        printf("斐波那契数列第", i, "项:", fib_value);
    }
    
    // 停止生成器
    var stop_result = wrapped_fib("stop");
    printf("生成器停止结果:", stop_result);
    
    // 测试3: 状态机示例
    printf("\n3. 测试状态机:");
    var state_co = coroutine.create(state_machine);
    
    success, value = coroutine.resume(state_co);
    printf("初始状态:", value);
    
    success, value = coroutine.resume(state_co, "下一步");
    printf("处理状态:", value);
    
    success, value = coroutine.resume(state_co, "完成");
    printf("最终状态:", value);
    
    // 测试协程状态变化
    printf("\n4. 测试协程状态监控:");
    var final_status = coroutine.status(co1);
    printf("生产者协程最终状态:", final_status);
    
    final_status = coroutine.status(state_co);
    printf("状态机协程最终状态:", final_status);
}

// ==================== 高级协程示例 ====================

// 示例4: 协程间的协作
function worker_a()
{
    printf("Worker A 开始工作");
    coroutine.yield("A步骤1完成");
    printf("Worker A 继续工作");
    coroutine.yield("A步骤2完成");
    return "Worker A 工作完成";
}

function worker_b()
{
    printf("Worker B 开始工作");
    coroutine.yield("B步骤1完成");
    printf("Worker B 继续工作");
    coroutine.yield("B步骤2完成");
    return "Worker B 工作完成";
}

function test_coroutine_cooperation()
{
    printf("\n=== 测试协程协作 ===");
    
    var co_a = coroutine.create(worker_a);
    var co_b = coroutine.create(worker_b);
    
    // 交替执行两个协程
    var a_done = false, b_done = false;
    
    while (!a_done || !b_done) {
        if (!a_done) {
            var success, result = coroutine.resume(co_a);
            if (success) {
                printf("Worker A:", result);
                if (result == "Worker A 工作完成") {
                    a_done = true;
                }
            }
        }
        
        if (!b_done) {
            var success, result = coroutine.resume(co_b);
            if (success) {
                printf("Worker B:", result);
                if (result == "Worker B 工作完成") {
                    b_done = true;
                }
            }
        }
    }
    
    printf("所有协程工作完成");
}

// ==================== 错误处理测试 ====================

function error_prone_coroutine()
{
    printf("这个协程会抛出错误");
    coroutine.yield("第一步正常");
    
    // 模拟错误
    undefined_function();  // 这会抛出错误
    
    coroutine.yield("这一步不会执行");
}

function test_error_handling()
{
    printf("\n=== 测试错误处理 ===");
    
    var error_co = coroutine.create(error_prone_coroutine);
    
    // 第一次resume应该正常
    var success, result, error_msg = coroutine.resume(error_co);
    printf("第一次resume - 成功:", success, "结果:", result);
    
    // 第二次resume应该出错
    success, result, error_msg = coroutine.resume(error_co);
    printf("第二次resume - 成功:", success);
    if (!success) {
        printf("错误信息:", error_msg);
    }
    
    // 检查协程状态
    var status = coroutine.status(error_co);
    printf("出错后协程状态:", status);
}

// ==================== 主测试函数 ====================

function main()
{
    printf("开始XScript协程测试");
    printf("================================");
    
    test_coroutine_apis();
    test_coroutine_cooperation();
    test_error_handling();
    
    printf("================================");
    printf("协程测试完成");
    
    system_pause();
}

// 执行测试
main();