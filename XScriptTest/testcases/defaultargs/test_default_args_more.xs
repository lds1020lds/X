var x = 10;

function prev_ref(a, b = a + 1, c = b + 1)
{
    printf("prev_ref", a, b, c);
    return a + b + c;
}

function call_expr(a = x + 1)
{
    printf("call_expr", a);
    return a;
}

function with_varargs(a = 1, b = 2, ...)
{
    printf("with_varargs", a, b);
    return a + b;
}

function explicit_nil(a = 7)
{
    printf("explicit_nil", a);
    return a;
}

var r1 = prev_ref(3);
printf("r1", r1);

var r2 = prev_ref(3, 10);
printf("r2", r2);

x = 20;
var r3 = call_expr();
printf("r3", r3);

var r4 = call_expr(5);
printf("r4", r4);

var r5 = with_varargs();
printf("r5", r5);

var r6 = with_varargs(10, 20, 30, 40);
printf("r6", r6);

var r7 = explicit_nil(nil);
printf("r7", r7);
