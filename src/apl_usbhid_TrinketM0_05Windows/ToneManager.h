/*
 * tonemanager.h  v1.0
 */
#ifndef _TONEMANAGER_H_
#define _TONEMANAGER_H_
#include <Arduino.h>
//機能的に補足説明
//データ計算時に、設定されていないbitはOFFとして計算される。エラーではない。

class ToneManager {
#define TONE_DEFAULT_LEN 8
#define TONE_DEFAULT_THRESH 50
#define TONE_VALCOUNT 8
  uint8_t toneDevicePin;        //pin番号 
  uint8_t thresholdLevel;       //閾値
  uint8_t bitLen;               //データ長。最大8
  boolean vals[TONE_VALCOUNT];  //bit受信データ保管

public:
  int begin(uint8_t tonePin , uint8_t len);
  void clear();
  boolean readBit(uint8_t n);
  boolean judgeTone(void);
  int getToneVal(void);
   
private:
  uint16_t getToneLevel(void);
};
#endif
// END OF FILE
