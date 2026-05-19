gTest = 100;


function div(a, b)
{
	return a / b;
}
function Add(a, b)
{
	return a + b;
}

function max(a, b, c)
{
	if ((a > b) && (a > c))
	{
		return a;
	}
	else if ((b > c) && (b > a))
	{
		return b;
	}
	else 
	{
		return c;
	}
}



function TestExp()
{
	var a , b = 1, 3;
	a = (a + ((a + 1) * Add(a, b) + b) * ((a + 1) * Add(a, b) + a)) + 
	(a + ((a + 1) * Add(a, b) + b) * ((a + 1) * Add(a, b) + a)); 
	
	printf(a);
}

function TestAnd()
{
	printf(max(1,4, 8));
	printf(max(10,2, 6));
	printf(max(101,2000, 604));
}

function TestIf()
{
	var i, k, isShushu = 0, 0, 0;
	for (i = 2; i < gTest; i++)
	{
		isShushu = 1;
		for( k = 2; k < i; k++)
		{
			if ((i % k) == 0)
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



//function main()
{
	var t = getCurrentTime();
	gTest = 100000;
	TestIf();
	printf(getCurrentTime() - t);
	//TestCallTable();
	//TestAnd();
	//TestFor();
	//TestWhile();
	//TestRecurseCall(6);
	//TestCallTable();
	system_pause();
}