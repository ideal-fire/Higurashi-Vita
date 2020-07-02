// numbers: int
// strings: char*
// arrays: int*
struct jsonTarget{
	char* name;
	int off; // offset relative to the start of the struct to shove this value in
};
void parseJson(const char* _jsonFilename, struct jsonTarget* _targets, int _numTargets, int _structSize, void*** _retArr, int* _retLen);
