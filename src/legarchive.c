#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct{
	char* filename;
	int64_t position;
	int32_t length;
}legArchiveEntry;
typedef struct{
	int32_t totalFiles;
	char* filename;
	legArchiveEntry* fileList;
}legArchive;
typedef struct{
	FILE* fp;
	int internalPosition; // Relative to the start of this file
	int totalLength;
	char inArchive;

	// These next two are for reloading the file.
	char* filename; // Filename of the archive, malloc'd
	int64_t startPosition; // Start position in the archive
}legArchiveFile;

char* strstrGood(char* _big, char* _small, int _length){
	int _cachedStrlen = strlen(_small);
	char* _endPointer = _big+_length-_cachedStrlen+1;
	for (;_big!=_endPointer;++_big){
		int i;
		for (i=0;i<_cachedStrlen;++i){
			if (_big[i]==_small[i]){
				if (i==_cachedStrlen-1){
					return _big;
				}
			}else{
				break;
			}
		}
	}
	return NULL;
}

signed char searchForString(FILE* fp, char* _searchTerm){
	char _lastReadData[512];
	int _cachedStrlen = strlen(_searchTerm);
	char _isFirst=1;
	while (1){
		int _totalRead;
		if (_isFirst){
			_isFirst=0;
			_totalRead = fread(_lastReadData,1,512,fp);
		}else{
			memcpy(_lastReadData,&(_lastReadData[512-_cachedStrlen]),_cachedStrlen);
			_totalRead = fread(&(_lastReadData[_cachedStrlen]),1,512-_cachedStrlen,fp);
			_totalRead+=_cachedStrlen;
		}
		char* _foundString = strstrGood(_lastReadData,_searchTerm,512);
		if (_foundString!=NULL){
			// Seek to where we found the string
			fseek(fp,-1*(_totalRead-(long)(_foundString-&(_lastReadData[0]))),SEEK_CUR);
			return 0;
		}
		if (_totalRead!=512){
			return 1;
		}
	}
}

legArchiveFile getAdvancedFile(legArchive _passedArchive, const char* _passedFilename){
	int32_t i;

	// Make the passed string lowercase because the file list is stored lowercase
	int _cachedStrlen = strlen(_passedFilename);
	char _lowerString[_cachedStrlen+1];
	_lowerString[_cachedStrlen]='\0';
	for (i=0;i<_cachedStrlen;++i){
		_lowerString[i]=tolower(_passedFilename[i]);
	}

	legArchiveFile _returnFile;
	_returnFile.fp=NULL;
	for (i=0;i<_passedArchive.totalFiles;++i){
		if (strcmp(_passedArchive.fileList[i].filename,_lowerString)==0){

			_returnFile.fp = fopen(_passedArchive.filename,"r");
			fseek(_returnFile.fp,_passedArchive.fileList[i].position,SEEK_SET);
			_returnFile.internalPosition=0;
			_returnFile.totalLength = _passedArchive.fileList[i].length;
			_returnFile.startPosition = _passedArchive.fileList[i].position;
			_returnFile.inArchive=1;
			_returnFile.filename = malloc(strlen(_passedArchive.filename)+1);
				strcpy(_returnFile.filename,_passedArchive.filename);
			return _returnFile;
		}
	}
	printf("File not found.\n");
	return _returnFile;
}
FILE* openArchiveFile(legArchive _passedArchive, const char* _passedFilename){
	return getAdvancedFile(_passedArchive,_passedFilename).fp;
}
char* readLowerNullString(FILE* fp){
	char _currentRead[261];
	int i;
	for (i=0;;++i){
		int _lastReadChar=fgetc(fp);
		if (_lastReadChar==0 || _lastReadChar==EOF){
			_currentRead[i]='\0';
			break;
		}
		_currentRead[i] = tolower(_lastReadChar);
	}
	char* _returnString = malloc(strlen(_currentRead)+1);
	strcpy(_returnString,_currentRead);
	return _returnString;
}
legArchive loadLegArchive(const char* _filename){
	legArchive _returnArchive;
	FILE* fp = fopen(_filename,"rb");

	fseek(fp,-8,SEEK_END);
	int64_t _tableStartPosition;
	fread(&_tableStartPosition,8,1,fp);
	fseek(fp,_tableStartPosition,SEEK_SET);

	char _foundMagic[11];
	_foundMagic[10]='\0';
	fread(_foundMagic,1,10,fp);
	if (strcmp(_foundMagic,"LEGARCHTBL")!=0){
		printf("bad archive.");
		memset(&_returnArchive,0,sizeof(_returnArchive));
	}else{
		_returnArchive.filename = malloc(strlen(_filename)+1);
		strcpy(_returnArchive.filename,_filename);

		int32_t _readTotalFiles;
		fread(&_readTotalFiles,4,1,fp);

		_returnArchive.fileList = malloc(sizeof(legArchiveEntry)*_readTotalFiles);
		_returnArchive.totalFiles = _readTotalFiles;

		int i;
		for (i=0;i<_readTotalFiles;++i){
			_returnArchive.fileList[i].filename = readLowerNullString(fp);
			fread(&(_returnArchive.fileList[i].position),8,1,fp);
			fread(&(_returnArchive.fileList[i].length),4,1,fp);
		}

	}
	fclose(fp);
	return _returnArchive;
}
void freeArchive(legArchive _toFree){
	int i;
	for (i=0;i<_toFree.totalFiles;++i){
		free(_toFree.fileList[i].filename);
	}
	free(_toFree.fileList);
	free(_toFree.filename);
}
