#include "HID.h"
//nkano add
#define u8 unsigned char

#define REPORT_ID 1
#define VOLUME_UP 0
#define VOLUME_DOWN 1
#define VOLUME_MUTE 2

class ConsumerControl_
{
public:
  ConsumerControl_(void);
  void press(u8 buttonBit);
  void release(void);
};

extern ConsumerControl_ ConsumerControl;
