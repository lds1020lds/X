

src = "./res"
dst = "../bin"
ignore_list = [ "*.zip", "*.svn", "*.npSln", "*.h", "*.cpp", "*.ipa", "*.exe", "*.tmp", "*.Editor", "*.db", "*.py", "*.meta",
    "*.npModule",
    "*.npSprite", "*.npAction", "*.npCourse", "*.npLayout", "*.npTemplate", "*.npSource", "*.atlas", "*.log",
    "JP_S_*.txt"]


function pack_file(filename, fh)
{
	var xmlFile = XmlDocument:OpenFile(filename, 1);
	var groupsNode = xmlFile:FirstChild("Groups");

	var pos1 = filename:rfind("/") + 6;
	var num = filename:rfind(".") - pos1;
	level = filename:sub(pos1, num);
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
	var savePath = src $ "/Level/JPSpecialLevelData.bin"
	var f = File:open(savePath, "wb");
	
	var outTotalFilePath = src $ "/Level";
	var fileList = os.listdir(outTotalFilePath);
	foreach(k, file in lenum(fileList))
	{
		if (file:endWith(".txt"))
		{
			pack_file(file, f);
		}
	}
	
	f:close();
}

	
function MergeMetaFiles(srcDir)
{
	moduleMetaList = ["\n<Module>"]
    spriteMetaList = ["\n<Sprite>"]
    actionMetaList = ["\n<Action>"]
	
	files = os.listdir(srcDir);
	foreach( index, file in lenum(files) )
	{
		if ( string.endWith(file, ".meta") )
		{
			fh = File:open(file, "r")
			meta = "\n\t<Meta Guid=\"_guid_\" Path=\"_path_\" File=\"_file_\" />";
			
			guid = fh:readline(256)
            fPath = fh:readline(256)
            fFile = fh:readline(256)
            meta = string.replace(meta, "_guid_", guid)
            meta = string.replace(meta, "_path_", fPath)
            meta = string.replace(meta, "_file_", fFile)
			
            if (string.endWith(file, ".npModule.meta"))
                moduleMetaList:append(meta);
            else if (string.endWith(file, ".npSprite.meta"))
                spriteMetaList:append(meta);
            else if (string.endWith(file, ".npAction.meta"))
                actionMetaList:append(meta)
			fh:close()
		}
	}
	
	var mergeFile = src $ "/2DSResource.meta"
	fh = File:open(mergeFile, "w");
	fh:write("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>")
    fh:writelines(moduleMetaList)
    fh:write("\n</Module>")
    fh:writelines(spriteMetaList)
    fh:write("\n</Sprite>")
    fh:writelines(actionMetaList)
    fh:write("\n</Action>")
	fh:close()
}

function RemoveFiles(srcDir, postfixs)
{
	var files = os.listdir(srcDir);
    foreach(i, f in lenum(files))
	{
        if (f:endWith(postfixs))
		{
			os.remove(f);
		}   
	}
}

function CollectFiles(srcDir, postfixs, outputFile)
{
    var postfixFiles = []
	var files = os.listdir(srcDir);
    foreach(i, f in lenum(files))
	{
        var matchSuffix = 0;
        foreach(k, postfix in lenum(postfixs))
		{
			 if (f:endWith(postfix) == True)
			 {
				matchSuffix = 1;
                break;
			 }      
		}
        
		if (matchSuffix)
		{
			var fullname = f:sub(string.len(srcDir) + 1);
			postfixFiles:append(fullname $ "\n");
		}
	}

    fh = File:open(outputFile, "w")
    fh:writelines(postfixFiles)
    fh:close()
}


function makeBinFiles()
{
    RemoveFiles(src, ".pzd")
	var normalPath = src:replace("/", "\\")
    var filename = "filebinary.exe .pzd";
	printf(filename)
    os.setpwd(src);
    os.system(filename)
    os.setpwd("..")
}
	

function get_all_moudle_folders(folder)
{
	var folderfilter = "MODULE."
	var modulefolders = []
	var files, dirs = os.listdir(folder);
	foreach(i, dir in lenum(dirs))
	{
		var lastPos = dir:rfind("/");
		basedir = dir:sub(lastPos + 1);
		if(basedir:startWith(folderfilter))
			modulefolders:append(dir)
	}
	return modulefolders
}
	
function get_source_files_from(modulefolder)
{
	var filefilter = ".npSource"
	var sourcefiles = []
	var files, dirs = os.listdir(modulefolder);
	foreach(i, f in lenum(files))
	{
		if(f:endWith(filefilter))
		{
			var fullname = f:sub(modulefolder:len() + 1);
			sourcefiles:append(fullname)
		}
	}
	return sourcefiles;
}
	
function get_all_source_files_from(resFolder)
{
	var outxmlfile = src $ "/npsource.xml";

	var moduleFolderFmt = "<ModuleFolder Folder=\"_path_\">\n"
	var moduleFolderCloseFmt = "</ModuleFolder>\n"
	var sourceFileFmt = "\t<SourceFile File=\"_file_\" />\n"
	var fh = File:open(outxmlfile,"w")
	modulefolders = get_all_moudle_folders(resFolder);
	var resFolderLen = resFolder:len();
	foreach(i, folder in lenum(modulefolders))
	{
		var relativepath = folder:sub(resFolderLen + 1);
		fh:write(moduleFolderFmt:replace("_path_", relativepath))
		sourcefiles = get_source_files_from(folder)
		foreach(s,f in lenum(sourcefiles))
			fh:write(sourceFileFmt:replace("_file_", f));
		fh:write(moduleFolderCloseFmt)
	}
	fh:close()
}


function RemovePngFile(fileName)
{
	var folderfilter = "MODULE."
	var modulefolders = []
	var files, dirs = os.listdir(folder);
	foreach(i, dir in lenum(dirs))
	{
		var lastPos = dir:rfind("/");
		basedir = dir:sub(lastPos + 1);
		if(basedir:startWith(folderfilter))
		{
			var moudleFiles = os.listdir(dir);
			foreach(k, name in lenum(moudleFiles))
			{
				if ( name:endWith(".png") && !name:endWith(".BIG.png"))
				{
					os.remove(name);
				}
			}
		}
	}
}

function RemoveBinFiles(fileName)
{
	var modulefolders = []
	var files = os.listdir(fileName);
	foreach(i, name in lenum(files))
	{
		if ( name:endWith(".npModule.bin") 
			|| name:endWith(".npSprite.bin") 
			|| name:endWith(".npAction.bin") 
			|| name:endWith(".npLayout.bin") 
			|| name:endWith(".nTemplate.bin") 
			|| name:endWith(".lua.bin") 
			|| name:endWith(".npCourse.bin"))
		{
			os.remove(name);
		}
	}
}

function rewriteNpConfig(versionFile)
{
	var file = File:open(versionFile,"r")
	
	fileStr = file:read(file:size())
	file:close()
	fileStr = fileStr:replace("<BinaryLoad>False</BinaryLoad>", "<BinaryLoad>True</BinaryLoad>")
	file = File:open(versionFile,"w")
	file:write(fileStr)
	file:close()
}
/*
curTime = getCurrentTime();
makeDataToBin();
printf("MergeMetaFiles finished, use time:", getCurrentTime() - curTime)
os.system("pause")
*/

curTime = getCurrentTime();
var totalTimeStart = curTime
var h = os.exec("XScripter.exe MakeLevelData.xs")
printf("MergeMetaFiles start")
MergeMetaFiles(src)

printf("MergeMetaFiles finished, use time:", getCurrentTime() - curTime)
curTime = getCurrentTime();
printf("MakeNpSource start");

get_all_source_files_from(src);

printf("MakeNpSource finished, use time:", getCurrentTime() - curTime)
curTime = getCurrentTime();

printf("CollectFiles start");
CollectFiles(src, [".npLayout", ".particle", ".controller"], src $ "/CollectFile.txt")
printf("CollectFiles finished, use time:", getCurrentTime() - curTime)
curTime = getCurrentTime();

printf("makeBinFiles start");
makeBinFiles();
printf("makeBinFiles finished, use time:", getCurrentTime() - curTime)
curTime = getCurrentTime();

printf("MakeSpecialLevel start");
MakeSpecialLevel();
printf("MakeSpecialLevel finished, use time:", getCurrentTime() - curTime)
curTime = getCurrentTime();

printf("copy files to bin start");
if (!os.rmdir(dst))
	dst = "../bin1"
var ret = os.copydir(src, dst, nil, ignore_list)
os.copyfile(src $ "/2DSResource.meta", dst $ "/2DSResource.meta")
os.remove(dst $ "/filebinary")
os.remove(dst $ "/filepackage")
os.remove(dst $ "/CollectFile.txt")

binsrc = src $ "/../rawConfig/BinFiles/client"
bindst = dst $ "/Config"
rewriteNpConfig(dst $ "/NPEngine.npConfig");
os.wait(h);	
os.copydir(binsrc, bindst, "*.bin")
os.copyfile("./PCLogin.dll", "../bin")


printf("copy files to bin finished, use time:", getCurrentTime() - curTime)
curTime = getCurrentTime();

printf("remove useless files start");
textureFile = dst $ "/Texture"
moduleFile = dst $ "/Module"

RemoveFiles(textureFile, ".png")
RemoveFiles(moduleFile, ".png")
RemoveFiles(textureFile, ".PNG")
RemoveFiles(moduleFile, ".PNG")
RemoveFiles(src, ".pzd")
RemoveBinFiles(src);

printf("remove useless files finished, use time:", getCurrentTime() - curTime)

printf("makeres finished, total time is ",getCurrentTime() - totalTimeStart);
os.system("pause")

