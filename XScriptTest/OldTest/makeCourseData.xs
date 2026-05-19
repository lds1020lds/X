
require("TestXml.xs");
function makeDataToBin()
{
	
    var binFileList = []
    var binFileListTotal = {}
	
	var outTotalFileMaxSize = 1024 * 900
    var outTotalFileSize = 0
    var outTotalFileIdx = 0
    var outTotalFileName = "Level/Level_" $ toString(outTotalFileIdx)
    var outTotalFilePath = "Level";
    var outTotalFile = File:open(outTotalFileName, "wb")
	var fileList = os.listdir(outTotalFilePath);
	foreach(k, path in lenum(fileList))
	{
		if (string.endWith(path, ".npCourse"))
		{
			binPath = path $ ".bin";
			//printf("loadXmlFile:", path)
			loadXmlFile(path, binPath)
			
			inSingleFileSize = os.getsize(binPath)
			if (outTotalFileSize + inSingleFileSize >= outTotalFileMaxSize)
			{
                    binFileListTotal[outTotalFileName] = binFileList
                    binFileList = []
                    outTotalFileSize = 0
                    outTotalFileIdx = outTotalFileIdx + 1
                    outTotalFileName = "Level/Level_" $ toString(outTotalFileIdx)
                   
                   // printf("Generate level bin file: " , outTotalFileName)
                    outTotalFile:close()
                    outTotalFile = File:open(outTotalFileName, "wb")
			}
			
			inSingleFile = File:open(binPath, "rb")
            inSingleFileData = inSingleFile:read(inSingleFileSize)
            outTotalFile:write(inSingleFileData)
            inSingleFile:close()
            outTotalFileSize += inSingleFileSize
			var startPos = string.rfind(path, "/") + 1;
			var EndPos = string.rfind(path, ".");
            singleFileName = string.sub(path, startPos, EndPos - startPos);
            binFileList:append([singleFileName, inSingleFileSize])
			garbageCollect();
		}
	}
	
	outTotalFile:close()
    binFileListTotal[outTotalFileName] = binFileList
    binFileList = []
	
	foreach( outFilePathName, files in ipairs(binFileListTotal))
	{
		
		fileNum = files:size();
        inSingleFileSize2 = os.getsize(outFilePathName)
        inSingleFile2 = File:open(outFilePathName, "rb")
		
        outTotalFilePath2 = outFilePathName $ ".lv"
        outTotalFile2 = File:open(outTotalFilePath2, "wb")

        bytes = struct.pack("ii", 9527, fileNum)
        outTotalFile2:write(bytes)


        for(i = 0; i < fileNum; i++)
		{
            localName = files[i][0]
            localSize = files[i][1]

            bytes = struct.pack("i", localSize) 
            outTotalFile2:write(bytes)
            bytes = struct.pack("32s", localName)
            outTotalFile2:write(bytes)
        }
		printf("outTotalFilePath2", outTotalFilePath2, inSingleFileSize2)
        inSingleFileData2 = inSingleFile2:read(inSingleFileSize2)
        outTotalFile2:write(inSingleFileData2)
        outTotalFile2:close()
        inSingleFile2:close()
        os.remove(outFilePathName)
	}
}

function pack_file(filename, fh)
{
	var xmlFile = XmlDocument:OpenFile(filename, 1);
	var groupsNode = xmlFile:FirstChild("Groups");

	var pos1 = filename:rfind("/") + 6;
	var num = filename:rfind(".") - pos1;
	level = filename:sub(pos1, num);
	printf(level)
	level = level:atoi();
	
	var groups = groupsNode:ChildElementList("Group");
	var childCount = groups:size();	
	fh:write(struct.pack("ii", level, childCount))

	foreach(i, grp in lenum(groups))
	{
		fh:write(struct.pack("7i2f", grp:Attribute("ID"):atoi(), 
			grp:Attribute("LaserNum"):atoi(), 
			grp:Attribute("CenterX"):atoi(), 
			grp:Attribute("StartY"):atoi(), 
			grp:Attribute("Space"):atoi(), 
			grp:Attribute("Dist"):atoi(), 
			grp:Attribute("DistFly"):atoi(), 
			grp:Attribute("TimeToWait"):atof(), 
			grp:Attribute("TimeToFly"):atof()
		))
		var actions = grp:ChildElementList("Action");
		fh:write(struct.pack("i",actions:size()));
		
		foreach(s, act in lenum(actions))
		{
			fh:write(struct.pack("fi3f5i", act:Attribute("TimeToWait"):atof(),
				act:Attribute("MoveDir"):atoi(),
				act:Attribute("TimeToAim"):atof(),
				act:Attribute("TimeToShoot"):atof(),
				act:Attribute("TimeToMove"):atof(),
				 act:Attribute("DistToMove"):atoi(),
				  act:Attribute("AimMoveDir"):atoi(),
				   act:Attribute("AimMoveDist"):atoi(),
				    act:Attribute("ShootMoveDir"):atoi(),
					 act:Attribute("ShootMoveDist"):atoi() ));
		}
	}
}

function MakeSpecialLevel()
{
	var savePath = "Level/JPSpecialLevelData.bin"
	var f = File:open(savePath, "wb");
	
	var outTotalFilePath = "Level";
	var fileList = os.listdir(outTotalFilePath);
	printf(f)
	foreach(k, file in lenum(fileList))
	{
		if (file:endWith(".txt"))
		{
			pack_file(file, f);
		}
	}
	
	f:close();
}


var curTime = getCurrentTime();
//MakeSpecialLevel();
makeDataToBin();
printf(getCurrentTime() - curTime)
os.system("pause")
