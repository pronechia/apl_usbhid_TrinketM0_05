/*
 * VolumeControl.cpp v1.0
 */
#include "HID.h"
#include "VolumeControl.h"

#define CONTROL_DELAY_DEVICE 500  //USBデバイス状態変更後の待ち時間。
#define CONTROL_DELAY_TIME 150

//
VolumeControl::VolumeControl(void)
{
  //ConsumerControlをコンストラクトするときに必要な処理を行っている。
  //ここでは何もしない
}
//
void VolumeControl::begin() {
  //他のHIDデバイスとの共存では必要になる可能性あり
  //USBDevice.detach();  //USBAPI.hに定義されている関数
  //delay(CONTROL_DELAY_DEVICE);
  //USBDevice.init();  //USBAPI.hに定義されている関数
  //USBDevice.attach();  //USBAPI.hに定義されている関数
  //delay(CONTROL_DELAY_DEVICE);
  //HID().begin();
}
//
// 音量をほぼ最高になるまでUPする。
//
void VolumeControl::volumeUP(uint8_t pOS) {
  int wcount;
  
  if (pOS == 1 ) wcount = 24;//WinPCでは、UPの刻みが小さいので、多く回す
  else if (pOS == 2 ) wcount = 10;
  else wcount = 0;  //本来エラーなので何もしない
  for (int j=0; j<wcount; j++) {
    ConsumerControl.press(VOLUME_UP); // 音量大ボタンを押す
    ConsumerControl.release(); // ボタンを離す
    delay(CONTROL_DELAY_TIME);
  }
}
//一度だけ
void VolumeControl::volumeDOWN() {
  
    ConsumerControl.press(VOLUME_DOWN); // 音量小ボタンを押す
    ConsumerControl.release(); // ボタンを離す
    delay(CONTROL_DELAY_TIME);
}
//一度だけ
void sub_Control_mute() {
  
    ConsumerControl.press(VOLUME_MUTE);
    ConsumerControl.release(); // ボタンを離す
    delay(CONTROL_DELAY_TIME);
}
