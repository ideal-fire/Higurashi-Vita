#include <stdio.h>
#include <stdlib.h>
#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/text.h>
#include "jsonParser.h"
#include "main.h"
#undef wasJustPressed
#undef isDown

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
int* fragPlayOrder;
int playedFrags;
//
void setFragPlayed(int _id){
	fragPlayOrder = realloc(fragPlayOrder,sizeof(int)*(++playedFrags));
	fragPlayOrder[playedFrags-1]=_id;
}
struct fragmentJson* getFragmentById(int _id){
	if (_id<=numFragments){
		if (fragmentInfo[_id-1]->id==_id){
			return fragmentInfo[_id-1];
		}
	}
	for (int i=0;i<numFragments;++i){
		if (fragmentInfo[i]->id==_id){
			return fragmentInfo[i];
		}
	}
	fprintf(stderr,"unknown fragment id\n");
	return NULL;
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
}
void viewConnections(){
	char** _connectionList = malloc(sizeof(char*)*playedFrags);
	for (int i=0;i<playedFrags;++i){
		struct fragmentJson* j = getFragmentById(fragPlayOrder[i]);
		_connectionList[i]=(playerLanguage ?  j->title : j->titlejp);
	}
	int _choice = showMenu(0,"Connections",playedFrags,_connectionList,1);
	if (_choice>=0){
		RunScript(scriptFolder,getFragmentById(fragPlayOrder[_choice])->script,1);
	}
	free(_connectionList);
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
	return 0;
}
void connectFragmentMenu(){
	char* _optionNames[numFragments];
	char _showMap[numFragments];
	optionProp _props[numFragments];
	for (int i=0;i<numFragments;++i){
		_optionNames[i]=playerLanguage ? fragmentInfo[i]->title : fragmentInfo[i]->titlejp;
		_showMap[i]=1;
		_props[i]=0;
	}
	_realHeight = screenHeight;
	screenHeight=screenHeight-currentTextHeight*3-DRAWDESCRIPTIONPADDING(screenHeight)*4;
	int _choice = showMenuAdvanced(0, "Connect Fragment", numFragments, _optionNames, NULL, _showMap, _props, NULL, MENUPROP_CANPAGEUPDOWN | MENUPROP_CANQUIT, _drawDescription);
	screenHeight=_realHeight;
	
	_lastDescriptionIndex=-1;
	if (_lastDescriptionIndex!=-1){
		_lastDescriptionIndex=-1;
		freeWrappedText(_descLinesCount,_descriptionLines);
	}
}
#define RESETCONNECTIONSTIME 2000
#define RESETCONNECTIONSTEXT "Breaking..."
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
	PlayMenuSound();
}
void fragmentMenu(){
	int _choice=0;
	while(1){
		char* _options[] = {
			"Connect fragment",
			"View my connections",
			"Reset my connections",
			"Back",
		};
		_choice = showMenu(_choice,"Fragments",sizeof(_options)/sizeof(char*),_options,1);
		switch(_choice){
			case 0:
				connectFragmentMenu();
				break;
			case 1:
				viewConnections();
				break;
			case 2:
				startResetConnections();
				break;
			case -1:
			case 3:
				return;
		}
	}
	
	
}
