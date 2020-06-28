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
		return 0;
	end
	return globalFlags[flag];
end

// wrappers
function DrawSpriteWithFilteringFixedSize(_slot, _filename, _unkOne, _unkStyle, _x, _y, _w, _h, _unkThree, _unkFour, _unkFive, _unkSix, _layer, _time, _waitForComplete)
	DrawSpriteFixedSize(_slot,_filename,_unkOne,_x,_y,0,0,0,_w,_h,0,false,false,_unkStyle,0,_layer,_time,_waitForComplete);
end
function DrawSpriteWithFiltering(_slot, _filename, _filter, _unkStyle, _x, _y, _ignoredOne, _ignoredTwo, _ignoredThree, _ignoredFour, _layer, _time, _waitForCompletion)
	DrawSprite(_slot,_filename,NULL,_x,_y,0,0,0,0,false,false,_unkStyle,256,_layer,_time,_waitForCompletion);
end

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
ShakeScreen=pu;
ShakeScreenSx=pu;
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
//
BlurOffOn=pu;
Break=pu;
ChangeBustshot=pu;
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
GetLocalFlag=pu;
GetPositionOfSprite=pu;
HideGallery=pu;
JumpScript=pu;
LanguagePrompt=pu;
MoveBustshot=pu;
MoveSpriteEx=pu;
PreloadBitmap=pu;
Return=pu;
RevealGallery=pu;
SavePoint=pu;
SetGuiPosition=pu;
SetLocalFlag=pu;
SetSkipAll=pu;
SetTextFade=pu;
SetValidityOfFilmToFace=pu;
SetValidityOfInterface=pu;
SpringText=pu;
StartShakingOfAllObjects=pu;
StartShakingOfBustshot=pu;
StartShakingOfSprite=pu;
StartShakingOfWindow=pu;
StoreValueToLocalWork=pu;
TerminateShakingOfAllObjects=pu;
TerminateShakingOfBustshot=pu;
TerminateShakingOfSprite=pu;
TerminateShakingOfWindow=pu;
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
ShiftSection=wu
ShowChapterPreview=wu
ShowChapterScreen=wu
ShowExtras=wu
ShowTips=wu
StopFragment=wu
Update=wu
