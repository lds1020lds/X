var global_default = 10;

function f(a, b = 2, c = b + global_default)
{
    printf("f", a, b, c);
    return a + b + c;
}

function g(a = 1, b = 2, ...)
{
    printf("g", a, b);
    return a + b;
}

function h(a = 1)
{
    printf("h", a);
    return a;
}

var r1 = f(1, 3, 4);
printf("r1", r1);
var r2 = f(1, 3);
printf("r2", r2);
var r3 = f(1);
printf("r3", r3);

global_default = 20;
var r4 = f(1);
printf("r4", r4);

var r5 = h(nil);
printf("r5", r5);

var r6 = g();
printf("r6", r6);
var r7 = g(5);
printf("r7", r7);
