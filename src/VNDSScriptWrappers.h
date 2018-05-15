#ifndef VNDSSCRIPTWRAPPERS
#define VNDSSCRIPTWRAPPERS

#define _VNDSWAITDISABLE 0
// Fadein time in milliseconds used if one not given by vnds script
#define VNDS_IMPLIED_BACKGROUND_FADE 300
#define VNDS_IMPLIED_SETIMG_FADE 200
#define VNDS_HIDE_BOX_ON_BG_CHANGE 1

signed int* pastSetImgX;
signed int* pastSetImgY;
signed int* currentSetImgX;
signed int* currentSetImgY;
void increaseVNDSBustInfoArraysSize(int _oldMaxBusts, int _newMaxBusts){
	pastSetImgX = recalloc(pastSetImgX, _newMaxBusts * sizeof(signed int), _oldMaxBusts * sizeof(signed int));
	pastSetImgY = recalloc(pastSetImgY, _newMaxBusts * sizeof(signed int), _oldMaxBusts * sizeof(signed int));

	currentSetImgX = recalloc(currentSetImgX, _newMaxBusts * sizeof(signed int), _oldMaxBusts * sizeof(signed int));
	currentSetImgY= recalloc(currentSetImgY, _newMaxBusts * sizeof(signed int), _oldMaxBusts * sizeof(signed int));
}


//char nextVndsBustshotSlot=0;

// Helper for two functions
void _vndsChangeScriptFiles(const char* _newFilename){
	char _tempstringconcat[strlen(scriptFolder)+strlen(_newFilename)+1];

	changeMallocString(&currentScriptFilename,_newFilename);

	strcpy(_tempstringconcat,scriptFolder);
	strcat(_tempstringconcat,_newFilename);
	if (checkFileExist(_tempstringconcat)==0){
		LazyMessage("Script file not found,",_tempstringconcat,NULL,NULL);
		return;
	}
	fclose(nathanscriptCurrentOpenFile);
	nathanscriptCurrentOpenFile = fopen(_tempstringconcat,"r");
	nathanscriptCurrentLine=1;
}

// Will always start on an avalible line
void vndswrapper_text(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (currentLine==MAXLINES){
		if (vndsClearAtBottom){
			ClearMessageArray(1);
			currentLine=0;
		}else{
			LastLineLazyFix(&currentLine);
		}
	}
	if (nathanvariableToString(&_passedArguments[0])[0]=='@'){ // Line that doesn't wait for input
		OutputLine(&(nathanvariableToString(&_passedArguments[0])[1]),Line_ContinueAfterTyping,isSkipping);
		currentLine++;
		outputLineWait();
	}else if (nathanvariableToString(&_passedArguments[0])[0]=='!'){ // Blank line that requires button push
		OutputLine("\n",Line_WaitForInput,isSkipping);
		outputLineWait();
	}else if (nathanvariableToString(&_passedArguments[0])[0]=='~'){ // I guess insert a blank line, don't wait for input.
		currentMessages[currentLine][0]=0;
		currentLine++;
	}else{ // Normal line
		OutputLine(nathanvariableToString(&_passedArguments[0]),Line_WaitForInput,isSkipping);
		currentLine++;
		outputLineWait();
	}
	
}
void vndswrapper_choice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* _choiceSet = nathanvariableToString(&_passedArguments[0]);
	//|
	int i;
	short _totalChoices=1;
	for (i=0;i<strlen(_choiceSet);i++){
		if (_choiceSet[i]=='|'){
			_totalChoices++;
		}
	}

	// New array
	nathanscriptVariable* _fakeArgumentArray = malloc(sizeof(nathanscriptVariable)*2);

	_fakeArgumentArray[0].variableType = NATHAN_TYPE_FLOAT;
	_fakeArgumentArray[0].value = malloc(sizeof(float));
	POINTER_TOFLOAT(_fakeArgumentArray[0].value)=_totalChoices;

	char** _argumentTable = malloc(sizeof(char*)*(_totalChoices+1));
	_fakeArgumentArray[1].variableType = NATHAN_TYPE_ARRAY;
	_fakeArgumentArray[1].value = _argumentTable;

	memcpy(&(_argumentTable[0]),&(_totalChoices),sizeof(short));
	int _currentMainStringIndex=0;
	for (i=0;i<_totalChoices;i++){
		int _thisSegmentStartIndex=_currentMainStringIndex;
		for (;;_currentMainStringIndex++){
			if (_choiceSet[_currentMainStringIndex]=='|' || _choiceSet[_currentMainStringIndex]==0){
				_choiceSet[_currentMainStringIndex]=0;
				char* _newBuffer = malloc(strlen(&(_choiceSet[_thisSegmentStartIndex]))+1);
				if (_choiceSet[_thisSegmentStartIndex]==' '){ // Trim up to one leading space
					_thisSegmentStartIndex++;
				}
				strcpy(_newBuffer,&(_choiceSet[_thisSegmentStartIndex]));
				_argumentTable[i+1]=_newBuffer;
				_currentMainStringIndex++;
				break;
			}
		}
	}

	scriptSelect(_fakeArgumentArray,2,NULL,NULL);
	char _numberToStringBuffer[20];
	sprintf(_numberToStringBuffer,"%d",lastSelectionAnswer+1);
	genericSetVar("selected","=",_numberToStringBuffer,&nathanscriptGamevarList,&nathanscriptTotalGamevar);

	freeNathanscriptVariableArray(_fakeArgumentArray,2);
}
void vndswrapper_delay(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping!=1 && capEnabled==1){
		#if !_VNDSWAITDISABLE
			long _totalMillisecondWaitTime = ((nathanvariableToFloat(&_passedArguments[0])/(float)60)*1000);
			wait(_totalMillisecondWaitTime);
		#else
			printf("delay command is disable.\n");
		#endif
	}
}
void vndswrapper_cleartext(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	ClearMessageArray(1);
	if (_numArguments==1 && (nathanvariableToString(&_passedArguments[0])[0]=='!')){
		clearHistory();
	}
}

// bgload filename.extention [dsFadeinFrames]
void vndswrapper_bgload(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* _passedFilename = nathanvariableToString(&_passedArguments[0]);
	#if VNDS_HIDE_BOX_ON_BG_CHANGE
		char _didHideBackground=0;
		if (lastBackgroundFilename==NULL || strcmp(lastBackgroundFilename,_passedFilename)!=0){
			_didHideBackground=1;
			hideTextbox();
		}
	#endif
	DrawScene(_passedFilename,_numArguments==2 ? floor((nathanvariableToFloat(&_passedArguments[1])/60)*1000) : VNDS_IMPLIED_BACKGROUND_FADE);
	nextVndsBustshotSlot=0;
	// Move last image position buffers
	memcpy(pastSetImgX,currentSetImgX,sizeof(signed int)*maxBusts);
	memcpy(pastSetImgY,currentSetImgY,sizeof(signed int)*maxBusts);
	memset(currentSetImgX,0,sizeof(signed int)*maxBusts);
	memset(currentSetImgY,0,sizeof(signed int)*maxBusts);
	//
	#if VNDS_HIDE_BOX_ON_BG_CHANGE
		if (_didHideBackground){
			showTextbox();
		}
	#endif
}
// setimg file x y
// setimg MGE_000099.png 75 0
void vndswrapper_setimg(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	signed int _cachedX = nathanvariableToInt(&_passedArguments[1]);
	signed int _cachedY = nathanvariableToInt(&_passedArguments[2]);
	signed int _imageFadein = VNDS_IMPLIED_SETIMG_FADE;
	if (nextVndsBustshotSlot>=maxBusts){
		increaseVNDSBustInfoArraysSize(maxBusts,nextVndsBustshotSlot+1); // No need to change maxBusts variable here, it will be changed in DrawBustshot call
	}else{
		//printf("do the check.\n");
		//printf("%d: %d;%d;%d;%d\n",nextVndsBustshotSlot,_cachedX,_cachedY,pastSetImgX[nextVndsBustshotSlot],pastSetImgY[nextVndsBustshotSlot]);
		if (_cachedX==pastSetImgX[nextVndsBustshotSlot] && _cachedY==pastSetImgY[nextVndsBustshotSlot]){
			//printf("is confirme dfor smash 5.\n");
			_imageFadein=0;
		}
	}
	currentSetImgX[nextVndsBustshotSlot]=_cachedX;
	currentSetImgY[nextVndsBustshotSlot]=_cachedY;
	DrawBustshot(nextVndsBustshotSlot, nathanvariableToString(&_passedArguments[0]), _cachedX, _cachedY, 0, _imageFadein, (_imageFadein!=0), 0);
	nextVndsBustshotSlot++; // Prepare for next bust
}

// jump file.scr [label]
void vndswrapper_jump(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	_vndsChangeScriptFiles(nathanvariableToString(&_passedArguments[0]));
	if (_numArguments==2){ // Optional label argument
		genericGotoLabel(nathanvariableToString(&_passedArguments[1]));
	}
}

// ENDSCRIPT and END_OF_FILE do the same thing
void vndswrapper_ENDOF(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	_vndsChangeScriptFiles("main.scr");
}

// Same as setvar
void vndswrapper_gsetvar(nathanscriptVariable* _argumentList, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* _passedVariableName = nathanvariableToString(&_argumentList[0]);
	if (_passedVariableName[0]=='~'){
		printf("Wait, I'm pretty sure that isn't/shouldn't be allowed.\nClearing global variables, I mean.");
		return;
	}
	char* _passedModifier = nathanvariableToString(&_argumentList[1]);
	char* _passedNewValue = nathanvariableToString(&_argumentList[2]);
	genericSetVar(_passedVariableName,_passedModifier,_passedNewValue,&nathanscriptGlobalvarList,&nathanscriptTotalGlobalvar);
	// Resave the global variable file
	char _globalsSaveFilePath[strlen(saveFolder)+strlen("vndsGlobals")+1];
	strcpy(_globalsSaveFilePath,saveFolder);
	strcat(_globalsSaveFilePath,"vndsGlobals");
	FILE* fp = fopen(_globalsSaveFilePath,"w");
	unsigned char _tempFileFormat = VNDSGLOBALSSAVEFORMAT;
	fwrite(&_tempFileFormat,sizeof(unsigned char),1,fp);
	saveVariableList(fp,nathanscriptGlobalvarList,nathanscriptTotalGlobalvar);
	fclose(fp);
}
// music file
// music ~
void vndswrapper_music(nathanscriptVariable* _argumentList, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* _passedFilename = nathanvariableToString(&_argumentList[0]);
	if (_passedFilename[0]=='~'){
		StopBGM(0);
		return;
	}
	PlayBGM(_passedFilename,128,0);
}

// sound filename
// sound ~
// the argument to make it play multiple times is a lie, at least for the original VNDS
void vndswrapper_sound(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping==0 && seVolume>0){
		char* _passedFilename = nathanvariableToString(&_passedArguments[0]);
		if (_passedFilename[0]!='~'){
			removeFileExtension(_passedFilename);
			GenericPlaySound(0,_passedFilename,256,PREFER_DIR_SE,seVolume);
		}else{
			// TODO - Stop all sounds
		}
	}
}

#endif