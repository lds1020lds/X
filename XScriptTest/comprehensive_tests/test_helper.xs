// ============================================================
// XScript 综合测试 - 公共辅助函数
// ============================================================

gPassCount = 0;
gFailCount = 0;

// 简单断言辅助函数
function assert(condition, desc)
{
    if (condition)
    {
        gPassCount++;
    }
    else
    {
        gFailCount++;
        printf("[FAIL] " $ desc);
    }
}

function assertEqual(actual, expected, desc)
{
    if (actual == expected)
    {
        gPassCount++;
    }
    else
    {
        gFailCount++;
        printf("[FAIL] " $ desc $ " expected=" $ toString(expected) $ " actual=" $ toString(actual));
    }
}

function summary()
{
    printf("========================================");
    printf("Test Summary: " $ toString(gPassCount) $ " passed, " $ toString(gFailCount) $ " failed");
    printf("========================================");
}
