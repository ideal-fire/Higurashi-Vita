// nooooo you have to import a library you can't just write what you need
// numbers: int
// strings: char*
// arrays: int*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <goodbrew/config.h>
#include <goodbrew/base.h>
#include <goodbrew/useful.h>
#include <goodbrew/text.h>
#include "main.h"
#include "jsonParser.h"
static char isValidAtoiString(char* _in){
	while(isspace(*_in)){
		_in++;
	}
	return isNumberString(_in);
}
static void processEscaped(char* _in){
	int _cachedLen = strlen(_in);
	for (int i=0;i<_cachedLen;++i){
		if (_in[i]=='\\'){
			memmove(&(_in[i]),&(_in[i+1]),_cachedLen-i);
			if (_in[i]=='b'){
				_in[i]=0x08;
			}else if (_in[i]=='f'){
				_in[i]=0x0c;
			}else if (_in[i]=='n'){
				_in[i]='\n';
			}else if (_in[i]=='r'){
				_in[i]='\r';
			}else if (_in[i]=='t'){
				_in[i]='\t';
			} // there's also \\ and \"
			--_cachedLen;
		}
	}
}
void parseJson(const char* _jsonFilename, struct jsonTarget* _targets, int _numTargets, int _structSize, void*** _retArr, int* _retLen){
	crossFile* fp = crossfopen(_jsonFilename,"rb");
	void** _retList=NULL;
	int _retListSize=0;
	char _inObj=0; // are we in an object
	while(1){
		char* _curLine = easygetline(fp);
		if (_curLine==NULL || strlen(_curLine)==0){
			break;
		}
		char* _quotePos = strchr(_curLine,'"');
		if (_quotePos==NULL){
			if (strchr(_curLine,'{')!=NULL){
				_inObj=1;
				_retListSize++;
				_retList=realloc(_retList,sizeof(void*)*_retListSize);
				_retList[_retListSize-1]=malloc(_structSize);
			}else if (strchr(_curLine,'}')!=NULL){
				if (!_inObj){
					fprintf(stderr,"ending object while in one\n");
				}
				_inObj=0;
			}
			goto next;
		}
		if (!_inObj){
			fprintf(stderr,"thingie found while not in object\n");
		}		
		char* _otherQuote = strchr(_quotePos+1,'"');
		if (_otherQuote==NULL){
			fprintf(stderr,"error in line %s\n",_curLine);
			goto next;
		}
		*_otherQuote='\0';
		
		void* _dest=NULL;		
		char* _elementName = _quotePos+1;
		for (int i=0;i<_numTargets;++i){
			if (strcmp(_targets[i].name,_elementName)==0){
				_dest=(char*)_retList[_retListSize-1]+_targets[i].off;
				break;
			}
		}
		if (!_dest){
			fprintf(stderr,"unknown name %s\n",_elementName);
			goto next;
		}

		char* _valueStartPos = &_elementName[strlen(_elementName)+2]; // skip the :
		if (*_valueStartPos=='"'){
			char* _endPos = strrchr(_valueStartPos,'"');
			if (_endPos==NULL){
				fprintf(stderr,"bad end!\n");
				goto next;
			}
			*_endPos='\0';
			_valueStartPos++;
			char* _newString = strdup(_valueStartPos);
			processEscaped(_newString);
			memcpy(_dest,&_newString,sizeof(char*));
		}else if (*_valueStartPos=='['){
			int* _destArr=malloc(sizeof(int)); // stores the size as the first element
			int _destArrSize=1;
			char* _numStart=_valueStartPos;
			do{
				int _val = atoi(_numStart+1);
				if (_val==0 && !isValidAtoiString(_numStart+1)){
					break;
				}
				_destArr=realloc(_destArr,sizeof(int)*(++_destArrSize));;
				_destArr[_destArrSize-1]=_val;
				_numStart=strchr(_numStart+1,',');
			}while(_numStart!=NULL);
			_destArr[0]=_destArrSize-1;
			memcpy(_dest,&_destArr,sizeof(int*));
		}else{
			// assume it's a number
			int _val = atoi(_valueStartPos);
			memcpy(_dest,&_val,sizeof(int));
		}
	next:
		free(_curLine);
	}
	crossfclose(fp);
	*_retArr=_retList;
	*_retLen=_retListSize;
}
