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

var singleLine = """hello""";
assertEqual(singleLine, "hello", "triple string single line");

var multiLine = """line1
line2
line3""";
assertEqual(multiLine, "line1\nline2\nline3", "triple string keeps newlines");

var quoteText = """say "hello" to XScript""";
assertEqual(quoteText, "say \"hello\" to XScript", "triple string allows double quotes");

var escaped = """tab:\t newline:\n slash:\\ end""";
assertEqual(escaped, "tab:\t newline:\n slash:\\ end", "triple string supports common escapes");

var emptyNormal = "";
assertEqual(emptyNormal, "", "normal empty string still works");

var concatenated = "prefix:" $ """A
B""" $ ":suffix";
assertEqual(concatenated, "prefix:A\nB:suffix", "triple string can concat");

function echo(value)
{
    return value;
}

assertEqual(echo("""arg
text"""), "arg\ntext", "triple string can be function argument");

var switchValue = """AA
BB""";
var matched = 0;
switch (switchValue)
{
    case "AA\nBB":
        matched = 1;
        break;
    default:
        matched = -1;
        break;
}
assertEqual(matched, 1, "triple string works with switch string case");

printf("triple string tests done");