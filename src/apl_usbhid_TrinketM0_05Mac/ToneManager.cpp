/*
 * tonemanager.cpp v1.0
 */
#include "ToneManager.h"
//
//public methods ------------------------------
//pin番号保持、閾値を決める、clearは別関数とした
int ToneManager::begin(uint8_t tonePin , uint8_t len) {
  toneDevicePin =tonePin;
  if ((len < 1) || (len > TONE_DEFAULT_LEN)) bitLen = TONE_DEFAULT_LEN;
  else bitLen = len;
  thresholdLevel = TONE_DEFAULT_THRESH;
  return 0;
}
//bit保持領域をOFFにする。最大８bit
void ToneManager::clear() {
  for (int j=0; j<TONE_VALCOUNT; j++) vals[j] = false;
}
// n= 1 to 8
boolean ToneManager::readBit(uint8_t n) {
  uint8_t j;
  
  //音のON/OFFからbitを判定しn番目にセット
  if (n < 1 || n > bitLen ) return false;
  //
  //5msec x 40=200msec loop
  boolean w_found = false;
  for (j=0; j<40; j++) {
    if (getToneLevel() > thresholdLevel) {
      //ONなら抜ける
      w_found = true;
      break;
    }
    delay(5); 
  }
  //loopを回り切ったらタイムオーバーとして抜ける
  if (!w_found) return false;
  //30msec delay 後の音でON/OFFが決まる
  delay(30);
  //正確性を上げるために、二回loop。一度でもONならONと判定する。
  w_found = false;
  for (j=0; j<2; j++) {
    if (getToneLevel() > thresholdLevel) w_found = true;
    delay(1);
  }
  //一度でもONならONと判定する
  if (w_found) vals[n-1] = true;
  else vals[n-1] = false;
  return true;
}
//
//判定　analog値を読んで閾値以上ならON,その他はOFF
// rc 1:ON 0:OFF
boolean ToneManager::judgeTone() {  
  if (getToneLevel() > thresholdLevel) return true;
  return false;
}
//
int ToneManager::getToneVal() {
  //厳密にはbitLenまでだが、それ以外は必ずfalseなので８ビット回す 
  //注意）受信は下位ビットから来る
  int w_valL = vals[3] * 8 + vals[2] * 4 + vals[1] * 2 + vals[0];
  int w_valH = vals[7] * 8 + vals[6] * 4 + vals[5] * 2 + vals[4];
  int w_val  = w_valH * 16 + w_valL;
  return w_val;
}
//
// private methods ------------------------------
//
uint16_t ToneManager::getToneLevel() {
  //analog値をそのまま返す。DEBUG用
  return analogRead(toneDevicePin);  
}
