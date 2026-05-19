function iterValue(a, b)
{
    if (b == nil)
        return a;

    return b;
}

function iterKey(a, b)
{
    if (b == nil)
        return nil;

    return a;
}

function range(start, end, step)
{
    if (step == nil)
        step = 1;

    var result = [];
    var current = start;

    while (1)
    {
        if (step > 0)
        {
            if (current >= end)
                break;
        }
        else
        {
            if (current <= end)
                break;
        }

        result:append(current);
        current += step;
    }

    return result;
}

function collect(source)
{
    var result = [];

    iterator(a, b in source)
    {
        result:append(iterValue(a, b));
    }

    return result;
}

function map(source, fn)
{
    var result = [];

    iterator(a, b in source)
    {
        var value = iterValue(a, b);
        var key = iterKey(a, b);
        result:append(fn(value, key));
    }

    return result;
}

function filter(source, fn)
{
    var result = [];

    iterator(a, b in source)
    {
        var value = iterValue(a, b);
        var key = iterKey(a, b);
        if (fn(value, key))
            result:append(value);
    }

    return result;
}

function reduce(source, fn, init)
{
    var acc = init;
    var hasAcc = 0;

    if (init != nil)
        hasAcc = 1;

    iterator(a, b in source)
    {
        var value = iterValue(a, b);
        var key = iterKey(a, b);

        if (hasAcc)
        {
            acc = fn(acc, value, key);
        }
        else
        {
            acc = value;
            hasAcc = 1;
        }
    }

    return acc;
}

function each(source, fn)
{
    var count = 0;

    iterator(a, b in source)
    {
        var value = iterValue(a, b);
        var key = iterKey(a, b);
        fn(value, key);
        count++;
    }

    return count;
}

function enumerate(source)
{
    var result = [];
    var index = 0;

    iterator(a, b in source)
    {
        var value = iterValue(a, b);
        var key = iterKey(a, b);
        result:append({ index = index, key = key, value = value });
        index++;
    }

    return result;
}

function zip(left, right)
{
    var result = [];
    var leftIter = left:__iterator__();
    var rightIter = right:__iterator__();

    while (1)
    {
        var leftOk, leftA, leftB = leftIter:__next__();
        var rightOk, rightA, rightB = rightIter:__next__();

        if (leftOk == 0)
            break;

        if (rightOk == 0)
            break;

        result:append({ left = iterValue(leftA, leftB), right = iterValue(rightA, rightB) });
    }

    return result;
}

var exports =
{
    range = range,
    collect = collect,
    map = map,
    filter = filter,
    reduce = reduce,
    each = each,
    enumerate = enumerate,
    zip = zip
};

settable(exports, "foreach", each);

return exports;
