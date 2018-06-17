#ifndef THELEGARCHIVEHEADERFILEHASBEENINCLUDED
#define THELEGARCHIVEHEADERFILEHASBEENINCLUDED 

#include <stdint.h>
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
}legArchiveFile;

FILE* openArchiveFile(legArchive _passedArchive, const char* _passedFilename);
legArchive loadLegArchive(const char* _filename);
void freeArchive(legArchive _toFree);
legArchiveFile getAdvancedFile(legArchive _passedArchive, const char* _passedFilename);

#endif