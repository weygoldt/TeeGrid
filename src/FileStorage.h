/*
  FileStorage - High level handling of file storage of logger data.
  Created by Jan Benda, August 28th, 2023.
*/

#ifndef FileStorage_h
#define FileStorage_h


void setupStorage(const char *software, Input &aidata, char *gainstr=0);
void openNextFile();
void openBackupFile();
void storeData();


#endif

