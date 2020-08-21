/*
 *volumeControl.h v1.0
 */
#ifndef _VOLUMECONTROL_H_
#define _VOLUMECONTROL_H_
#include "HID.h"
#include "ConsumerControl.h"

#define u8 unsigned char

#define REPORT_ID 1
#define VOLUME_UP 0
#define VOLUME_DOWN 1
#define VOLUME_MUTE 2

class VolumeControl
{
public:
  VolumeControl(void);
  void begin(void);
  void volumeUP(uint8_t);
  void volumeDOWN(void);
};

//extern VolumeControl_ VolumeControl;
#endif
