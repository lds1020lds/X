function failTest(msg, expected, got)
{
    printf("FAIL", msg, "expected", expected, "got", got);
    __return_regs_test_failed__(msg);
}

function assertEqual(a, b, msg)
{
    if (a != b)
    {
        failTest(msg, b, a);
    }
    else
    {
        printf("OK", msg);
    }
}

function leftValue()
{
    return "L";
}

function rightValue()
{
    return "R";
}

function middleValue()
{
    return "M";
}

function makePair()
{
    return leftValue(), rightValue();
}

function join2(a, b)
{
    return a $ b;
}

function join3(a, b, c)
{
    return a $ b $ c;
}

function makeAdder(prefix)
{
    return function(value)
    {
        return prefix $ value;
    };
}

function makeObj(prefix)
{
    return {
        prefix = prefix,
        combine = function(self, value)
        {
            return self.prefix $ value;
        }
    };
}

function test_binary_expression_return_regs()
{
    printf("--- test_binary_expression_return_regs ---");

    assertEqual(leftValue() $ rightValue(), "LR", "binary concat keeps left function return");
    assertEqual(leftValue() $ middleValue() $ rightValue(), "LMR", "concat chain keeps previous function returns");
    assertEqual(join2(leftValue(), rightValue()), "LR", "call args push function returns immediately");
    assertEqual(join3(leftValue(), middleValue(), rightValue()), "LMR", "multi call args keep order");
}

function test_fstring_return_regs()
{
    printf("--- test_fstring_return_regs ---");

    assertEqual(f"{leftValue()}{rightValue()}", "LR", "adjacent f-string function returns");
    assertEqual(f"[{leftValue()}][{middleValue()}][{rightValue()}]", "[L][M][R]", "f-string multiple function returns with literals");
    assertEqual(f"nested={join2(leftValue(), rightValue())}", "nested=LR", "f-string nested call args");
}

function test_multi_return_regs()
{
    printf("--- test_multi_return_regs ---");

    var a, b = leftValue(), rightValue();
    assertEqual(a, "L", "multi assignment first return expression");
    assertEqual(b, "R", "multi assignment second return expression");

    var p, q = makePair();
    assertEqual(p, "L", "multi return first value");
    assertEqual(q, "R", "multi return second value");
}

function returnAdjacentValues()
{
    return leftValue(), rightValue();
}

function test_return_statement_regs()
{
    printf("--- test_return_statement_regs ---");

    var a, b = returnAdjacentValues();
    assertEqual(a, "L", "return statement keeps first function return");
    assertEqual(b, "R", "return statement keeps second function return");
}

function test_callee_return_regs()
{
    printf("--- test_callee_return_regs ---");

    assertEqual(makeAdder(leftValue())(rightValue()), "LR", "function return as callee survives argument call");
    assertEqual(makeObj(leftValue()):combine(rightValue()), "LR", "method callee survives argument call");
}

function makeIteratorFromReturns(first, second)
{
    return {
        __iterator__ = function(self)
        {
            var index = 0;
            return {
                __next__ = function(t)
                {
                    index = index + 1;
                    if (index == 1)
                        return 1, first;
                    if (index == 2)
                        return 1, second;
                    return 0;
                }
            };
        }
    };
}

function test_iterator_return_regs()
{
    printf("--- test_iterator_return_regs ---");

    var count = 0;
    var text = "";
    iterator(item in makeIteratorFromReturns(leftValue(), rightValue()))
    {
        text = text $ item;
        count = count + 1;
    }

    assertEqual(count, 2, "iterator object with function returns count");
    assertEqual(text, "LR", "iterator object with function returns order");
}

test_binary_expression_return_regs();
test_fstring_return_regs();
test_multi_return_regs();
test_return_statement_regs();
test_callee_return_regs();
test_iterator_return_regs();

printf("return register tests done");