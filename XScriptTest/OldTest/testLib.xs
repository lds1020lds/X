
global gTest = 10;


function TestIf()
{
	var i = 0, k = 0, isShushu = 0;
	for (i = 2; i < gTest; i++)
	{
		isShushu = 1;
		for( k = 2; k < i; k++)
		{
			if (((i % k) == 0) && (i != k))
			{
				isShushu = 0;
				break;
			}
		}
		
		if (isShushu)
		{
		     //printf(i);
		}
	}
}


function LuaTest(s)
{
	printf(s);
}

function TestFor()
{
	var a = 0;
	var b = 0;
	for (a = 0; a <= 100; a++)
	{
		b += a;
	}
	printf(b);
}

function TestWhile()
{
	var a = 0;
	var b = 0;
	while (a <= 100)
	{
		if (a == 11)
			break;
		b += a;
		a++;
	}
	printf(b);
}

function TestRecurseCall(count)
{
	var value = 1;
	if (count > 1)
	{
		 value = TestRecurseCall(count - 1) * count;
		 printf(value);
		 return value;
	}
	else 
	{
		printf(1);
		return 1;
	}
}


function TestCallTable()
{
	var e;
	table a, b;
	a["1"] = 1;
	b["2"] = TestClass2::GetInstance();
	a["b"] = b;
	a["b"]["c"]= 3;
	e = a.b["2"]:Test(10,20); 
	LuaTest(e.x);
	LuaTest(e[0]);
	LuaTest(e[1]);
	LuaTest(e.sub.x);
	
	TestOut(e);
	printf(type(a));
	printf(type(a["b"]["c"]));
	printf(type(b["2"]));
}

