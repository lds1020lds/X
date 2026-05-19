
function TestMetaTable()
{
	var metaTable = {a="a", b="b"};
	printf(metaTable);
	var t = {c = "c", d = "d"};
	metaTable.__index = metaTable;
	metaTable.__newindex = metaTable;
	setmetatable(t, metaTable);
	printf(t.a);
	t.f = 1;
	printf(metaTable.f)
	
}


function TestMetaTable2()
{
	var metaTable = {__index= function(table, key){ return "index2";}
	, __newindex=function(table, key, val) 
	{
		table[key] = val $ "__newindex";
	} };
	var t = {c = "c", d = "d"};
	setmetatable(t, metaTable);
	printf(t.a);
}

function TestMetaTable3()
{
	var metaTable = {a="a", b="b"};
	var metaTable2 = {e="e", f="f"};
	
	metaTable2.__index = metaTable2;
	setmetatable(metaTable, metaTable2);
	
	var t = {c = "c", d = "d"};
	metaTable.__index = metaTable;
	setmetatable(t, metaTable);
	
	printf(t.e);
}

function TestMetaTable4()
{
	var metaTable = {a="a", b="b"};
	var metaTable2 = {__index = function(table, key) { return "index4"}};
	setmetatable(metaTable, metaTable2);
	
	var t = {c = "c", d = "d"};
	metaTable.__index = metaTable;
	setmetatable(t, metaTable);
	
	printf(t.e);
}

function TestTagMethod()
{
	var metaTable = {
	__add = function(table, intValue)
	{
		table.add += intValue;
		return table;
	}
	,
	__sub = function(table, intValue)
	{
		table.minus -= intValue;
		return table;
	}
	,
	
	__mul = function(table, intValue)
	{
		table.mul *= intValue;
		return table;
	}
	,
	__div = function(table, intValue)
	{
		table.div /= intValue;
		return table;
	}
	,
	__equal = function(table, table2)
	{
		return table.equal ==  table2.equal;
	}
	,
	__mod = function(table, intValue)
	{
		table.mod %= intValue;
		return table;
	}
	,
	__pow = function(table, intValue)
	{
		table.pow ^= intValue;
		return table;
	}
	,
	__less = function(table, table2)
	{
		return table.less < table2.less;
	}
	,
	__lessequal = function(table, table2)
	{
		return table.lessequal <= table2.lessequal;
	}
	,
	__concat = function(table, intValue)
	{
		table.concat $= intValue;
		return table;
	}
	,
	__neg = function(table)
	{
		table.neg = -table.neg;
		return table;
	}
	,
	__call = function(callValue)
	{
		printf("table call " $ toString(callValue));
	}
	};
	
	var compareTable1  = { equal = 4, less = 4, lessequal = 5};
	var compareTable2  = { equal = 5, less = 6, lessequal = 7};
	var compareTable3  = { equal = 5, less = 6, lessequal = 6};
	
	var testTable = {add = 1, minus = 1, mul = 2, div = 100, equal = 4, mod = 36, pow = 3, neg = 7, less = 5, lessequal = 6, concat = "concat"};
	setmetatable(testTable, metaTable);
	setmetatable(compareTable1, metaTable);
	setmetatable(compareTable2, metaTable);
	setmetatable(compareTable3, metaTable);
	
	testTable += 2;
	testTable -= 2;
	testTable *= 2;
	testTable /= 2;
	testTable %= 7;
	testTable ^= 2;
	testTable $= "string";
	
	printf("testTable == compareTable1", testTable == compareTable1);
	printf("testTable == compareTable2", testTable == compareTable2);
	
	printf("testTable < compareTable1", testTable < compareTable1);
	printf("testTable < compareTable2", testTable < compareTable2);
	printf("testTable <= compareTable1", testTable <= compareTable1);
	printf("testTable <= compareTable2", testTable <= compareTable2);
	
	printf("testTable > compareTable1", testTable > compareTable1);
	printf("testTable > compareTable2", testTable > compareTable2);
	printf("testTable >= compareTable1", testTable >= compareTable1);
	printf("testTable >= compareTable2", testTable >= compareTable2);
	
	printf("testTable >= compareTable3", testTable >= compareTable3);
	printf("testTable <= compareTable3", testTable <= compareTable3);
	
	testTable(compareTable1);
	printf(testTable);
	
	foreach(key, value in ipairs(testTable))
	{
		printf(key, value);
	}
}
TestTagMethod();
/*
TestMetaTable();

TestMetaTable2();

TestMetaTable3();
TestMetaTable4();
*/
system_pause();

