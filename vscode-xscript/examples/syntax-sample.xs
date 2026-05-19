// XScript 语法高亮测试样例

require("comprehensive_tests/test_helper.xs");

var numberValue = 42;
var hexValue = 0xff;
var floatValue = 3.14;
var text = "hello\nworld";
var longText = """line1
line2 with "quotes"
line3""";
var tableValue = {name = "xscript", count = 1, ("dynamic") = true};
var listValue = [1, 2, 3];

function add(a, b)
{
    return a + b;
}

function tableValue:inc(delta)
{
    self.count += delta;
    return self;
}

var sum = add(numberValue, hexValue);
var selected = nil || tableValue.name;
var ok = (sum > 0) && (text != "");
var maxValue = (sum > 10) ? sum or 10;
var mapper = lambda x: x * 2;

for (var i = 0; i < list.size(listValue); i++)
{
    listValue[i] = mapper(listValue[i]);
}

foreach (k, v in pairs(tableValue))
{
    printf(k $ "=" $ toString(v));
}

switch (text)
{
    case "hello":
    case "hello\nworld":
        printf("matched text");
        break;
    default:
        printf("unknown text");
        break;
}

var message = f"sum={sum}, name={tableValue.name}";

function greet(name = "guest", prefix = "hello")
{
    return f"{prefix} {name}";
}

var {name: tableName, count = 0} = tableValue;
var [firstItem, secondItem] = listValue;

function useDefer()
{
    defer printf("cleanup");
    return greet(tableName);
}

try
{
    useDefer();
}
catch e
{
    printf(e);
}
finally
{
    printf("finally");
}

generator function range(start, end)
{
    var i = start;
    while (i < end)
    {
        emit i;
        i++;
    }
}

iterator(v in range(0, 3))
{
    printf(v);
}

async function loadText(path = "data.txt")
{
    var content = await read_file_async(path);
    return content;
}
