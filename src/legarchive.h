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
	char inArchive;

	// These next two are for reloading the file.
	char* filename; // Filename of the archive, malloc'd
	int64_t startPosition; // Start position in the archive
}legArchiveFile;

FILE* openArchiveFile(legArchive _passedArchive, const char* _passedFilename);
legArchive loadLegArchive(const char* _filename);
void freeArchive(legArchive _toFree);
legArchiveFile getAdvancedFile(legArchive _passedArchive, const char* _passedFilename);

#endif
