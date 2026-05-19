async function fail_top()
{
    var x = nil;
    x();
    return 1;
}

var ok, err = pcall(function()
{
    return run_until_complete(fail_top());
});
printf("top reject caught", ok != 0);
