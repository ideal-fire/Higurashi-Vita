void setFragPlayed(int _id);
void parseFragmentFile(const char* _filename);
void fragmentMenu();
void saveHiguGame();
struct fragmentJson;
extern struct fragmentJson** fragmentInfo;
extern int* fragPlayOrder;
extern int playedFrags;
