key = {0x15ac3f5d,0x1e865afc,0x6583a5b1,0x54d6f3ea}
function TestReadXml()
{
	f = XmlDocument:OpenFile("test.xml", 1);
	dataXml = f:FirstChild("Data");
	serverXml = dataXml:FirstChild("Server");
	qzoneServerElem = serverXml:FirstChildElement("QQServer");
	serverInfo = qzoneServerElem:FirstChildElement("HostInfo");
	host = serverInfo:Attribute("Host");
	port = serverInfo:Attribute("Port");
	timeout = serverInfo:Attribute("Timeout");
	printf(host, string.utf8togbk(port), timeout);
}

function TestCreateXml()
{
	f = XmlDocument:CreateDocument();
	rootElement = XmlElement:CreateElement("Persons");
	f:LinkEndChild(rootElement);
	personElement = XmlElement:CreateElement("Person");
	rootElement:LinkEndChild(personElement);
	personElement:SetAttribute("ID", "1");
	nameElement = XmlElement:CreateElement("Name");
	ageElement = XmlElement:CreateElement("Age");
	personElement:LinkEndChild(nameElement);
	personElement:LinkEndChild(ageElement);
	
	nameText = XmlNode:CreateText("周星驰");
	ageText = XmlNode:CreateText("40");
	nameElement:LinkEndChild(nameText);
	ageElement:LinkEndChild(ageText);
	
	f:SaveFile("test2.xml");
}




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
/*
var curTime = getCurrentTime();
loadXmlFile("L_101.npCourse", "L_101.npCourse.tmp");
var source = File:open("L_101.npCourse.tmp", "rb");
var buf = source:read(source:size());
source:close();
EncriptFile(buf, "L_101.npCourse.bin");
printf(getCurrentTime() - curTime)
//loadXmlFile();
*/
/*
TestReadXml();
TestCreateXml();
*/

