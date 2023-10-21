#include <SetupPCM.h>


bool R40setupPCM(InputTDM &tdm, ControlPCM186x &cpcm,
		 bool offs, float pregain) {
  cpcm.begin();
  bool r = cpcm.setMicBias(false, true);
  if (!r) {
    Serial.println("not available");
    return false;
  }
  cpcm.setRate(tdm, aisettings.rate());
  if (tdm.nchannels() < aisettings.nchannels()) {
    if (aisettings.nchannels() - tdm.nchannels() == 2) {
      if (pregain == 1.0) {
        cpcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
	              offs, ControlPCM186x::INVERTED);
        Serial.println("configured for 2 channels without preamplifier");
      }
      else {
        cpcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
	              offs, ControlPCM186x::INVERTED);
        Serial.printf("configured for 2 channels with preamplifier x%.0f\n", pregain);
      }
    }
    else {
      if (pregain == 1.0) {
        cpcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                      ControlPCM186x::CH4L, ControlPCM186x::CH4R,
		      offs, ControlPCM186x::INVERTED);
        Serial.println("configured for 4 channels without preamplifier");
      }
      else {
        cpcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
                      ControlPCM186x::CH2L, ControlPCM186x::CH2R,
		      offs, ControlPCM186x::INVERTED);
        Serial.printf("configured for 4 channels with preamplifier x%.0f\n", pregain);
      }
    }
    cpcm.setSmoothGainChange(false);
    cpcm.setGain(aisettings.gain());
    cpcm.setFilters(ControlPCM186x::FIR, false);
    pcm = &cpcm;
  }
  else {
    // channels not recorded, but need to be configured to not corupt TDM bus:
    cpcm.setupTDM(ControlPCM186x::CH1L, ControlPCM186x::CH1R, offs);
    cpcm.powerdown();
    Serial.println("powered down");
  }
  return true;
}


bool R4setupPCM(InputTDM &tdm, ControlPCM186x &cpcm, bool offs) {
  cpcm.begin();
  bool r = cpcm.setMicBias(false, true);
  if (!r) {
    Serial.println("not available");
    return false;
  }
  cpcm.setRate(tdm, aisettings.rate());
  if (tdm.nchannels() < NCHANNELS) {
    if (NCHANNELS - tdm.nchannels() == 2) {
      cpcm.setupTDM(tdm, ControlPCM186x::CH2L, ControlPCM186x::CH2R,
                    offs, ControlPCM186x::INVERTED);
      Serial.println("configured for 2 channels");
    }
    else {
      cpcm.setupTDM(tdm, ControlPCM186x::CH2L, ControlPCM186x::CH2R,
                    ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                    offs, ControlPCM186x::INVERTED);
      Serial.println("configured for 4 channels");
    }
    cpcm.setSmoothGainChange(false);
    cpcm.setGain(aisettings.gain());
    cpcm.setFilters(ControlPCM186x::FIR, false);
    pcm = &cpcm;
  }
  else {
    // channels not recorded, but need to be configured to not corupt TDM bus:
    cpcm.setupTDM(ControlPCM186x::CH2L, ControlPCM186x::CH2R, offs);
    cpcm.powerdown();
    Serial.println("powered down");
  }
  return true;
}


