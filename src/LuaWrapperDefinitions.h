#ifndef GUARD_LUAWRAPPER
#define GUARD_LUAWRAPPER
//===============================================================
//===============================================================
//===============================================================
GENERATELUAWRAPPER(scriptDisplayWindow);
GENERATELUAWRAPPER(scriptClearMessage);
GENERATELUAWRAPPER(scriptOutputLine);
GENERATELUAWRAPPER(scriptOutputLineAll);
GENERATELUAWRAPPER(scriptWait);
GENERATELUAWRAPPER(scriptDrawSceneWithMask);
GENERATELUAWRAPPER(scriptDrawScene);
GENERATELUAWRAPPER(scriptNotYet);
GENERATELUAWRAPPER(scriptPlayBGM);
GENERATELUAWRAPPER(scriptStopBGM);
GENERATELUAWRAPPER(scriptFadeoutBGM);
GENERATELUAWRAPPER(scriptDrawBustshotWithFiltering);
GENERATELUAWRAPPER(scriptDrawBustshot);
GENERATELUAWRAPPER(scriptSetValidityOfInput);
GENERATELUAWRAPPER(scriptFadeAllBustshots);
GENERATELUAWRAPPER(scriptDisableWindow);
GENERATELUAWRAPPER(scriptFadeBustshotWithFiltering);
GENERATELUAWRAPPER(scriptFadeBustshot);
GENERATELUAWRAPPER(scriptPlaySE);
GENERATELUAWRAPPER(scriptPlayVoice);
GENERATELUAWRAPPER(scriptCallScript);
GENERATELUAWRAPPER(scriptChangeScene);
GENERATELUAWRAPPER(scriptDrawSprite);
GENERATELUAWRAPPER(scriptMoveSprite);
GENERATELUAWRAPPER(scriptFadeSprite);
GENERATELUAWRAPPER(scriptSelect);
GENERATELUAWRAPPER(scriptLoadValueFromLocalWork);
GENERATELUAWRAPPER(scriptCallSection);
GENERATELUAWRAPPER(scriptDrawFilm);
GENERATELUAWRAPPER(scriptFadeFilm);
GENERATELUAWRAPPER(scriptFadeBG);
GENERATELUAWRAPPER(scriptMoveBust);
GENERATELUAWRAPPER(scriptGetScriptLine);
GENERATELUAWRAPPER(scriptDebugFile);
GENERATELUAWRAPPER(scriptOptionsEnableVoiceSetting);
GENERATELUAWRAPPER(scriptOptionsSetTextMode);
GENERATELUAWRAPPER(scriptLoadADVBox);
GENERATELUAWRAPPER(scriptOptionsSetTips);
GENERATELUAWRAPPER(scriptOptionsCanChangeBoxAlpha);
GENERATELUAWRAPPER(scriptImageChoice);
GENERATELUAWRAPPER(scriptSetPositionsSize);

GENERATELUAWRAPPER(scriptSetIncludedFileExtensions);
GENERATELUAWRAPPER(scriptSetForceCapFilenames);

//===============================================================
//===============================================================
//===============================================================

void initLuaWrappers(){
	PUSHLUAWRAPPER(scriptOutputLine,"OutputLine")
	PUSHLUAWRAPPER(scriptClearMessage,"ClearMessage")
	PUSHLUAWRAPPER(scriptOutputLineAll,"OutputLineAll")
	PUSHLUAWRAPPER(scriptWait,"Wait")
	PUSHLUAWRAPPER(scriptDrawScene,"DrawScene")
	PUSHLUAWRAPPER(scriptDrawSceneWithMask,"DrawSceneWithMask")
	PUSHLUAWRAPPER(scriptPlayBGM,"PlayBGM")
	PUSHLUAWRAPPER(scriptFadeoutBGM,"FadeOutBGM")
	PUSHLUAWRAPPER(scriptDrawBustshotWithFiltering,"DrawBustshotWithFiltering");
	PUSHLUAWRAPPER(scriptSetValidityOfInput,"SetValidityOfInput")
	PUSHLUAWRAPPER(scriptFadeAllBustshots,"FadeAllBustshots")
	PUSHLUAWRAPPER(scriptDisableWindow,"DisableWindow")
	PUSHLUAWRAPPER(scriptDrawBustshot,"DrawBustshot")
	PUSHLUAWRAPPER(scriptFadeBustshotWithFiltering,"FadeBustshotWithFiltering")
	PUSHLUAWRAPPER(scriptFadeBustshot,"FadeBustshot")
	PUSHLUAWRAPPER(scriptDrawScene,"DrawBG")
	PUSHLUAWRAPPER(scriptPlaySE,"PlaySE")
	PUSHLUAWRAPPER(scriptStopBGM,"StopBGM")
	PUSHLUAWRAPPER(scriptCallScript,"CallScript") // Somehow, this works. No idea how.
	PUSHLUAWRAPPER(scriptSelect,"Select")
	PUSHLUAWRAPPER(scriptLoadValueFromLocalWork,"LoadValueFromLocalWork")
	PUSHLUAWRAPPER(scriptCallSection,"CallSection")
	PUSHLUAWRAPPER(scriptCallSection,"JumpSection") // TODO - Is this correct?
	PUSHLUAWRAPPER(scriptChangeScene,"ChangeScene")
	PUSHLUAWRAPPER(scriptDrawSprite,"DrawSprite")
	PUSHLUAWRAPPER(scriptMoveSprite,"MoveSprite")
	PUSHLUAWRAPPER(scriptFadeSprite,"FadeSprite")
	PUSHLUAWRAPPER(scriptDrawFilm,"DrawFilm")
	PUSHLUAWRAPPER(scriptFadeFilm,"FadeFilm") // Used for fixing negative and removing films?
	PUSHLUAWRAPPER(scriptFadeBG,"FadeBG")
	PUSHLUAWRAPPER(scriptPlayVoice,"PlayVoice")
	PUSHLUAWRAPPER(scriptDisplayWindow,"DisplayWindow")

	// Options changing commands
	PUSHLUAWRAPPER(scriptOptionsEnableVoiceSetting,"OptionsEnableVoiceSetting");
	PUSHLUAWRAPPER(scriptLoadADVBox,"OptionsLoadADVBox");
	PUSHLUAWRAPPER(scriptOptionsSetTextMode,"OptionsSetTextMode");
	PUSHLUAWRAPPER(scriptOptionsSetTips,"OptionsSetTipExist");
	PUSHLUAWRAPPER(scriptOptionsCanChangeBoxAlpha,"OptionsCanChangeBoxAlpha");
	PUSHLUAWRAPPER(scriptSetPositionsSize,"OptionsSetPositionSize")
	PUSHLUAWRAPPER(scriptSetIncludedFileExtensions,"OptionsSetIncludedFileExtensions")
	PUSHLUAWRAPPER(scriptSetForceCapFilenames,"OptionsSetResourceUppercase")

	// Commands exclusive to my engine
	PUSHLUAWRAPPER(scriptDebugFile,"Debugfile")
	PUSHLUAWRAPPER(scriptMoveBust,"MoveBust")
	PUSHLUAWRAPPER(scriptImageChoice,"ImageChoice");

	// Functions that intentionally do nothing
	PUSHLUAWRAPPER(scriptNotYet,"SetFontId")
	PUSHLUAWRAPPER(scriptNotYet,"SetCharSpacing")
	PUSHLUAWRAPPER(scriptNotYet,"SetLineSpacing")
	PUSHLUAWRAPPER(scriptNotYet,"SetFontSize")
	PUSHLUAWRAPPER(scriptNotYet,"SetNameFormat")
	PUSHLUAWRAPPER(scriptNotYet,"SetScreenAspect")
	PUSHLUAWRAPPER(scriptNotYet,"SetWindowPos")
	PUSHLUAWRAPPER(scriptNotYet,"SetWindowSize")
	PUSHLUAWRAPPER(scriptNotYet,"SetWindowMargins")
	PUSHLUAWRAPPER(scriptNotYet,"SetGUIPosition")
	PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfSaving")
	PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfLoading")
	PUSHLUAWRAPPER(scriptNotYet,"EnableJumpingOfReturnIcon")

	// Functions I should implement
	PUSHLUAWRAPPER(scriptNotYet,"SetSpeedOfMessage")
	PUSHLUAWRAPPER(scriptNotYet,"ShakeScreen")
	PUSHLUAWRAPPER(scriptNotYet,"ShakeScreenSx")
	PUSHLUAWRAPPER(scriptNotYet,"SetDrawingPointOfMessage")
	PUSHLUAWRAPPER(scriptNotYet,"SetStyleOfMessageSwinging")
	PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfWindowDisablingWhenGraphicsControl")
	PUSHLUAWRAPPER(scriptNotYet,"StopSE")
	PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfTextFade")
	PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfSkipping")
	PUSHLUAWRAPPER(scriptNotYet,"GetAchievement")
	PUSHLUAWRAPPER(scriptNotYet,"SetFontOfMessage")
	PUSHLUAWRAPPER(scriptNotYet,"ActivateScreenEffectForcedly")
	PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfUserEffectSpeed")
	PUSHLUAWRAPPER(scriptNotYet,"Negative") // Command for color inversion
									// Negative( 1000, TRUE ); is inveted
									// FadeFilm( 200, TRUE ); fixes it??!
									// Name provably means to negate the colors, or replace the colors with their complementary ones on the other side of the color wheel
									// First arg is maybe time when it fades to inverted and argument is proably if it's inverted
	// Not investigated yet
		PUSHLUAWRAPPER(scriptNotYet,"BlurOffOn")
		PUSHLUAWRAPPER(scriptNotYet,"Break")
		PUSHLUAWRAPPER(scriptNotYet,"ChangeBustshot")
		PUSHLUAWRAPPER(scriptNotYet,"ChangeVolumeOfBGM")
		PUSHLUAWRAPPER(scriptNotYet,"ChapterPreview")
		PUSHLUAWRAPPER(scriptNotYet,"CheckTipsAchievements")
		PUSHLUAWRAPPER(scriptNotYet,"CloseGallery")
		PUSHLUAWRAPPER(scriptNotYet,"ControlMotionOfSprite")
		PUSHLUAWRAPPER(scriptNotYet,"DisableBlur")
		PUSHLUAWRAPPER(scriptNotYet,"DisableEffector")
		PUSHLUAWRAPPER(scriptNotYet,"DisableGradation")
		PUSHLUAWRAPPER(scriptNotYet,"DrawBGWithMask")
		PUSHLUAWRAPPER(scriptNotYet,"DrawBustFace")
		PUSHLUAWRAPPER(scriptNotYet,"DrawFace")
		PUSHLUAWRAPPER(scriptNotYet,"DrawSpriteWithFiltering")
		PUSHLUAWRAPPER(scriptNotYet,"DrawStandgraphic")
		PUSHLUAWRAPPER(scriptNotYet,"EnableBlur")
		PUSHLUAWRAPPER(scriptNotYet,"EnableHorizontalGradation")
		PUSHLUAWRAPPER(scriptNotYet,"EnlargeScreen")
		PUSHLUAWRAPPER(scriptNotYet,"ExecutePlannedControl")
		PUSHLUAWRAPPER(scriptNotYet,"FadeAllBustshots2")
		PUSHLUAWRAPPER(scriptNotYet,"FadeAllBustshots3")
		PUSHLUAWRAPPER(scriptNotYet,"FadeFace")
		PUSHLUAWRAPPER(scriptNotYet,"FadeOutMultiBGM")
		PUSHLUAWRAPPER(scriptNotYet,"FadeOutSE")
		PUSHLUAWRAPPER(scriptNotYet,"FadeScene")
		PUSHLUAWRAPPER(scriptNotYet,"FadeSceneWithMask")
		PUSHLUAWRAPPER(scriptNotYet,"FadeSpriteWithFiltering")
		PUSHLUAWRAPPER(scriptNotYet,"GetLocalFlag")
		PUSHLUAWRAPPER(scriptNotYet,"GetPositionOfSprite")
		PUSHLUAWRAPPER(scriptNotYet,"HideGallery")
		PUSHLUAWRAPPER(scriptNotYet,"JumpScript")
		PUSHLUAWRAPPER(scriptNotYet,"LanguagePrompt")
		PUSHLUAWRAPPER(scriptNotYet,"MoveBustshot")
		PUSHLUAWRAPPER(scriptNotYet,"MoveSpriteEx")
		PUSHLUAWRAPPER(scriptNotYet,"NullOp")
		PUSHLUAWRAPPER(scriptNotYet,"OpenGallery")
		PUSHLUAWRAPPER(scriptNotYet,"PlaceViewTip")
		PUSHLUAWRAPPER(scriptNotYet,"PlaceViewTip2")
		PUSHLUAWRAPPER(scriptNotYet,"PlusStandgraphic1")
		PUSHLUAWRAPPER(scriptNotYet,"PlusStandgraphic2")
		PUSHLUAWRAPPER(scriptNotYet,"PlusStandgraphic3")
		PUSHLUAWRAPPER(scriptNotYet,"PreloadBitmap")
		PUSHLUAWRAPPER(scriptNotYet,"Return")
		PUSHLUAWRAPPER(scriptNotYet,"RevealGallery")
		PUSHLUAWRAPPER(scriptNotYet,"SavePoint")
		PUSHLUAWRAPPER(scriptNotYet,"SetGuiPosition")
		PUSHLUAWRAPPER(scriptNotYet,"SetLocalFlag")
		PUSHLUAWRAPPER(scriptNotYet,"SetSkipAll")
		PUSHLUAWRAPPER(scriptNotYet,"SetTextFade")
		PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfFilmToFace")
		PUSHLUAWRAPPER(scriptNotYet,"SetValidityOfInterface")
		PUSHLUAWRAPPER(scriptNotYet,"SpringText")
		PUSHLUAWRAPPER(scriptNotYet,"StartShakingOfAllObjects")
		PUSHLUAWRAPPER(scriptNotYet,"StartShakingOfBustshot")
		PUSHLUAWRAPPER(scriptNotYet,"StartShakingOfSprite")
		PUSHLUAWRAPPER(scriptNotYet,"StartShakingOfWindow")
		PUSHLUAWRAPPER(scriptNotYet,"StoreValueToLocalWork")
		PUSHLUAWRAPPER(scriptNotYet,"TerminateShakingOfAllObjects")
		PUSHLUAWRAPPER(scriptNotYet,"TerminateShakingOfBustshot")
		PUSHLUAWRAPPER(scriptNotYet,"TerminateShakingOfSprite")
		PUSHLUAWRAPPER(scriptNotYet,"TerminateShakingOfWindow")
		PUSHLUAWRAPPER(scriptNotYet,"TitleScreen")
		PUSHLUAWRAPPER(scriptNotYet,"ViewChapterScreen")
		PUSHLUAWRAPPER(scriptNotYet,"ViewExtras")
		PUSHLUAWRAPPER(scriptNotYet,"ViewTips")
		PUSHLUAWRAPPER(scriptNotYet,"WaitForInput")
		PUSHLUAWRAPPER(scriptNotYet,"WaitToFinishSEPlaying")
		PUSHLUAWRAPPER(scriptNotYet,"WaitToFinishVoicePlaying")
}

#endif