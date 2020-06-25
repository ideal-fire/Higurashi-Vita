// insensitive, just like me
#include <goodbrew/config.h>
#if GBPLAT == GB_LINUX
// only for gnu/linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
static int numKnownDirs=0;
struct scannedDir* knownDirs=NULL;
struct scannedDir{
	char* path; // does not end with slash
	char** names;
	int numNames;
};
static int compareStringsInsensitive(const void* first, const void* second){
	return strcasecmp(*((char**)first),*((char**)second));
}
// returns index if it's there.
// returns the index it should be at, plus one, times negative one if not.
static int searchKnownDirs(char* _target){
	int _leftBound=0;
	int _rightBound=numKnownDirs; // exclusive
	int _cmpRes=0;
	int i=0;
	while(_rightBound>_leftBound){
		i = (_leftBound+_rightBound)/2;
		_cmpRes = strcmp(_target,knownDirs[i].path);
		if (_cmpRes==0){
			return i;
		}else if (_cmpRes>0){
			_leftBound=i+1;
		}else if (_cmpRes<0){
			_rightBound=i;
		}
	}
	return ((_cmpRes>0 ? (i+1) : i)+1)*-1;
}
static void* maybeExpandArray(void* _arrStart, int* _curSize, int _curUsed, int _elemSize){
	if (_curUsed<*_curSize){
		return _arrStart;
	}
	if (*_curSize==0){
		*_curSize=1;
	}
	(*_curSize)=(*_curSize)*2;
	void* _ret = malloc(_elemSize*(*_curSize));
	memcpy(_ret,_arrStart,_elemSize*_curUsed);
	free(_arrStart);
	return _ret;
}
static char scanNewDir(const char* _path, int _destIndex){
	DIR* _curDir = opendir(_path);
	if (!_curDir){
		perror(NULL);
		return 1;
	}
	char** _nameList = NULL;
	int _nameListSize=0;
	int _nameListUsed=0;
	while(1){
		errno=0;
		struct dirent* _curEntry;
		if (!(_curEntry=readdir(_curDir))){
			if (errno){
				perror("readdir");
				exit(1);
			}
			break;
		}
		_nameList = maybeExpandArray(_nameList,&_nameListSize,_nameListUsed,sizeof(char*));
		_nameList[_nameListUsed++]=strdup(_curEntry->d_name);
	}
	if (closedir(_curDir)){
		perror(NULL);
	}
	qsort(_nameList,_nameListUsed,sizeof(char*),compareStringsInsensitive);
	{
		numKnownDirs++;
		struct scannedDir* _newKnownDirs = malloc(sizeof(struct scannedDir)*numKnownDirs);
		memcpy(_newKnownDirs,knownDirs,_destIndex*sizeof(struct scannedDir));
		_newKnownDirs[_destIndex].path=strdup(_path);
		_newKnownDirs[_destIndex].names=_nameList;
		_newKnownDirs[_destIndex].numNames=_nameListUsed;
		memcpy(&(_newKnownDirs[_destIndex+1]),&(knownDirs[_destIndex]),sizeof(struct scannedDir)*(numKnownDirs-_destIndex-1));
		free(knownDirs);
		knownDirs=_newKnownDirs;
	}
	return 0;
}
// if the file exists, return its path malloc
char* insensitiveFileExists(const char* _path){
	char* _ret=NULL;
	char* _workablePath = strdup(_path);
	// strip it to just the directory
	int i;
	for (i=strlen(_workablePath)-1;i>0;--i){
		if (_workablePath[i]=='/'){
			_workablePath[i]='\0';
			break;
		}
	}
	if (i==0){
		fprintf(stderr,"bad path passed %s\n",_path);
		goto err;
	}
	// search this directory if we haven't
	int _dirIndex = searchKnownDirs(_workablePath);
	if (_dirIndex<0){
		_dirIndex=(_dirIndex*-1)-1;
		if (scanNewDir(_workablePath,_dirIndex)){
			goto err;
		}
	}
	//
	char* _targetPtr=&_workablePath[strlen(_workablePath)+1];
	char** _foundName = bsearch(&_targetPtr,knownDirs[_dirIndex].names,knownDirs[_dirIndex].numNames,sizeof(char*),compareStringsInsensitive);
	if (_foundName!=NULL){
		int _dirLen = strlen(_workablePath);
		_workablePath[_dirLen]='/';
		memcpy(&_workablePath[_dirLen+1],*_foundName,strlen(*_foundName));
		_ret=_workablePath;
		_workablePath=NULL;
	}
err:
	free(_workablePath);
	return _ret;
}
#else
char* insensitiveFileExists(const char* _filename){
	if (checkFileExists(_filename)){
		return strdup(_filename);
	}else{
		return NULL;
	}
}
#endif
