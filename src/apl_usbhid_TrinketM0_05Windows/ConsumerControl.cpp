
#include "ConsumerControl.h"

static const uint8_t _hidReportDescriptor[] PROGMEM = {
  0x05, 0x0c, // USAGE_PAGE (Consumer)
  0x09, 0x01, // USAGE (Consumer Control)
  0xa1, 0x01, // COLLECTION (Application)
  0x85, REPORT_ID, // REPORT_ID (X)
  0x15, 0x00, // LOGICAL_MINIMUM (0)
  0x25, 0x01, // LOGICAL_MAXIMUM (1)
  0x09, 0xe9, // USAGE (Volume Increment)
  0x09, 0xea, // USAGE (Volume Decrement)
  0x09, 0xe2, // USAGE (Mute)
  0x75, 0x01, // REPORT_SIZE (1)
  0x95, 0x03, // REPORT_COUNT (3)
  0x81, 0x02, // INPUT (Data,Var,Abs)
  0x75, 0x01, // REPORT_SIZE (1)
  0x95, 0x05, // REPORT_COUNT (5)
  0x81, 0x03, // INPUT (Const,Var,Abs)
  0xC0 //   END_COLLECTION
};

ConsumerControl_::ConsumerControl_(void)
{
  static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
  HID().AppendDescriptor(&node);
}

void ConsumerControl_::press(u8 buttonBit)
{
  u8 m[1];
  m[0]=(1 << buttonBit);
  HID().SendReport(REPORT_ID, m, sizeof(m));
}

void ConsumerControl_::release()
{
  u8 m[1];
  m[0]=0;
  HID().SendReport(REPORT_ID, m, sizeof(m));
}

ConsumerControl_ ConsumerControl;
