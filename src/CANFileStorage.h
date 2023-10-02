/*
  CANFileStorage - High level handling of CAN synchronized file storage of logger data.
  Created by Jan Benda, September 18th, 2023.
*/

#ifndef CANFileStorage_h
#define CANFileStorage_h


void setupGridStorage(const char *path, const char *software,
		      Input &aidata, char *gainstr=0);
void openNextGridFile();
void storeGridData(bool master=false);


#endif

