function test_metatable_full_coverage()
{

    var opMeta = {
        __add = function(a, b) { return {val = a.val + b.val}; },
    };

    function make_meta_value(val)
    {
        var obj = {val = val};
        setmetatable(obj, opMeta);
        return obj;
    }

    var a = make_meta_value(8);
    var b = make_meta_value(3);
    var c = a + b
    printf(c)
}

test_metatable_full_coverage()