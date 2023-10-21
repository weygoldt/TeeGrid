/*
  SetupPCM - Setup PCM186x chips for specific channels of the 
  Teensy_Amp 4.x PCBs.
  Created by Jan Benda, Octoner 21th, 2023.
*/

#ifndef SetupPCM_h
#define SetupPCM_h


bool R40SetupPCM(InputTDM &tdm, ControlPCM186x &cpcm, bool offs, float pregain);

bool R4SetupPCM(InputTDM &tdm, ControlPCM186x &cpcm, bool offs);


#endif
