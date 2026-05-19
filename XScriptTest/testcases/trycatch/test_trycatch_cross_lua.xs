function thrower(v)
{
    throw v;
    return 999;
}

function middle(v)
{
    return thrower(v);
}

function test_outer_catches_child()
{
    var x = 0;
    try
    {
        x = middle(77);
    }
    catch e
    {
        printf("outer caught child", e);
        x = e;
    }
    printf("cross lua result", x);
    return x;
}

var r = test_outer_catches_child();
printf("cross lua final", r);
