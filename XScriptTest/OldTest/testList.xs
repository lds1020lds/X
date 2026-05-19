function compare(v1, v2, r)
{
	if(r == 1)
		return string.atoi(v1) > string.atoi(v2); 
	else
		return string.atoi(v1) < string.atoi(v2); 
}

function key(v)
{
	return string.atoi(v); 
}
a = ["1", "19", "2", "3", "11", "12", "22"]
a:sort();
printf(a);
a:sortWithCmp(lambda v1,v2:string.atoi(v1) > string.atoi(v2))
printf(a);
a:sortWithCmp(compare)
printf(a);
a:sortWithCmp(compare, 1)
printf(a);
a:sortWithKey(lambda v : v:atoi())
printf(a);
a:sortWithKey(key, 1)
printf(a);
os.system("pause")

