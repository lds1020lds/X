function TestPack()
{
	var format = "2ihcfd4s";
	var fileName = "testPack.dat";
	var buf = struct.pack(format, 202561,3045, 656, 254, 3.14, 9.186, "ssss");
	f = File:open(fileName, "w");
	f:write(buf);
	f:close();
	
	f2 = File:open(fileName, "rb");
	size = f2:size();
	readBuf = f2:read(size);
	f2:close();
	var b1, b2, b3, b4, b5, b6, b7 = struct.unpack(format, readBuf);
	printf(b1, b2, b3, b4, b5, b6, b7);
}

TestPack();
os.system("pause");