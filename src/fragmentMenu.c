// https://github.com/MyLegGuy/Higurashi-Vita/issues/20
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/text.h>
#include <goodbrew/useful.h>
#include "jsonParser.h"
#include "main.h"
#undef wasJustPressed
#undef isDown

#define BROKENSUFFIX "[*]"

struct fragmentJson{
	int id;
	char* title;
	char* description;
	char* titlejp;
	char* descriptionjp;
	char* script;
	int* prereqs;
};
struct fragmentJson** fragmentInfo;
int numFragments;
//
#define FSTATUS_BROKEN 1
#define FSTATUS_PLAYED 2
char* fragStatus; // by index
// hardcoded config. these are by ID
int lockedUntilPrereq=51; // all it's requirements must be played
int bonusNoErrFrag=52; // you unlock it if none are broken
//
char fragmentsModeOn(){
	int _fragLoopOn;
	return (getLocalFlag("LFragmentLoop",&_fragLoopOn) && _fragLoopOn);
}
void parseFragmentFile(const char* _filename){
	struct fragmentJson _sample;
	struct jsonTarget _targets[8] = {
		{"Id",(char*)&(_sample.id)-(char*)&_sample},
		{"Title",(char*)&(_sample.title)-(char*)&_sample},
		{"Description",(char*)&(_sample.description)-(char*)&_sample},
		{"TitleJp",(char*)&(_sample.titlejp)-(char*)&_sample},
		{"DescriptionJp",(char*)&(_sample.descriptionjp)-(char*)&_sample},
		{"Script",(char*)&(_sample.script)-(char*)&_sample},
		{"Prereqs",(char*)&(_sample.prereqs)-(char*)&_sample},
	};
	void** _arr;
	parseJson(_filename,_targets,sizeof(_targets)/sizeof(struct jsonTarget),sizeof(struct fragmentJson),&_arr,&numFragments);
	fragmentInfo = malloc(sizeof(struct fragmentJson*)*numFragments);
	for (int i=0;i<numFragments;++i){
		fragmentInfo[i]=_arr[i];
	}
	if (!fragStatus){
		fragStatus = malloc(numFragments);
		memset(fragStatus,0,numFragments);
	}
}
static int _lastDescriptionIndex=-1;
static int _descLinesCount;
static char** _descriptionLines;
static int _realHeight;
#define DRAWDESCRIPTIONPADDING(x) (x/300)
int _drawDescription(int i){
	int _padding = DRAWDESCRIPTIONPADDING(_realHeight);
	if (_lastDescriptionIndex!=i){
		freeWrappedText(_descLinesCount,_descriptionLines);
		wrapText(playerLanguage ? fragmentInfo[i]->description : fragmentInfo[i]->descriptionjp, &_descLinesCount, &_descriptionLines, screenWidth-_padding*4);
	}
	int _rectH=_realHeight-screenHeight;
	int _rectStart=_realHeight-_rectH;
	{
		int _lineSpace=(_rectH-_padding*2)/currentTextHeight;
		if (_descLinesCount>_lineSpace){
			int _space=(_descLinesCount-_lineSpace)*currentTextHeight;
			_rectStart-=_space;
			_rectH+=_space;
		}
	}
	drawRectangle(0,_rectStart,screenWidth,_rectH,255,255,255,255);
	_rectStart+=_padding;
	drawRectangle(_padding,_rectStart,screenWidth-_padding*2,_rectH-_padding*2,0,0,0,255);
	_rectStart+=_padding;
	drawWrappedText(_padding*2,_rectStart,_descriptionLines,_descLinesCount,255,255,255,255);

	{ // debug draw
		int _x = screenWidth-textWidth(normalFont,"aaaaaaaaaaa");
		gbDrawTextf(normalFont,_x,screenHeight/3,255,255,255,255,"DEBUG",fragmentInfo[i]->id);
		gbDrawTextf(normalFont,_x,screenHeight/3+currentTextHeight,255,255,255,255,"ID: %d",fragmentInfo[i]->id);
		gbDrawTextf(normalFont,_x,screenHeight/3+currentTextHeight*2,255,255,255,255,"prereqs:");
		for (int k=1;k<=fragmentInfo[i]->prereqs[0];++k){
			gbDrawTextf(normalFont,_x,screenHeight/3+(currentTextHeight)*(k+2),255,255,255,255,"%d",fragmentInfo[i]->prereqs[k]);
		}
	}
	return 0;
}
char fragPlayable(int _index){
	struct fragmentJson* j = fragmentInfo[_index];
	for (int i=1;i<=j->prereqs[0];++i){
		if (!(fragStatus[j->prereqs[i]-1] & FSTATUS_PLAYED)){
			return 0;
		}
	}
	return 1;
}
// all but the last one
char didPerfect(){
	for (int i=0;i<numFragments-1;++i){
		if (fragStatus[i]!=FSTATUS_PLAYED){
			return 0;
		}
	}
	return 1;
}
void regenOptionProps(optionProp* _props){
	for (int i=0;i<numFragments;++i){
		if (fragStatus[i] & FSTATUS_PLAYED){
			_props[i]=OPTIONPROP_GOODCOLOR;
		}else{
			if (fragStatus[i] & FSTATUS_BROKEN){
				_props[i]=OPTIONPROP_BADCOLOR;
				if (fragPlayable(i)){
					_props[i]|=OPTIONPROP_GOODCOLOR;
				}
			}else{
				_props[i]=0;
			}
		}
	}
}
void connectFragmentMenu(){
	char* _optionNames[numFragments];
	char _showMap[numFragments];
	memset(_showMap,1,numFragments);

	optionProp* _props = malloc(numFragments*sizeof(optionProp));
	memset(_props,0,numFragments*sizeof(optionProp));
	regenOptionProps(_props);
	_showMap[bonusNoErrFrag-1]=didPerfect();
	
	for (int i=0;i<numFragments;++i){
		char* _baseStr=(playerLanguage ? fragmentInfo[i]->title : fragmentInfo[i]->titlejp);
		if (fragStatus[i] & FSTATUS_BROKEN){
			_optionNames[i]=easyCombineStrings(2,_baseStr,BROKENSUFFIX);
		}else{
			_optionNames[i]=strdup(_baseStr);
		}
	}
	int _choice=0;
	while(1){
		if (!fragmentsModeOn()){
			break;
		}
		_showMap[lockedUntilPrereq-1]=fragPlayable(lockedUntilPrereq-1);
		char _retClickInfo;
		{
			_realHeight = screenHeight;
			screenHeight=screenHeight-currentTextHeight*3-DRAWDESCRIPTIONPADDING(screenHeight)*4;
			_choice = showMenuAdvanced(_choice, "Fragment list", numFragments, _optionNames, NULL, _showMap, _props, &_retClickInfo, MENUPROP_CANPAGEUPDOWN | MENUPROP_CANQUIT, _drawDescription);
			screenHeight=_realHeight;
		}
		if (_lastDescriptionIndex!=-1){
			_lastDescriptionIndex=-1;
			freeWrappedText(_descLinesCount,_descriptionLines);
		}
		if (_choice>=0){
			if (!(fragStatus[_choice] & FSTATUS_PLAYED)){
				if (fragPlayable(_choice)){
					fragStatus[_choice]|=FSTATUS_PLAYED;
				}else if (!(fragStatus[_choice] & FSTATUS_BROKEN)){
					// todo - play animation if i so desire
					free(_optionNames[_choice]);
					_optionNames[_choice]=easyCombineStrings(2,(playerLanguage ? fragmentInfo[_choice]->title : fragmentInfo[_choice]->titlejp),BROKENSUFFIX);
					fragStatus[_choice]|=FSTATUS_BROKEN;
				}
				_showMap[bonusNoErrFrag-1]=didPerfect();
				regenOptionProps(_props);
			}
			if ((fragStatus[_choice] &FSTATUS_PLAYED) && !(_retClickInfo & MENURET_LBUTTON)){
				RunScript(scriptFolder,fragmentInfo[_choice]->script,1);
			}
			saveHiguGame();
		}else{
			break;
		}
	}
	free(_props);
}
#define RESETCONNECTIONSTIME 2000
#define RESETCONNECTIONSTEXT "Resetting..."
void startResetConnections(){
	u64 _startTime=getMilli();
	while(1){
		startDrawing();
		unsigned char _p=partMoveFillsCapped(getMilli(),_startTime,RESETCONNECTIONSTIME,255);
		drawRectangle(0,0,screenWidth,screenHeight,_p,_p,_p,255);
		gbDrawText(normalFont,easyCenter(textWidth(normalFont,RESETCONNECTIONSTEXT),screenWidth),easyCenter(textHeight(normalFont),screenHeight),RESETCONNECTIONSTEXT,255-_p,255-_p,255-_p);
		endDrawing();
		controlsStart();
		if (wasJustPressed(BUTTON_B)){
			return;
		}
		if (getMilli()>=_startTime+RESETCONNECTIONSTIME){
			break;
		}
		controlsEnd();
	}
	memset(fragStatus,0,numFragments);
	PlayMenuSound();
	saveHiguGame();
}
void fragmentMenu(){
	int _choice=0;
	while(1){
		if (!fragmentsModeOn()){
			break;
		}
		char* _options[] = {
			"Fragment list",
			"Reset my connections",
			"Back",
		};
		_choice = showMenu(_choice,"Fragments",sizeof(_options)/sizeof(char*),_options,1);
		switch(_choice){
			case 0:
				connectFragmentMenu();
				break;
			case 1:
				startResetConnections();
				break;
			case -1:
			case 2:
				return;
		}
	}
}
