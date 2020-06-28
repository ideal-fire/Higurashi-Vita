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
GENERATELUAWRAPPER(scriptNotYetFlash);
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
GENERATELUAWRAPPER(scriptSetFontSize);
GENERATELUAWRAPPER(scriptNegative);
GENERATELUAWRAPPER(scriptSetAllTextColor);
GENERATELUAWRAPPER(scriptHigurashiGetRandomNumber);
GENERATELUAWRAPPER(scriptHideTextboxAdvanced);
GENERATELUAWRAPPER(scriptEnlargeScreen);
GENERATELUAWRAPPER(scriptDrawSpriteFixedSize);

GENERATELUAWRAPPER(scriptScalePixels);
GENERATELUAWRAPPER(scriptDefineImageName);
GENERATELUAWRAPPER(scriptLoadImageNameSheet);

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
	PUSHLUAWRAPPER(scriptNegative,"Negative") // Command for color inversion
									// Negative( 1000, TRUE ); is inveted
									// FadeFilm( 200, TRUE ); fixes it??!
									// Name provably means to negate the colors, or replace the colors with their complementary ones on the other side of the color wheel
									// First arg is maybe time when it fades to inverted and argument is proably if it's inverted
	PUSHLUAWRAPPER(scriptSetAllTextColor,"SetColorOfMessage")
	PUSHLUAWRAPPER(scriptHigurashiGetRandomNumber,"GetRandomNumber")
	PUSHLUAWRAPPER(scriptHideTextboxAdvanced,"HideWindow")
	PUSHLUAWRAPPER(scriptEnlargeScreen,"EnlargeScreen")
	PUSHLUAWRAPPER(scriptDrawSpriteFixedSize,"DrawSpriteFixedSize");

	// Options changing commands
	PUSHLUAWRAPPER(scriptOptionsEnableVoiceSetting,"OptionsEnableVoiceSetting")
	PUSHLUAWRAPPER(scriptLoadADVBox,"OptionsLoadADVBox")
	PUSHLUAWRAPPER(scriptOptionsSetTextMode,"OptionsSetTextMode")
	PUSHLUAWRAPPER(scriptOptionsSetTips,"OptionsSetTipExist")
	PUSHLUAWRAPPER(scriptOptionsCanChangeBoxAlpha,"OptionsCanChangeBoxAlpha")
	PUSHLUAWRAPPER(scriptSetPositionsSize,"OptionsSetPositionSize")
	PUSHLUAWRAPPER(scriptSetIncludedFileExtensions,"OptionsSetIncludedFileExtensions")
	PUSHLUAWRAPPER(scriptSetFontSize,"OptionsSetFontSize")
	PUSHLUAWRAPPER(scriptScalePixels,"scalePixels")
	PUSHLUAWRAPPER(scriptDefineImageName,"defineImageName")
	PUSHLUAWRAPPER(scriptLoadImageNameSheet,"loadImageNameSheet")
	// set
	PUSHEASYLUAINTSETFUNCTION(oMenuQuit)
	PUSHEASYLUAINTSETFUNCTION(oMenuVNDSSettings)
	PUSHEASYLUAINTSETFUNCTION(oMenuVNDSSave)
	PUSHEASYLUAINTSETFUNCTION(oMenuRestartBGM)
	PUSHEASYLUAINTSETFUNCTION(oMenuArtLocations)
	PUSHEASYLUAINTSETFUNCTION(oMenuVNDSBustFade)
	PUSHEASYLUAINTSETFUNCTION(oMenuDebugButton)
	PUSHEASYLUAINTSETFUNCTION(oMenuTextboxMode)
	PUSHEASYLUAINTSETFUNCTION(oMenuTextOverBG)
	PUSHEASYLUAINTSETFUNCTION(oMenuFontSize)
	PUSHEASYLUAINTSETFUNCTION(dynamicAdvBoxHeight)
	PUSHEASYLUAINTSETFUNCTION(textOnlyOverBackground)
	PUSHEASYLUAINTSETFUNCTION(advboxHeight)
	PUSHEASYLUAINTSETFUNCTION(setADVNameSupport)
	PUSHEASYLUAINTSETFUNCTION(advNamesPersist)
	PUSHEASYLUAINTSETFUNCTION(setTextboxTopPad)
	PUSHEASYLUAINTSETFUNCTION(setTextboxBottomPad)
	PUSHEASYLUAINTSETFUNCTION(setADVNameImageHeight)
	// get
	PUSHEASYLUAINTSETFUNCTION(getTextDisplayMode)

	// Commands exclusive to my engine
	PUSHLUAWRAPPER(scriptDebugFile,"Debugfile")
	PUSHLUAWRAPPER(scriptNotYetFlash,"DebugFlash");
	PUSHLUAWRAPPER(scriptMoveBust,"MoveBust")
	PUSHLUAWRAPPER(scriptImageChoice,"ImageChoice")
	LUAREGISTER(L_setDropshadowColor,"setDropshadowColor")
	// Exclusive lua only vnds interaction commands
	LUAREGISTER(L_setVNDSVar,"setVNDSVar")
	LUAREGISTER(L_settleBust,"settleBust")

}

#endif
