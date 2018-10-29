#ifndef UTILHHAVESBEENINCLUDED
#define UTILHHAVESBEENINCLUDED

char* formatf(va_list _startedList, const char* _stringFormat);
char* easySprintf( const char* _stringFormat, ... );
void seekPast(FILE* fp, unsigned char _target);

#endif