
function pa()
{
	var a, b = 1, 2;
	a += b;
	coroutine.yield(b, a);
	a += b;
}

function fib(n)
{
	var a, b = 1, 2;
	a += b;
	//printf(a);
	//printf("fib yield");
	var ya, yb = coroutine.yield(b, a);
	//printf("ya, yb:", ya, yb);
	a += n;
	var ret, f = loadstring("pa()");
	f();
	//printf(a);
}
		
		
function TestWhile(a1, a2)
{
	//printf("start args:", a1, a2);
	var a = 0;
	var b = 0;
	while (a <= 10000)
	{
		if (a == 1600)
			break;
		b += a;
		a++;
	}
	//printf(b);
	//printf("TestWhile yield");
	var ya, yb = coroutine.yield(b, a);
	//printf("ya, yb:", ya, yb);
	a = 0;
	b = 0;
	while (a <= 100)
	{
		if (a == 1600)
			break;
		b += a;
		a++;
	}
	fib(5);
	
	//printf(b);
	//printf("coroutine finished");
	
}


var co = coroutine.create(TestWhile);
//printf("coroutine.resume1");
var ret, ra, rb = coroutine.resume(co, 10, 20);
//printf("ret, ra, rb", ret, ra, rb);

//printf("coroutine.resume2");
ret, ra, rb = coroutine.resume(co, 100, 200);
//printf("ret, ra, rb", ret, ra, rb);
//printf("coroutine.resume3");

ret, ra, rb = coroutine.resume(co, 1000, 2000);
//printf("ret, ra, rb", ret, ra, rb);
ret, ra, rb = coroutine.resume(co, 1, 2);
//printf("ret, ra, rb", ret, ra, rb);
ret, ra, rb = coroutine.resume(co, 1, 2);
//printf("ret, ra, rb", ret, ra, rb);

//printf("---------------------------");

coFunc = coroutine.wrap(TestWhile);
//printf("coroutine.resume1");
var ret, ra, rb = coFunc(10, 20);
//printf("ret, ra, rb", ret, ra, rb);

//printf("coroutine.resume2");
ret, ra, rb = coFunc(100, 200);
//printf("ret, ra, rb", ret, ra, rb);
//printf("coroutine.resume3");

ret, ra, rb = coFunc(1000, 2000);
//printf("ret, ra, rb", ret, ra, rb);
ret, ra, rb = coFunc(1, 2);
//printf("ret, ra, rb", ret, ra, rb);
ret, ra, rb = coFunc(1, 2);
//printf("ret, ra, rb", ret, ra, rb);

var a1, a2, a3, a4 = 3,4,5,6;
a1 = a1 + a2 + a3;
//printf(a1);

system_pause();
