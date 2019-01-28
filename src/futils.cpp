#include "futils.h"



void ReadFile(fs::FS &fs , const char* path)
{
    File file=fs.open(path);
    if (!file||file.isDirectory())
    {
        Serial.println("failed to read");
        return;
    }
    while (file.available())
    {
        Serial.write(file.read());
    }
}

unsigned long ReadFileW(fs::FS &fs,const char* path,char* buff)
{
    File file=fs.open(path);
    if (!file||file.isDirectory())
    {
        Serial.println("failed to read");
        return 0;
    }
    file.readBytes(buff,file.size());
    return file.size();
}

void WriteValue(fs::FS &fs,String value,char* timestamp,const char* path)
{
    File file=fs.open(path,FILE_APPEND);
    if (!file)
    {
        file=fs.open(path,FILE_WRITE);
        return;
    }
    //std::ostringstream strs;
    //strs<<value;
    //std:String str=strs.str;
    file.print(value);
    file.print(" ");
    file.print(timestamp);
    file.print("\n");
}
void WriteSetpoint1(fs::FS &fs,String value,const char* path)
{
    File file=fs.open(path,FILE_WRITE);
    if (!file)
    {
        file=fs.open(path,FILE_WRITE);
        return;
    }
    //std::ostringstream strs;
    //strs<<value;
    //std:String str=strs.str;
    file.print(value);
    file.print("\n");
}

void deleteLOG(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

int GetLogSize(fs::FS &fs,const char * path)
{
    File file=fs.open(path);
    int sz=(int)file.size();
    file.close();
    return sz;
}

void ReadFileByte(fs::FS &fs,const char* path,char* buff)
{
    File file=fs.open(path);
    if (!file||file.isDirectory())
    {
        Serial.println("failed to read");
    }
    file.readBytes(buff,1);
}