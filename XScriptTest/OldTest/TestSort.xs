function select_sort(A)
{
    var i,j,min,m;
	n = A:size();
    for(i=0;i<n-1;i++)
    {
        min=i;
        for(j=i+1;j<n;j++)
        {
            if(A[min]>A[j])
            {
                min=j;
            }
        }
        if(min!=i)
        {
            tmp = A[min];
			A[min]=A[i];
			A[i]=tmp;
        }
    }
}

function TestLoop()
{
	var total = 0;
	for (i = 0; i < 100000000; i++)
	{
		total += i;
	}
}

l=[]
for (i = 0; i < 10000; i++)
{
	l:append(math.random() % 100000);
}
var startTime = getCurrentTime()
printf("startTime " $ startTime);
select_sort(l)
printf(getCurrentTime());
	var total = 0;
	while (i < 100000000)
	{
		total += i;
		i++;
	}
	printf(getCurrentTime());
os.system("pause")