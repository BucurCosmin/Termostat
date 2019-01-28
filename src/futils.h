#include "FS.h"
#include "SPIFFS.h"

void ReadFile(fs::FS &fs , const char* path);
void WriteValue(fs::FS &fs,String value,char* timestamp,const char* path);
void WriteSetpoint1(fs::FS &fs,String value,const char* path);
unsigned long ReadFileW(fs::FS &fs,const char* path,char* buff);
void deleteLOG(fs::FS &fs, const char * path);
int GetLogSize(fs::FS &fs,const char * path);
void ReadFileByte(fs::FS &fs,const char* path,char* buff);