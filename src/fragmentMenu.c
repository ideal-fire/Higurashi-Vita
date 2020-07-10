// https://github.com/MyLegGuy/Higurashi-Vita/issues/20
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
		struct fragmentJson* j = fragmentInfo[fragPlayOrder[i]-1];
		_connectionList[i]=(playerLanguage ?  j->title : j->titlejp);
	}
	int _choice = showMenu(0,"Connections",playedFrags,_connectionList,1);
	if (_choice>=0){
		RunScript(scriptFolder,fragmentInfo[fragPlayOrder[_choice]-1]->script,1);
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
// returns the 1-based index of when it was fully completed.
// compare it to _indexById to know if it's a shattered fragment
// returns -1 if it was attempted, but it's shattered and still incomplete
// returns -2 if not attempted
int requirementsMet(int _id, int* _indexById, int* _cachedCompletions){
	struct fragmentJson* _json = fragmentInfo[_id-1];
	int _maxSlot = _indexById[_id-1]; // all of these are offset by 1, so it's okay
	if (_maxSlot==0){
		return -2;
	}
	int _completedOn=_maxSlot;
	for (int i=1;i<=_json->prereqs[0];++i){
		int _curIndex = _json->prereqs[i]-1;
		if (_cachedCompletions[_curIndex]==0){
			_cachedCompletions[_curIndex]=requirementsMet(_json->prereqs[i],_indexById,_cachedCompletions);
		}
		if (_cachedCompletions[_curIndex]<0){
			return -1;
		}
		if (_cachedCompletions[_curIndex]>_completedOn){
			_completedOn=_cachedCompletions[_curIndex];
		}
	}
	return _completedOn;
}
// _cachedCompletions stores which slot a fragment was fully complete on.
// it will be either the index which a fragment was completed on, or the index where its last prereq was completed.
// when using the id as index, subtract 1.
// for both arrays, subtract 1 from their values. 0 stands for not attempted fragment.
void initReqChecking(int** _retIndexById, int** _retCachedCompletions){
	int* _indexById = malloc(numFragments*sizeof(int));
	memset(_indexById,0,numFragments*sizeof(int));
	int* _cachedCompletions = malloc(numFragments*sizeof(int));
	memset(_cachedCompletions,0,numFragments*sizeof(int));
	for (int i=0;i<playedFrags;++i){
		_indexById[fragPlayOrder[i]-1]=i+1;
	}
	*_retIndexById=_indexById;
	*_retCachedCompletions=_cachedCompletions;
}
void regenOptionProps(optionProp* _ret, int* _indexById, int* _cachedCompletions){
	for (int i=0;i<playedFrags;++i){
		int _index = fragPlayOrder[i]-1;
		if (_ret[_index]!=OPTIONPROP_GOODCOLOR){
			int _cstate = requirementsMet(fragPlayOrder[i],_indexById,_cachedCompletions);
			if (_cstate==-2){
				_ret[_index]=0;
			}else if (_cstate==-1){
				_ret[_index]=OPTIONPROP_BADCOLOR;
			}else{
				_ret[_index]=OPTIONPROP_GOODCOLOR;
				if (_cstate>_indexById[_index]){
					_ret[_index]|=OPTIONPROP_BADCOLOR;
				}
			}
		}
	}
}
void connectFragmentMenu(){
	char* _optionNames[numFragments];
	char _showMap[numFragments];
	memset(_showMap,1,numFragments);
	// TODO - hide the last 3 or whatever

	int* _indexById;
	int* _cachedCompletions;
	initReqChecking(&_indexById,&_cachedCompletions);
	optionProp* _props = malloc(numFragments*sizeof(optionProp));
	memset(_props,0,numFragments*sizeof(optionProp));
	regenOptionProps(_props,_indexById,_cachedCompletions);
	
	for (int i=0;i<numFragments;++i){
		_optionNames[i]=playerLanguage ? fragmentInfo[i]->title : fragmentInfo[i]->titlejp;
	}
	int _choice=0;
	while(1){
		{
			int _fragLoopOn;
			if (getLocalFlag("LFragmentLoop",&_fragLoopOn) && !_fragLoopOn){
				break;
			}
		}
		_realHeight = screenHeight;
		screenHeight=screenHeight-currentTextHeight*3-DRAWDESCRIPTIONPADDING(screenHeight)*4;
		_choice = showMenuAdvanced(_choice, "Fragment list", numFragments, _optionNames, NULL, _showMap, _props, NULL, MENUPROP_CANPAGEUPDOWN | MENUPROP_CANQUIT, _drawDescription);
		screenHeight=_realHeight;
		if (_lastDescriptionIndex!=-1){
			_lastDescriptionIndex=-1;
			freeWrappedText(_descLinesCount,_descriptionLines);
		}
		if (_choice>=0){
			//RunScript(scriptFolder,fragmentInfo[_choice]->script,1);
			if (_indexById[fragmentInfo[_choice]->id-1]==0){
				setFragPlayed(fragmentInfo[_choice]->id);
				// update information for requirement checking
				_indexById[fragmentInfo[_choice]->id-1]=playedFrags;
				for (int j=0;j<numFragments;++j){
					if (_cachedCompletions[j]<0){
						_cachedCompletions[j]=0;
					}
				}
				regenOptionProps(_props,_indexById,_cachedCompletions);
			}
			saveHiguGame();
		}else{
			break;
		}
	}
	free(_props);
	free(_indexById);
	free(_cachedCompletions);
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
	free(fragPlayOrder);
	playedFrags=0;
	fragPlayOrder=NULL;
	PlayMenuSound();
	saveHiguGame();
}
void fragmentMenu(){
	int _choice=0;
	while(1){
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
