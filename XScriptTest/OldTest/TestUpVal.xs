function TestUpVal(a, b)
{
	var c, d = 1, 2;
	
	function InterFunc1()
	{
		var i,  r = 0, 0;
		for ( i = 0; i < a; i++)
		{
			r += i;
		}
		return r;
	}
	function InterFunc2(e)
	{
		c += e;
		d += e;
		a += e;
	}
	function InterFunc3( )
	{
		printf(c + d + a);
	}
	
	return InterFunc1, InterFunc2, InterFunc3;
}


system_pause();