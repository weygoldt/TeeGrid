/*
  Banner - ASCII art banner for serial output.
  Created by Jan Benda, August 16th, 2024.
*/

#ifndef Banner_h
#define Banner_h


#include <Arduino.h>


void printBanner(const char *software=NULL, Stream &stream=Serial);


#endif

