// CHANGE THIS WHEN I UPDATE THIS FILE!
// 1 - for Higurashi-Vita v1.7
HAPPYFILEVERSION=1;

Line_ContinueAfterTyping=0;
Line_WaitForInput=1;
Line_Normal=2;
NULL=0;

FALSE=false;
TRUE=true;


globalFlags = {};
globalFlags["GLanguage"]=1;

// You see, Im lazy. When doing text wrap for text, I use OutputLine to make the text array for me. I call this function on the tip menu so I dont have to do a lot for string creation
function EasyOutputLine(line)
	OutputLine(nil, nil, nil, line, Line_ContinueAfterTyping);
end

// This function is called when the user pressed x on the Windows test build
function quitxfunction()
	os.exit()
end

// This function will, hopefully, free the other script by getting rid of its main function
function FreeTrash()
	main=nil;
	collectgarbage()
end

// Returns the version of this very file. This is used for Android.
function GetHappyLuaVersion()
	return HAPPYFILEVERSION;
end

// Sets a global flag for future data stuff
// If you pass false or true then it will be changed to 0 or 1 
function SetGlobalFlag(flag, val)
	if (val==false) then
		val=0;
	elseif (val==true) then
		val=1;
	end
	globalFlags[flag]=val;
	//print(flag .. " is now " .. val)
end

// Gets a global flag that was set with SetGlobalFlag
// Returns 0 if it is an unknown flag
function GetGlobalFlag(flag)
	if (globalFlags[flag]==nil) then
		//print("returning " .. 0 .. " for flag " .. flag)
		return 0;
	end
	//print("returning " .. globalFlags[flag] .. " for flag " .. flag)
	return globalFlags[flag];
end