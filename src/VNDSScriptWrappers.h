#ifndef VNDSSCRIPTWRAPPERS
#define VNDSSCRIPTWRAPPERS

#define VNDSDOESFADEREPLACE 0
#define _VNDSWAITDISABLE 0
// Fadein time in milliseconds used if one not given by vnds script
#define VNDS_IMPLIED_BACKGROUND_FADE 3000
#define VNDS_IMPLIED_SETIMG_FADE 300
#define VNDS_HIDE_BOX_ON_BG_CHANGE 1
#define MAXBUSTQUEUE 10

typedef struct{
	char* filename;
	signed int x;
	signed int y;
}bustQueue;

bustQueue currentBustQueue[MAXBUSTQUEUE] = {NULL};
signed char nextBustQueueSlot=0;
char isSpecialBustChange=0;

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

char isNear(int _unknownSpot, int _originalSpot , int _leeway){
	if (_originalSpot>=_unknownSpot-_leeway && _originalSpot<=_unknownSpot+_leeway){
		return 1;
	}
	return 0;
}
int getEmptyBustshotSlot(){
	int i;
	for (i=0;i<maxBusts;++i){
		if (Busts[i].isActive==0){
			return i;
		}
	}
	return i; // Array automatically expands when calling DrawBustshot
}
int getVNDSImgFade(){
	return vndsSpritesFade ? VNDS_IMPLIED_SETIMG_FADE : 0;
}
// Called in between VNDS command executions. Return < 0 if you don't want to execute the passed command.
int inBetweenVNDSLines(int _aboutToCommandIndex){
	if (isSpecialBustChange){
		if (_aboutToCommandIndex!=foundSetImgIndex){
			isSpecialBustChange=0;

			// Fadeout all bustshots that we definitely don't need anymore because they don't overlap any new ones
			signed int i, j;
			for (i=0;i<maxBusts;++i){
				if (Busts[i].isActive){
					char _bustIsSaved=0;
					for (j=0;j<nextBustQueueSlot;++j){
						//printf("%d x (%d and %d) y (%d and %d)\n",i,currentBustQueue[j].x,Busts[i].xOffset,currentBustQueue[j].y,Busts[i].yOffset);
						if (isNear(currentBustQueue[j].x,Busts[i].xOffset,20) && isNear(currentBustQueue[j].y,Busts[i].yOffset,20)){
							//printf("confirmed.\n");
							_bustIsSaved=1;
							break;
						}
					}
					if (_bustIsSaved==0){
						//printf("Remove unreplaced character %d\n",i);
						FadeBustshot(i,getVNDSImgFade(),0);
					}
				}
			}
			
			// Forget about all the duplicates
			for (j=0;j<nextBustQueueSlot;++j){
				for (i=0;i<maxBusts;++i){
					if (Busts[i].isActive){
						if (isNear(currentBustQueue[j].x,Busts[i].xOffset,20) && isNear(currentBustQueue[j].y,Busts[i].yOffset,20)){
							//printf("%s;%s;%d;%d\n",currentBustQueue[j].filename,Busts[i].relativeFilename,strlen(currentBustQueue[j].filename),strlen(Busts[i].relativeFilename));
							if (strcmp(currentBustQueue[j].filename,Busts[i].relativeFilename)==0){
								//printf("%d is a duplicate.",i);
								free(currentBustQueue[j].filename);
								currentBustQueue[j].filename=NULL;
								//MoveBustSlot(i,nextVndsBustshotSlot);
								//++nextVndsBustshotSlot;
								break;
							}
						}
					}
				}
			}

			// Show our new stuff
			for (j=0;j<nextBustQueueSlot;++j){
				// Don't do removed duplicates
				if (currentBustQueue[j].filename!=NULL){

					// If this is an expression change, instantly delete the old bust and show the new one
					char _didDeleteAParent=0;
					for (i=0;i<maxBusts;++i){
						if (Busts[i].isActive && Busts[i].bustStatus == BUST_STATUS_NORMAL){
							if (isNear(currentBustQueue[j].x,Busts[i].xOffset,20) && isNear(currentBustQueue[j].y,Busts[i].yOffset,20)){
								if (VNDSDOESFADEREPLACE){
									DrawBustshot(i, currentBustQueue[j].filename, currentBustQueue[j].x, currentBustQueue[j].y, i, getVNDSImgFade(), 0, 0);
								}else{
									//printf("Remove %d because it's too close.",i);
									FadeBustshot(i,0,0);
									DrawBustshot(getEmptyBustshotSlot(), currentBustQueue[j].filename, currentBustQueue[j].x, currentBustQueue[j].y, getEmptyBustshotSlot(), 0, 0, 0);
								}
								_didDeleteAParent=1;
								break;
							}
						}
					}

					// If it's a brand new bust, fade it in.
					if (_didDeleteAParent==0){
						//printf("will fadein %d\n",getEmptyBustshotSlot());
						DrawBustshot(getEmptyBustshotSlot(), currentBustQueue[j].filename, currentBustQueue[j].x, currentBustQueue[j].y, getEmptyBustshotSlot(), getVNDSImgFade(), 0, 0);
					}
					free(currentBustQueue[j].filename);
				}
			}

			waitForBustFade();

			// Shift busts back to their lowest possible slots
			for (i=1;i<maxBusts;++i){
				int _foundNewIndex = getEmptyBustshotSlot();
				if (_foundNewIndex!=i){
					MoveBustSlot(i,_foundNewIndex);
				}
			}
			nextVndsBustshotSlot = getEmptyBustshotSlot();
		}
	}
	return 0;
}

//char nextVndsBustshotSlot=0;

// Helper for two functions
void _vndsChangeScriptFiles(const char* _newFilename){
	char _tempstringconcat[strlen(scriptFolder)+strlen(_newFilename)+1];

	changeMallocString(&currentScriptFilename,_newFilename);

	strcpy(_tempstringconcat,scriptFolder);
	strcat(_tempstringconcat,_newFilename);
	if (checkFileExist(_tempstringconcat)==0){
		easyMessagef(1,"Script file not found, %s",_tempstringconcat);
		return;
	}
	crossfclose(nathanscriptCurrentOpenFile);
	nathanscriptCurrentOpenFile = crossfopen(_tempstringconcat,"rb");
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
	char* _gottenMessageString = nathanvariableToString(&_passedArguments[0]);

	// If there's no point in adding a blank line because we have an empty ADV box.
	char _shouldSkipSymbolLines=0;
	if (strlen(_gottenMessageString)<=1 && currentLine==0 && gameTextDisplayMode==TEXTMODE_ADV){
		_shouldSkipSymbolLines=1;
	}

	if (_gottenMessageString[0]=='@'){ // Line that doesn't wait for input
		if (!_shouldSkipSymbolLines){
			OutputLine(&(_gottenMessageString[1]),Line_ContinueAfterTyping,isSkipping);
			currentLine++;
			outputLineWait();
		}
	}else if (_gottenMessageString[0]=='!'){ // Blank line that requires button push
		if (!_shouldSkipSymbolLines){
			OutputLine("\n",Line_WaitForInput,isSkipping);
		}
		outputLineWait();
	}else if (_gottenMessageString[0]=='~'){ // I guess insert a blank line, don't wait for input.
		if (!_shouldSkipSymbolLines){
			currentMessages[currentLine][0]=0;
			currentLine++;
		}
	}else{ // Normal line
		OutputLine(_gottenMessageString,Line_WaitForInput,isSkipping);
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

				// Hima tips 09 doesn't seperate the variables with spaces in the script so they're not replaced correctly. This fixes.
				replaceIfIsVariable(&_newBuffer);

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
	nathanscriptConvertVariable(&(nathanscriptGetGameVariable("selected")->variable),NATHAN_TYPE_FLOAT);

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
	if (lastBackgroundFilename!=NULL && strcmp(lastBackgroundFilename,_passedFilename)==0){
		// If we're not really changing the background, just busts
		isSpecialBustChange=1;
		nextBustQueueSlot=0;
		nextVndsBustshotSlot=0;
	}else{
		#if VNDS_HIDE_BOX_ON_BG_CHANGE
			if (lastBackgroundFilename==NULL || strcmp(lastBackgroundFilename,_passedFilename)!=0){
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
		//#if VNDS_HIDE_BOX_ON_BG_CHANGE
		//	if (_didHideBackground){
		//		if (gameTextDisplayMode==TEXTMODE_ADV){
		//			ClearMessageArray(0); // Don't want fade transition so we need to do this here before OutputLine
		//		}
		//		//showTextbox();
		//	}
		//#endif
	}
}
// setimg file x y
// setimg MGE_000099.png 75 0
void vndswrapper_setimg(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* _passedFilename = nathanvariableToString(&_passedArguments[0]);
	if ((strlen(_passedFilename)==1 && _passedFilename[0]=='~') || _numArguments<3){ // I saw this in a game called YUME MIRU KUSURI.
		return;
	}
	if (!isSpecialBustChange){
		if (nextVndsBustshotSlot>=maxBusts){
			increaseVNDSBustInfoArraysSize(maxBusts,nextVndsBustshotSlot+1); // No need to change maxBusts variable here, it will be changed in DrawBustshot call
		}
		currentSetImgX[nextVndsBustshotSlot]=nathanvariableToInt(&_passedArguments[1]);
		currentSetImgY[nextVndsBustshotSlot]=nathanvariableToInt(&_passedArguments[2]);
		DrawBustshot(nextVndsBustshotSlot, _passedFilename, currentSetImgX[nextVndsBustshotSlot], currentSetImgY[nextVndsBustshotSlot], nextVndsBustshotSlot, getVNDSImgFade(), 1, 0);
		nextVndsBustshotSlot++; // Prepare for next bust
	}else{
		if (nextBustQueueSlot==MAXBUSTQUEUE){
			easyMessagef(1,"Too many busts in queue. No action will be taken. Report to MyLegGuy.");
		}else{
			currentBustQueue[nextBustQueueSlot].x = nathanvariableToInt(&_passedArguments[1]);
			currentBustQueue[nextBustQueueSlot].y = nathanvariableToInt(&_passedArguments[2]);
			currentBustQueue[nextBustQueueSlot].filename = mallocForString(_passedFilename);
			nextBustQueueSlot++;
		}
	}
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
	genericSetVarCommand(_argumentList, _totalArguments, _returnedReturnArray, _returnArraySize,&nathanscriptGlobalvarList,&nathanscriptTotalGlobalvar,0);

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
			// Change to OGG if needed
			if (strcmp(getFileExtension(_passedFilename),"aac")==0){
				_passedFilename[strlen(_passedFilename)-3]='\0';
				strcat(_passedFilename,"ogg");
			}
			//removeFileExtension(_passedFilename);
			GenericPlayGameSound(0,_passedFilename,256,PREFER_DIR_SE,seVolume);
		}else{
			// TODO - Stop all sounds
		}
	}
}

#endif