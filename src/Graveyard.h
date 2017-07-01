	//In my testing, 444 HZ cpu makes loading and freeing 130 KB sound file take about 200 miliseconds less and loading the biggest image file in Onikakushi PS3 patch take about 50 miliseconds less
	//
	//image width / screen width
	//image height / screen height
	//
	//the bigger result is the one we lock to
	//we multiple image width and height by said result to get resulting image
	
	//1280x800
	//
	//(?<!.)char(?!\*)
	//Make sure there's nothing behind char and that it doesn't have * after it
	//I can use this to find all normal char and replace it with signed char
	

	//CROSSMUSIC* testsong = LoadMusic("app0:a/testogg.ogg");
	//WriteSDLError();
	//PlayMusic(testsong);
	//WriteSDLError();
	//
	//FILE *fp;
	//fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	//fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	//fclose(fp);

	//SetClearColor(255,0,0,255);
	//CrossTexture* testTex = SafeLoadPNG("ux0:data/HIGURASHI/StreamingAssets/CG/c.png");
	//CrossTexture* testTex2 = SafeLoadPNG("ux0:data/HIGURASHI/StreamingAssets/CG/bg_080.png");
	//vita2d_texture_set_filters(testTex2,vita2d_texture_get_min_filter(testTex),vita2d_texture_get_mag_filter(testTex));
	//while (1==1){
	//	StartDrawingA();
	//	vita2d_draw_texture(testTex2,0,0);
	//	EndDrawingA();
	//}

	//printf("%s\n",currentPresetFilename);

	//fread
	//LoadPreset("./StreamingAssets/Presets/Watanagashi.txt");
	//return 1;
	
	// Sound loading test
	//scePowerSetArmClockFrequency(444);
	////scePowerSetBusClockFrequency(222);
	////scePowerSetGpuClockFrequency(222);
	//u64 startLoadTest = GetTicks();
	//CROSSSFX* noobSound = LoadSound("ux0:data/HIGURASHI/StreamingAssets/SE/s19/02/990200069.ogg");
	//FreeSound(noobSound);
	//u64 testResult = GetTicks()-startLoadTest;
	//WriteIntToDebugFile(testResult);
	//return 1;
	// Graphics loading test
	//scePowerSetArmClockFrequency(444);
	//scePowerSetBusClockFrequency(222);
	//scePowerSetGpuClockFrequency(222);
	//u64 startLoadTest = GetTicks();
	//CrossTexture* noobSound = LoadPNG("ux0:data/HIGURASHI/StreamingAssets/CG/haikei-.png");
	//FreeTexture(noobSound);
	//u64 testResult = GetTicks()-startLoadTest;
	//WriteIntToDebugFile(testResult);
	//return 1;

	//currentGameStatus=0;
	//LoadPreset("./StreamingAssets/Presets/Onikakushi.txt");
	//currentGameStatus=3;
	//int noob=19;
	//printf("Bout to print\n");
	//printf("%s\n",currentPresetTipList.theArray[noob]);
	//strcpy(nextScriptToLoad,"onik_001");
	//currentGameStatus=3;
	//// luaL_dofile(L,"ux0:data/HIGURASHI/StreamingAssets/Scripts/onik_001.txt");
	//RunScript(SCRIPTFOLDER,"onik_001",1);
	////currentGameStatus=3;
	//currentGameStatus=0;
	//char testtext[10];
	//strcpy(testtext,"aaacbbb");
	//testtext[3]=0x0A;
	//while (1){
	//	FpsCapStart();
	//	StartDrawingA();
	//	//DrawTextColored(5,5,"Noob",32,255,0,0);
	//	//DrawText(int x, int y, const char* text, float size){
	//	DrawText(5,5,testtext,fontSize);
	//	EndDrawingA();
	//	FpsCapWait();
	//}

	// Hypothesis:
	// I think that if I make one DrawText call with more text, it will be faster than multiple DrawText calls with the same amount of text in total
	// Experiment:
	// ONE MULTIPLE
	// 167 167
	// 167 167
	// 167 167
	// Conclusion:
	// It's not faster.
	// In fact, they're exactly the same speed.
	//char goodtext[] = "This is first line\nThis is second line\nThis is third line\nThis is 4 line\nThis is 5 line\nThis is 6 line.";
	//int k=0;
	//// Make sure stuff needed  to display this text is loaded before the test
	//for (k=0;k<10;k++){
	//	StartDrawingA();
	//	DrawText(5,5,goodtext,fontSize);
	//	EndDrawingA();
	//}
	//
	//u64 startLoadTest = GetTicks();
	//// The test
	//for (k=0;k<10;k++){
	//	StartDrawingA();
	//	DrawText(5,5,goodtext,fontSize);
	//	EndDrawingA();
	//}
	//u64 testResult = GetTicks()-startLoadTest;
	//WriteIntToDebugFile(testResult);
	//WriteToDebugFile("test2");
	//char goodtextA[] = "This is first line";
	//char goodTextB[] = "This is second line";
	//char goodTextC[] = "This is third line.";
	//char goodTextD[] = "This is 4 line";
	//char goodTextE[] = "This is 5 line";
	//char goodTextF[] = "This is 6 line.";
	//for (k=0;k<10;k++){
	//	StartDrawingA();
	//	DrawText(5,5,goodtextA,fontSize);
	//	DrawText(5,50,goodTextB,fontSize);
	//	DrawText(5,100,goodTextC,fontSize);
	//	DrawText(5,150,goodTextD,fontSize);
	//	DrawText(5,200,goodTextE,fontSize);
	//	DrawText(5,250,goodTextF,fontSize);
	//	EndDrawingA();
	//}
	//startLoadTest = GetTicks();
	//for (k=0;k<10;k++){
	//	StartDrawingA();
	//	DrawText(5,5,goodtextA,fontSize);
	//	DrawText(5,50,goodTextB,fontSize);
	//	DrawText(5,100,goodTextC,fontSize);
	//	DrawText(5,150,goodTextD,fontSize);
	//	DrawText(5,200,goodTextE,fontSize);
	//	DrawText(5,250,goodTextF,fontSize);
	//	EndDrawingA();
	//}
	//testResult = GetTicks()-startLoadTest;
	//WriteIntToDebugFile(testResult);
	//WriteToDebugFile("done");
	//return 1;