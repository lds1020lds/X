src = "./res"
	
key = {0x15ac3f5d,0x1e865afc,0x6583a5b1,0x54d6f3ea}
modifyTimeFile = src $ "/Level/ModifyTime.xml"
function encipher(v, k)
{
    var y=v[0];
    var z=v[1];
    var sum=0;
    var delta=0x9E3779B9;
    var n=16
    var w=array(2)
    e = y;
    while(n>0)
	{
        sum += delta
        y += ( z << 4 ) + k[0] # z + sum # ( z >> 5 ) + k[1]
        y = y & 0xffffffff;
        z += ( y << 4 ) + k[2] # y + sum # ( y >> 5 ) + k[3]
        z = z & 0xffffffff
        n -= 1
	}
	
    w[0]=y;
    w[1]=z;
    return w
}
	
function decipher(v, k)
{
    var y = v[0];
    var z= v[1];
    var sum = 0xE3779B90;
    var delta = 0x9E3779B9;
    var n = 16;
    

    while (n>0)
	{
        z -= ( y << 4 ) + k[2] ^ y + sum ^ ( y >> 5 ) + k[3]
	z = z & 0xffffffff;
        y -= ( z << 4 ) + k[0] ^ z + sum ^ ( z >> 5 ) + k[1]
	y = y & 0xffffffff;
        sum -= delta
    	sum = sum & 0xffffffff;
        n -= 1
	}
	
	var w = array(2);
    	w[0] =	y
    	w[1] =	z
   	 return w;
}

function getIntValue(v, offset)
{
	return v[offset] | (v[offset + 1] << 8) | (v[offset + 2] << 16) | (v[offset + 3] << 24);
}

function EncriptFile(bytes, destFile)
{
    var saveFile = File:open(destFile, "wb")
	var fileSize = string.rawlen(bytes);
	var totalLen = fileSize + 1;
	var nPadlen = totalLen%8;
	if (nPadlen != 0)
		nPadlen = 8 - nPadlen;
	
	srcBytes = array(8);
	xorBuffer = {key[0], key[2]}
	srcBytes[0] = nPadlen
	src_i = 1
	while(nPadlen > 0)
    {
		srcBytes[src_i] = 200 + nPadlen
		src_i += 1
		nPadlen -= 1
	}
    
	if (src_i >= 8)
    {
		values = {getIntValue(srcBytes, 0) # xorBuffer[0], getIntValue(srcBytes, 4) # xorBuffer[1]}
		enValues = encipher(values, key)
		xorBuffer = enValues
		saveBytes = struct.pack("ii", enValues[0], enValues[1])
		saveFile:write(saveBytes)
		src_i = 0;
    }
	
	dataIndex = 0
	while(dataIndex < fileSize)
    {
		if (src_i < 8)
        {
			srcBytes[src_i] = bytes[dataIndex];
			dataIndex += 1;
			src_i += 1;
        }
        
		if (src_i >= 8)
        {
            v1 = getIntValue(srcBytes, 0) # xorBuffer[0]
			v2 = getIntValue(srcBytes, 4) # xorBuffer[1]
			values = {v1, v2}
			enValues = encipher(values, key)
			xorBuffer = enValues
			saveBytes = struct.pack("ii", enValues[0], enValues[1])
			saveFile:write(saveBytes)
			src_i = 0;
        }
     }
     
     saveFile:close()	
}


function loadXmlFile(srcFile, destFile)
{
	var xmlFile = XmlDocument:OpenFile(srcFile, 1);
	sceneLayerList = xmlFile:FirstChild("SceneLayerList");
	firstSceneLayer = sceneLayerList:FirstChildElement("SceneLayer");
	
	var mapX, mapY, mapH, mapW = 0,0,0,0;
	var layerCount = 0;
	var layer = nil;
	layer = firstSceneLayer;
	while (layer != nil)
	{
		layerCount++;
		x=string.atoi(layer:Attribute("X"))
		y=string.atoi(layer:Attribute("Y"))
		
		cellR= string.atoi(layer:Attribute("CellR"));
		cellC= string.atoi(layer:Attribute("CellC"));
		cellW= string.atoi(layer:Attribute("CellW"));
		cellH= string.atoi(layer:Attribute("CellH"));
		
		if (mapX > x)
			mapX = x;
		if (mapY > y)
			mapY = y;
		if(mapW < cellC*cellW)
			mapW = (cellC*cellW);
		if (mapH < cellR*cellH)
			mapH = (cellR*cellH);
			
		layer = layer:NextSiblingElement("SceneLayer");
	}
	
	var levelFile = File:open(destFile $ ".tmp", "wb");
	levelFile:write(struct.pack("h4i", layerCount, mapX, mapY, mapW, mapH));
	
	layer = firstSceneLayer;
	while (layer != nil)
	{
		var childCount = layer:ChildElementCount("NP2DSFrameRef");
		layerName = layer:Attribute("Name")
		layerX = string.atoi(layer:Attribute("X"))
		layerY = string.atoi(layer:Attribute("Y"))
		levelFile:write(struct.pack("i",string.len(layerName)))
		levelFile:write(struct.pack(toString(string.len(layerName))$"s",layerName))
		levelFile:write(struct.pack("h2i",childCount, layerX, layerY))

		firstRef = layer:FirstChildElement("NP2DSFrameRef");
		while (firstRef != nil)
		{
			ref = firstRef;
			firstRef = firstRef:NextSiblingElement("NP2DSFrameRef");

			var worldTrans = ref:FirstChildElement("WorldTrans")
			var worldScale = ref:FirstChildElement("WorldScale")
			
			groupID = 0
			resfile = string.atoi(ref:Attribute("Resfile"))
			frameID = string.atoi(ref:Attribute("Frameid"))
			
			transX = string.atoi(worldTrans:Attribute("x"))
			transY = string.atoi(worldTrans:Attribute("y"))
			scaleX = string.atof(worldScale:Attribute("x"))
			scaleY = string.atof(worldScale:Attribute("y"))
			levelFile:write(struct.pack("5i2f", groupID, resfile,frameID, transX, transY, scaleX, scaleY))
			

			var firstSceneParamList = ref:FirstChildElement("SceneParamList");
			if (firstSceneParamList != nil)
			{
				var sceneParamNum = firstSceneParamList:ChildElementCount("SceneParam");
				levelFile:write(struct.pack("h", sceneParamNum));
			
				var firstSceneParam = firstSceneParamList:FirstChildElement("SceneParam");
				while( firstSceneParam != nil )
				{
					var paramName = firstSceneParam:Attribute("Name");
					var paramText = firstSceneParam:Attribute("Text");
					
					levelFile:write(struct.pack("i",string.len(paramName)))
					levelFile:write(struct.pack(toString(string.len(paramName))$"s",paramName))
					
					levelFile:write(struct.pack("i",string.len(paramText)))
					levelFile:write(struct.pack(toString(string.len(paramText))$"s",paramText))
					
					firstSceneParam = firstSceneParam:NextSiblingElement("SceneParam");
				}
				
				
			}
			else
			{
				levelFile:write(struct.pack("h", 0));
			}
		}
		
		layer = layer:NextSiblingElement("SceneLayer");
	}
	
	levelFile:close();
	
	var source = File:open(destFile $ ".tmp", "rb");
	var buf = source:read(source:size());
	source:close();
	EncriptFile(buf, destFile);
	os.remove(destFile $ ".tmp")
	
	xmlFile:DeleteDocument();
}

function SaveLevelModifyTime(savePath, timeTable)
{
	var f = XmlDocument:CreateDocument();
	var rootElement = XmlElement:CreateElement("Files");
	f:LinkEndChild(rootElement);
	foreach(k, v in ipairs(timeTable))
	{
		var personElement = XmlElement:CreateElement("File");
		personElement:SetAttribute("Name", k);
		personElement:SetAttribute("ModifyTime", toString(v));
		rootElement:LinkEndChild(personElement);
	}

	f:SaveFile(savePath);
	f:DeleteDocument();
}

function LoadLevelModifyTime(savePath, timeTable)
{
	var f = XmlDocument:OpenFile(savePath, 1);
	if (f != nil)
	{
		var dataXml = f:FirstChild("Files");
		var fileList = dataXml:ChildElementList("File");
		foreach (k, v in lenum(fileList))
		{
			var name = v:Attribute("Name");
			var mtime = v:Attribute("ModifyTime");
			timeTable[name] = mtime:atoi();
		}
		f:DeleteDocument();
	}
	
	
}

function makeDataToBin()
{
    var binFileList = []
    var binFileListTotal = {}
	
	var outTotalFileMaxSize = 1024 * 900
    var outTotalFileSize = 0
    var outTotalFileIdx = 0
    var outTotalFileName = src $ "/Level/Level_" $ toString(outTotalFileIdx)
    var outTotalFilePath = src $ "/Level";
    var outTotalFile = File:open(outTotalFileName, "wb")
	var fileList = os.listdir(outTotalFilePath);
	var xmlTime = 0
	var modifyTimeTable = {}
	LoadLevelModifyTime(modifyTimeFile, modifyTimeTable);
	var hasChanged = false;
	foreach(k, path in lenum(fileList))
	{

		if (string.endWith(path, ".npCourse"))
		{
			binPath = path $ ".bin";
			var mtime = os.getmtime(path);
			var isNew = false;
			if (os.exists(binPath) && modifyTimeTable[path] != nil)
			{
				isNew = (modifyTimeTable[path] >= mtime)
			}
			
			if (!isNew)
			{
				loadXmlFile(path, binPath)
				modifyTimeTable[path] = mtime;
				hasChanged = true
			}
			
			inSingleFileSize = os.getsize(binPath)
			if (outTotalFileSize + inSingleFileSize >= outTotalFileMaxSize)
			{
                    binFileListTotal[outTotalFileName] = binFileList
                    binFileList = []
                    outTotalFileSize = 0
                    outTotalFileIdx = outTotalFileIdx + 1
                    outTotalFileName = src $ "/Level/Level_" $ toString(outTotalFileIdx)
                    outTotalFile:close()
                    outTotalFile = File:open(outTotalFileName, "wb")
			}
			
			inSingleFile = File:open(binPath, "rb")
            inSingleFileData = inSingleFile:read(inSingleFileSize)
			
            var realSize = outTotalFile:write(inSingleFileData)
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
		//printf("outTotalFilePath2", outTotalFilePath2, inSingleFileSize2)
        inSingleFileData2 = inSingleFile2:read(inSingleFileSize2)
        outTotalFile2:write(inSingleFileData2)
        outTotalFile2:close()
        inSingleFile2:close()
        os.remove(outFilePathName)
	}
	
	if (hasChanged)
		SaveLevelModifyTime(modifyTimeFile, modifyTimeTable);
	
}

curTime = getCurrentTime();
printf("makeDataToBin start");
makeDataToBin();
printf("makeDataToBin finished, use time:", getCurrentTime() - curTime)
os.system("pause")
