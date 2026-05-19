function assertEqual(a, b, msg)
{
    if (a != b)
    {
        printf("FAIL", msg, "expected", b, "got", a);
    }
    else
    {
        printf("OK", msg);
    }
}

function makeRange(start, end, step)
{
    if (step == nil)
        step = 1;

    return
    {
        __iterator__ = function(self)
        {
            var index = start;

            var iter =
            {
                __next__ = function(t)
                {
                    if (index < end)
                    {
                        var value = index;
                        index += step;
                        return 1, value;
                    }

                    return 0;
                }
            };

            return iter;
        }
    };
}

function makePairs(count)
{
    return
    {
        __iterator__ = function(self)
        {
            var index = 0;

            return
            {
                __next__ = function(t)
                {
                    if (index < count)
                    {
                        index++;
                        return 1, index, index * 10;
                    }

                    return 0;
                }
            };
        }
    };
}

function makeNilValues()
{
    return
    {
        __iterator__ = function(self)
        {
            var index = 0;

            return
            {
                __next__ = function(t)
                {
                    index++;

                    if (index == 1)
                        return 1, nil;

                    if (index == 2)
                        return 1, "after nil";

                    return 0;
                }
            };
        }
    };
}

function makeSelfIterator(end)
{
    return
    {
        index = 0,
        __iterator__ = function(self)
        {
            return self;
        },
        __next__ = function(self)
        {
            if (self.index < end)
            {
                self.index = self.index + 1;
                return 1, self.index;
            }

            return 0;
        }
    };
}

generator function genRange(start, end, step)
{
    if (step == nil)
        step = 1;

    var i = start;
    while (i < end)
    {
        emit i;
        i += step;
    }
}

generator function genPairs(count)
{
    var i = 0;
    while (i < count)
    {
        i++;
        emit i, i + 100;
    }
}

generator function genWithReturn()
{
    emit 1;
    return 999;
}

var gSourceCallCount = 0;

function makeSourceOnce()
{
    gSourceCallCount++;
    return makeRange(1, 4, 1);
}


function test_list_basic()
{
    printf("--- test_list_basic ---");

    var sum = 0;
    var text = "";
    var count = 0;

    iterator(i in [1, 2, 3, 4])
    {
        sum += i;
        text $= toString(i);
        count++;
    }

    assertEqual(sum, 10, "list sum");
    assertEqual(text, "1234", "list order");
    assertEqual(count, 4, "list count");
}

function test_list_empty()
{
    printf("--- test_list_empty ---");

    var count = 0;

    iterator(i in [])
    {
        count++;
    }

    assertEqual(count, 0, "empty list count");
}

function test_list_nil_value()
{
    printf("--- test_list_nil_value ---");

    var count = 0;
    var nilCount = 0;
    var sum = 0;

    iterator(i in [1, nil, 3])
    {
        count++;
        if (i == nil)
            nilCount++;
        else
            sum += i;
    }

    assertEqual(count, 3, "list nil keeps count");
    assertEqual(nilCount, 1, "list nil payload");
    assertEqual(sum, 4, "list nil sum");
}

function test_custom_range()
{
    printf("--- test_custom_range ---");

    var result = "";
    var sum = 0;

    iterator(i in makeRange(1, 8, 2))
    {
        result $= toString(i);
        sum += i;
    }

    assertEqual(result, "1357", "custom range order");
    assertEqual(sum, 16, "custom range sum");
}

function test_multi_return_iterator()
{
    printf("--- test_multi_return_iterator ---");

    var keyText = "";
    var valueSum = 0;

    iterator(k, v in makePairs(3))
    {
        keyText $= toString(k);
        valueSum += v;
    }

    assertEqual(keyText, "123", "multi return keys");
    assertEqual(valueSum, 60, "multi return values");
}

function test_extra_iteration_variable_gets_nil()
{
    printf("--- test_extra_iteration_variable_gets_nil ---");

    var nilCount = 0;
    var sum = 0;

    iterator(a, b in makeRange(1, 4, 1))
    {
        sum += a;
        if (b == nil)
            nilCount++;
    }

    assertEqual(sum, 6, "extra var first value");
    assertEqual(nilCount, 3, "extra var nil");
}

function test_nil_payload_not_end()
{
    printf("--- test_nil_payload_not_end ---");

    var count = 0;
    var firstIsNil = 0;
    var second = "";

    iterator(v in makeNilValues())
    {
        count++;
        if (count == 1 && v == nil)
            firstIsNil = 1;
        if (count == 2)
            second = v;
    }

    assertEqual(count, 2, "nil payload does not stop");
    assertEqual(firstIsNil, 1, "first payload nil");
    assertEqual(second, "after nil", "second payload after nil");
}

function test_self_iterator()
{
    printf("--- test_self_iterator ---");

    var result = "";

    iterator(i in makeSelfIterator(3))
    {
        result $= toString(i);
    }

    assertEqual(result, "123", "self iterator result");
}

function test_break_and_continue()
{
    printf("--- test_break_and_continue ---");

    var result = "";

    iterator(i in [1, 2, 3, 4, 5])
    {
        if (i == 2)
            continue;

        if (i == 5)
            break;

        result $= toString(i);
    }

    assertEqual(result, "134", "iterator break continue");
}

function test_nested_iterator()
{
    printf("--- test_nested_iterator ---");

    var result = "";

    iterator(i in [1, 2])
    {
        iterator(j in [3, 4])
        {
            result $= toString(i) $ ":" $ toString(j) $ ";";
        }
    }

    assertEqual(result, "1:3;1:4;2:3;2:4;", "nested iterator result");
}

function test_source_expression_eval_once()
{
    printf("--- test_source_expression_eval_once ---");

    gSourceCallCount = 0;
    var sum = 0;

    iterator(i in makeSourceOnce())
    {
        sum += i;
    }

    assertEqual(gSourceCallCount, 1, "iterator source eval once");
    assertEqual(sum, 6, "iterator source eval once sum");
}

function test_manual_protocol_call()
{
    printf("--- test_manual_protocol_call ---");

    var list = [7, 8];
    var it = list:__iterator__();

    var ok1, v1 = it.__next__(it);
    var ok2, v2 = it.__next__(it);
    var ok3, v3 = it.__next__(it);

    assertEqual(ok1, 1, "manual list ok1");
    assertEqual(v1, 7, "manual list v1");
    assertEqual(ok2, 1, "manual list ok2");
    assertEqual(v2, 8, "manual list v2");
    assertEqual(ok3, 0, "manual list ok3 end");
    assertEqual(v3, nil, "manual list end value nil");
}

function test_table_basic_iterator()
{
    printf("--- test_table_basic_iterator ---");

    var t = { a = 10, b = 20 };
    t[0] = 5;

    var count = 0;
    var sum = 0;
    var keyText = "";

    iterator(k, v in t)
    {
        count++;
        sum += v;
        keyText $= toString(k) $ ";";
    }

    assertEqual(count, 3, "table iterator count");
    assertEqual(sum, 35, "table iterator values");
    assertEqual(table.contains(t, "__iterator__"), 0, "table iterator not stored in table");
}

function test_table_empty_iterator()
{
    printf("--- test_table_empty_iterator ---");

    var count = 0;

    iterator(k, v in {})
    {
        count++;
    }

    assertEqual(count, 0, "empty table iterator count");
}

function test_table_nil_value_iterator()
{
    printf("--- test_table_nil_value_iterator ---");

    var t = { "before nil", nil, "after nil" };
    var count = 0;
    var nilCount = 0;
    var text = "";

    iterator(k, v in t)
    {
        count++;
        if (v == nil)
            nilCount++;
        else
            text $= v;
    }

    assertEqual(count, 2, "table iterator skips nil value");
    assertEqual(nilCount, 0, "table iterator does not emit nil payload");
    assertEqual(text, "before nilafter nil", "table iterator keeps non-nil values");
}

function test_table_manual_protocol_call()
{
    printf("--- test_table_manual_protocol_call ---");

    var t = { a = 1 };
    var it = t:__iterator__();

    var ok1, k1, v1 = it:__next__();
    var ok2, k2, v2 = it:__next__();

    assertEqual(ok1, 1, "manual table ok1");
    assertEqual(k1, "a", "manual table key1");
    assertEqual(v1, 1, "manual table value1");
    assertEqual(ok2, 0, "manual table ok2 end");
    assertEqual(k2, nil, "manual table end key nil");
    assertEqual(v2, nil, "manual table end value nil");
}

function test_generator_range()
{
    printf("--- test_generator_range ---");

    var result = "";
    var sum = 0;

    iterator(i in genRange(2, 9, 3))
    {
        result $= toString(i);
        sum += i;
    }

    assertEqual(result, "258", "generator range order");
    assertEqual(sum, 15, "generator range sum");
}

function test_generator_multi_emit()
{
    printf("--- test_generator_multi_emit ---");

    var keys = "";
    var values = "";

    iterator(k, v in genPairs(3))
    {
        keys $= toString(k);
        values $= toString(v) $ ";";
    }

    assertEqual(keys, "123", "generator multi emit keys");
    assertEqual(values, "101;102;103;", "generator multi emit values");
}

function test_generator_return_not_iteration_value()
{
    printf("--- test_generator_return_not_iteration_value ---");

    var count = 0;
    var sum = 0;

    iterator(i in genWithReturn())
    {
        count++;
        sum += i;
    }

    assertEqual(count, 1, "generator return not count");
    assertEqual(sum, 1, "generator return not value");
}

function test_generator_manual_next_after_end()
{
    printf("--- test_generator_manual_next_after_end ---");

    var it = genRange(1, 3, 1);

    var ok1, v1 = it.__next__(it);
    var ok2, v2 = it.__next__(it);
    var ok3, v3 = it.__next__(it);
    var ok4, v4 = it.__next__(it);

    assertEqual(ok1, 1, "generator manual ok1");
    assertEqual(v1, 1, "generator manual v1");
    assertEqual(ok2, 1, "generator manual ok2");
    assertEqual(v2, 2, "generator manual v2");
    assertEqual(ok3, 0, "generator manual ok3 end");
    assertEqual(v3, nil, "generator manual v3 nil");
    assertEqual(ok4, 0, "generator manual ok4 still end");
    assertEqual(v4, nil, "generator manual v4 nil");
}

function run_all()
{
    test_list_basic();
    test_list_empty();
    test_list_nil_value();
    test_custom_range();
    test_multi_return_iterator();
    test_extra_iteration_variable_gets_nil();
    test_nil_payload_not_end();
    test_self_iterator();
    test_break_and_continue();
    test_nested_iterator();
    test_source_expression_eval_once();
    test_manual_protocol_call();
    test_table_basic_iterator();
    test_table_empty_iterator();
    test_table_nil_value_iterator();
    test_table_manual_protocol_call();
    test_generator_range();
    test_generator_multi_emit();
    test_generator_return_not_iteration_value();
    test_generator_manual_next_after_end();

    printf("--- iterator tests done ---");
}

run_all();
