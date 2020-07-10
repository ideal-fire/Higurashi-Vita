TEXTMODE_NVL = 0;
TEXTMODE_ADV = 1;
TEXTMODE_AVD = TEXTMODE_ADV; // Compatibility with old versions

Line_ContinueAfterTyping=0;
Line_WaitForInput=1;
Line_Normal=2;
NULL=nil;

FALSE=false;
TRUE=true;

globalFlags = {};
globalFlags["GLanguage"]=1;

localFlags = {};

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

// If you pass false or true then it will be changed to 0 or 1 
function flagValueFilter(val)
	if (val==false) then
	   return 0;
	elseif (val==true) then
		return 1;
	end
	return val;
end
function SetGlobalFlag(flag, val)
	globalFlags[flag]=flagValueFilter(val);
end
function SetLocalFlag(flag, val)
	localFlags[flag]=flagValueFilter(val);
end
// Returns 0 if it is an unknown flag
function GetGlobalFlag(flag)
	if (globalFlags[flag]==nil) then
		return 0;
	end
	return globalFlags[flag];
end
function GetLocalFlag(flag)
	if (localFlags[flag]==nil) then
		return 0;
	end
	return localFlags[flag];
end
//
function Return()
   // see FAKELUAERRORMSG definition
   error("higuvitafakeerr");
end

// wrappers
function DrawSpriteWithFilteringFixedSize(_slot, _filename, _unkOne, _unkStyle, _x, _y, _w, _h, _unkThree, _unkFour, _unkFive, _unkSix, _layer, _time, _waitForComplete)
	DrawSpriteFixedSize(_slot,_filename,_unkOne,_x,_y,0,0,0,_w,_h,0,false,false,_unkStyle,0,_layer,_time,_waitForComplete);
end
function DrawSpriteWithFiltering(_slot, _filename, _filter, _unkStyle, _x, _y, _ignoredOne, _ignoredTwo, _ignoredThree, _ignoredFour, _layer, _time, _waitForCompletion)
	DrawSprite(_slot,_filename,NULL,_x,_y,0,0,0,0,false,false,_unkStyle,256,_layer,_time,_waitForCompletion);
end
function MoveBustshot(_slot, _unkString, _x, _y, _z, _ignoredOne, _time, _waitForCompletion)
   MoveSprite(_slot,_x,_y,_z,0,256,0,_time,_waitForCompletion)
end
TerminateShakingOfSprite=TerminateShakingOfSprite; // these two not implemented in real game?
StartShakingOfSprite=StartShakingOfBustshot

// intentionally ignored
function pi()
   print("intentionally ignore: " .. debug.getinfo(1, "n").name);
end
SetFontId=pi;
SetCharSpacing=pi;
SetLineSpacing=pi;
SetFontSize=pi;
SetNameFormat=pi; // Used for ADV mode in 07th Modding patch
SetScreenAspect=pi;
SetWindowPos=pi;
SetWindowSize=pi;
SetWindowMargins=pi;
SetGUIPosition=pi;
SetValidityOfSaving=pi;
SetValidityOfLoading=pi;
EnableJumpingOfReturnIcon=pi;
RotateBG=pi;
DrawFragment=pi; // Used to draw a 3d cube. Disabled if GetGlobalFlag("GArtStyle")==1
TitleScreen=pi;
NullOp=pi;
OpenGallery=pi;
PlaceViewTip=pi;
PlaceViewTip2=pi;
PlusStandgraphic1=pi;
PlusStandgraphic2=pi;
PlusStandgraphic3=pi;

// print unimplemented
function pu()
	print("unimplemented: " .. debug.getinfo(1, "n").name);
end
//
SetSpeedOfMessage=pu;
SetDrawingPointOfMessage=pu;
SetStyleOfMessageSwinging=pu;
SetValidityOfWindowDisablingWhenGraphicsControl=pu;
StopSE=pu;
SetValidityOfTextFade=pu;
SetValidityOfSkipping=pu;
GetAchievement=pu;
SetFontOfMessage=pu;
ActivateScreenEffectForcedly=pu;
SetValidityOfUserEffectSpeed=pu;
ShakeScreen=pu;
ShakeScreenSx=pu;
//
BlurOffOn=pu;
Break=pu;
ChangeVolumeOfBGM=pu;
ChapterPreview=pu;
CheckTipsAchievements=pu;
CloseGallery=pu;
ControlMotionOfSprite=pu;
DisableBlur=pu;
DisableEffector=pu;
DisableGradation=pu;
DrawBGWithMask=pu;
DrawBustFace=pu;
DrawFace=pu;
DrawStandgraphic=pu;
EnableBlur=pu;
EnableHorizontalGradation=pu;
ExecutePlannedControl=pu;
FadeAllBustshots2=pu;
FadeAllBustshots3=pu;
FadeFace=pu;
FadeOutMultiBGM=pu;
FadeOutSE=pu;
FadeScene=pu;
FadeSceneWithMask=pu;
FadeSpriteWithFiltering=pu;
GetPositionOfSprite=pu;
HideGallery=pu;
JumpScript=pu;
LanguagePrompt=pu;
MoveSpriteEx=pu;
PreloadBitmap=pu;
RevealGallery=pu;
SavePoint=pu;
SetGuiPosition=pu;
SetSkipAll=pu;
SetTextFade=pu;
SetValidityOfFilmToFace=pu;
SetValidityOfInterface=pu;
SpringText=pu;
StoreValueToLocalWork=pu;
ViewChapterScreen=pu;
ViewExtras=pu;
ViewTips=pu;
WaitForInput=pu;
WaitToFinishSEPlaying=pu;
WaitToFinishVoicePlaying=pu;

// write unimplemented
function wu()
DebugFlash();
Debugfile(debug.getinfo(1, "n").name);
end
FragmentListScreen=wu
FragmentViewChapterScreen=wu
JumpScriptSection=wu
SetWindowBackground=wu
// based programmer left an explanation for this command in the script. "ShiftSection move sections without updating call stack or current scope" from "ShiftSection("FragmentChapterDisplay");" So we don't need it.
ShiftSection=wu
ShowChapterPreview=wu
ShowChapterScreen=wu
ShowExtras=wu
ShowTips=wu
StopFragment=wu
// another note from the programmers: "//start the queued move without waiting for it"
Update=wu
