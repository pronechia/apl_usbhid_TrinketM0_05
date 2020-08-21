/*
 *公開第五回）赤外線によるエアコンのON/OFFと温度調整制御 2020/08/05
 */
// WindowsPC用
#define USBHOST_WINPC
//#define USBHOST_MAC

//環境に合わせて内容を確認または修正すること　ーーここから -----------------------------
//①BME280 I2Cアドレスの設定　（0x76 or 0x77 のどちらか）
//  ブレッドボードは 0x76, MBE280のグローブ端子接続は 0x76
#define BME280DEVADDR 0x76
//#define BME280DEVADDR 0x77
//②Webserverに合わせて、ＵＲＬを記述すること
//String g_url_string = "http'//192.168.xxx.xxx/xxxxxxxx.htm"; //内部WebServer用
String g_url_string = "http'//localhost/example0501.htm"; //localhost用
//③ハード設定
#define LED_PIN 13
#define SW_PIN  1
#define IR_IN   3
#define IR_OUT  4
#define TONE_PIN  3
//#define TONE_PIN  4
//環境に合わせて内容を確認または修正すること　ーーここまで -----------------------------

#define WCS_DELAY_T1 150 //T1 ブラウザー応答時間150
#define WCS_DELAY_GAMEN 3000  //URLたたいてから画面が表示されるまで待つ
#define WCS_BITREAD_MAXCOUNT 5  //bit read のリトライ回数上限
#define WCS_SWWAIT_MAXCOUNT 30  //sw押下待ちのリトライ回数上限 30秒
#include "types.h"
#include <Wire.h>
#include <ZeroTimer.h>
#include <SparkFunBME280.h>
#include "lib_keyboard.h"
#include "ToneManager.h"
#include "VolumeControl.h"

BME280 mySensor;
ToneManager myToneM;
VolumeControl myVolumeC;

int16_t g_pass;  //HID出力したら後の待ち時間を制御する
boolean g_I2CNormal;
volatile int g_i; //timerカウント
volatile boolean g_high_spead;
int g_dataSyubetu = 1; //1-4。初期値は温度
int g_retry_count;
int g_mode;
//
//
//
void setup(){  
  g_pass = 1;
  g_i = 0;
  g_high_spead = 0;
  g_retry_count = 0;
  pinMode(LED_PIN,OUTPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  delay(5000);
  //LEDが点滅している間にSWを押せば学習モードに入る
  sub_fw_Blink(LED_PIN, 10, 50); //10回ｘ50msecON/OFF
  //SW=ONの時、学習モードに入る。その後、LEDが消灯していれば学習モードと分かる
  if (!sub_fw_isSWON(SW_PIN)) {
    pinMode(IR_IN, INPUT);
    pinMode(IR_OUT, OUTPUT);
    Serial.begin(115200);
    delay(200);
    Serial.println("sub_fw_isSWON ON LearningMOde");
    //定時間隔で無限ループ
    while(1) sub_learningMode(); 
    //定時間隔で無限ループ
  }
  //
  //ここからは制御モード
  // LEDが数回、高速点滅後、低速で点滅してる間にSWを押すと先に進む
  Wire.begin();
  delay(100); //I2Cデバイスの初期化待ち
  mySensor.setI2CAddress(BME280DEVADDR);
  //BME280の初期化ができない場合、値ゼロで動かす
  if (mySensor.beginI2C() == false) g_I2CNormal = false;
  else                              g_I2CNormal = true;
  pinMode(TONE_PIN, INPUT);
  myToneM.begin(TONE_PIN, 8);  //8bit
  sub_fw_Blink(LED_PIN, 3, 50); //動き始めたことを知らせる
  //
  //PC側の準備ができるまで、USB/HIDを出力しない
  //SWが押されるまで待つ 10msec x 30 x 100 = 30秒 でエラーとする
  boolean sw_pushed = false;
  for (int j = 0; j< (WCS_SWWAIT_MAXCOUNT * 100); j++) {  
    if (sub_fw_SWcheck(SW_PIN) == 1) {
      sw_pushed = true;
      break;
    }
    if ((j % 100) == 0) {
      sub_fw_Blink(LED_PIN, 1, 5);  //5msecx2=10msec
    } else {
      delay(10);
    }
  }
  if (sw_pushed == false )
    while(1) sub_fw_Blink(LED_PIN, 10, 50);  //LED点滅し続ける
  
  // 音量UPのため、HIDデバイスを定義する
  myVolumeC.begin();
#if defined USBHOST_WINPC
  myVolumeC.volumeUP(1);  //windows
#elif defined USBHOST_MAC
  myVolumeC.volumeUP(2);  //MAC
#else
  // 何もしない。後で音が拾えない可能性がある
#endif

  //
  delay(1000);  //USB/HID再定義のための待ち時間。
#if defined USBHOST_WINPC
  sub_kbd_begin(1);  //Windows用に初期化
#elif defined USBHOST_MAC
  sub_kbd_begin(2);  //Mac用に初期化
#else
  // 何もしない。後でKEYBOARDが動作しない可能性大
#endif
  delay(100); //HIDデバイスの初期化待ち
  //
  sub_fw_timerset();  //タイマー起動
}
//
//main loop
//
void loop(){
  // 40msec毎に処理を行う
  if (sub_fw_event(2)) sub_proc();
}
//
//USB/HID処理
//
void sub_proc() {
  static byte s_command = 0; //処理振り分け
  static byte s_first = 0;
  int j;
  int w_ch;
  
  //待ち時間がゼロになるまで何もしない。
  if (g_pass-- >= 0 ) return;
  //処理振り分け
  if (s_command == 0) { 
    s_command = 1;
    s_first = 1;
    sub_initurl(); //一回だけ実施
  } else if (s_command == 1) { 
    sub_out_kbd(1);  //start
    if (sub_check_tone()) s_command = 10;
    else s_command = 90;
  } else if (s_command == 10) {
    sub_out_kbd(10); //Data
    if (sub_check_tone()) s_command = 11;
    else s_command = 90;
  } else if (s_command == 11) {
    sub_out_kbd(11); //センサー値UP
    if (sub_check_tone()) s_command = 20;
    else s_command = 90;
  } else if (s_command == 20) {
    sub_out_kbd(20); //Inqu
    if (sub_check_tone()) s_command = 21;
    else s_command = 90;
    g_retry_count = 0; //次回のために初期化  
    myToneM.clear();
  } else if ((s_command >= 21) && (s_command <= 28)) {
    //制御情報を1ビットつづ受けとる。８ビット
    sub_out_kbd(s_command); //Inq no1 to no8
    g_high_spead = 1; //問い合わせは待ち時間なしで動かす。本当はS_command==21のみでいい
    if (myToneM.readBit(s_command - 20)) {
      //正常
      s_command = s_command+1;
      g_retry_count = 0; //次回のために初期化  
      //8bitは無駄なので、4bitで次へ
      if (s_command == 25 ) s_command = 29; 
    } else {
      //異常時は同じビットを再送要求する
      if (g_retry_count++ >= WCS_BITREAD_MAXCOUNT) s_command = 90; //リトライオーバーで異常へ
    }
  } else if (s_command == 29) { 
    g_high_spead = 0; //問い合わせは待ち時間なしで動かすモードを終了する
    //制御情報を1ビットつづ受けとる。８ビット
    sub_out_kbd(29); //Inqe end
    g_dataSyubetu = myToneM.getToneVal(); //受信した制御値を取り出す
    //IR出力
    w_ch = 0;
    if (g_dataSyubetu == 3 ) w_ch = 1; //ON
    else if (g_dataSyubetu == 6 ) w_ch = 2; //OFF
    else if (g_dataSyubetu == 9 ) w_ch = 1; //ON
    else if (g_dataSyubetu == 10 ) w_ch = 2; //OFF
    if (w_ch == 1 || w_ch == 2 ) {
      sub_qCommand(w_ch);    
      //IR出力しことをLEDで知らせる
      sub_fw_Blink(LED_PIN, 10, 50); //10回ｘ50msecON/OFF 計1秒
    }
    s_command = 10; //次のセンサーデータUPへ、
  } else if (s_command == 90) {
    //フォーカスがおかしくて音が拾えない場合にここにくる
    sub_out_kbd(90); //フォーカスをコマンドフィールドへ
    s_first = 1;
    s_command = 1;   //startコマンドから再開する     
  } else {
    //その他はエラー
    sub_fw_Blink(LED_PIN, 10, 40);
  }
  
  if (s_command == 1) {
    g_pass = 50; //40msec*50= 2sec
  } else if (s_command == 10) {
    if (s_first == 1) {
      g_pass = 50; //40msec*50= 2sec
      s_first = 0;
    } else {
      g_pass = 200; //40msec*200= 8sec
    }
  } else if ((s_command > 10) && (s_command <= 20)) {
    g_pass = 25; //40msec*25= 1sec
  } else if ((s_command > 20) && (s_command < 30)) {
    g_pass = 1; //high speed
  } else if (s_command == 90) {
    g_pass = 25; //40msec*25= 1sec
  } else {
    //フォーカスおかしいときも、一旦ここにきて待つ
    g_pass = 200; //40msec*200= 8sec
  }
}
//
//周期によって、決まった文字列処理（主にHTML画面への文字列入力）をおこなう
//
void sub_out_kbd(int8_t p_ctl) {
  int32_t w_temp;
  int32_t w_temp2;
  char w_buf[16];
  char w_buf2[16];
  char w_buf3[16];

  if (p_ctl == 0) {
    delay(WCS_DELAY_T1 );
  } else if (p_ctl == 1) {
    sub_moji_tab("Start");
    sub_kbd_strok(KEY_RETURN);
  } else if (p_ctl == 10) {
    sub_moji_tab("Data");
    sub_kbd_strok(KEY_RETURN);
  } else if (p_ctl == 11) {
    //ラベルと値を出力する
    //第五弾　必ず温度を取る
    sub_out_val(1, w_buf ,w_buf2, w_buf3);
    //sub_moji_tab(w_buf);  //ラベル
    sub_moji_tab(w_buf2);  //値
    //sub_moji_tab(w_buf3);  //単位
    sub_kbd_strok(KEY_RETURN);
  } else if (p_ctl == 20) {
    //制御情報を問い合わせる
    sub_moji_tab("Inqu");
    sub_kbd_strok(KEY_RETURN);
  } else if ((p_ctl > 20) && (p_ctl < 29)) {
    w_temp = p_ctl - 20;
    sprintf(w_buf, "%d", w_temp);
    sub_moji_tab(w_buf);
    sub_kbd_strok(KEY_RETURN);
  } else if (p_ctl == 29) {
    sub_moji_tab("e");
    sub_kbd_strok(KEY_RETURN);
  } else if (p_ctl == 90) {
    //OSに依存せずURLを再入力する
    sub_initurl();
  } else {
    sub_moji_tab("error");
    sub_kbd_strok(KEY_RETURN);
  }
}
//
//50msec*5回繰り返す。音が拾えたらrc=trueとなる
boolean sub_check_tone(void) {
  int j;

  //はじめは音が鳴っていないことを確認する
  if (myToneM.judgeTone()) {
    // (50msec x 2(on/OFF) x 50回)= 5sec 
    sub_fw_Blink(LED_PIN, 50, 50);
    return false;
  }
  //
  for (j=0; j<5; j++) {
    delay(50);   
    if (myToneM.judgeTone()) return true;
  }
  //エラー検知
  // (50msec x 2(on/OFF) x 10回)= 1sec 
  sub_fw_Blink(LED_PIN, 10, 50);
  return false; //50msecX5=250msecでも音が鳴らないため、3秒点滅後、エラーを返す    
}
//
//アプリ画面を呼び出すURLを出力する
void sub_initurl() {

  //URL入力前に入力エリアにフォーカスを当てる
  sub_kbd_preURL();
  delay(WCS_DELAY_T1);
  //URL文字出力
  sub_kbd_print((char *)g_url_string.c_str());
  sub_kbd_strok(KEY_RETURN);
  delay(WCS_DELAY_GAMEN); //画面が表示されるまで、十分の時間待つ
  sub_kbd_strok(KEY_TAB);
}
//タイマー割込み関数
void sub_timer_callback() {
  // 指定した間隔(default 20msec)で割込みが入る
  g_i++;
}
//
//指示されたデータ種別の文字列を出力する
//
void sub_out_val(int p_datasyubetu, char* p_1 ,char* p_2, char* p_3) {
  char w_buf[16];
  int32_t w_temp;
  int32_t w_temp2;

  if (p_datasyubetu == 1) {
    strcpy(p_1, "Temprature");
    //センサー値を得る
    w_temp = (int32_t)(mySensor.readTempC() * 100); //xx.xx度をxxxxに変換
    if (w_temp > 9999) strcpy(w_buf, "100.00");
    else {
      w_temp2 = w_temp - (w_temp / 100 * 100);
      sprintf(w_buf, "%d.%02d", w_temp / 100, w_temp2);
    }
    strcpy(p_2, w_buf);
    strcpy(p_3, "C");
  } else if (p_datasyubetu == 2) {
    sprintf(p_1, "Humidity");
    //センサー値を得る
    w_temp = (int32_t)(mySensor.readFloatHumidity() * 100); //xx.xx%をxxxxに変換
    if (w_temp > 9999) strcpy(w_buf, "100.00");
    else {
      w_temp2 = w_temp - (w_temp / 100 * 100);
      sprintf(w_buf, "%d.%02d", w_temp / 100, w_temp2);
    }
    sprintf(p_2, "%s", w_buf);
    //NG sprintf(p_3, "\%");  //特殊文字
    strcpy(p_3, "%");
  } else if (p_datasyubetu == 3) {
    strcpy(p_1, "pressure");
    //センサー値を得る
    w_temp = (int32_t)((mySensor.readFloatPressure() + 50.0) / 100.0); //1013hPa
    if (w_temp > 1999) strcpy(w_buf, "2000");
    else sprintf(w_buf, "%4d", w_temp);
    strcpy(p_2, w_buf);
    strcpy(p_3, "hPa");

  } else if (p_datasyubetu == 4) {
    strcpy(p_1, "DI");
    //温度、湿度から不快指数を計算する
    float w_t;
    float w_h;
    float w_w;
    int w_out;
    //0.81T + 0.01H(0.99T - 14.3 ) + 46.3
    w_t = mySensor.readTempC();
    w_h = mySensor.readFloatHumidity();
    w_w = (float)0.99 * w_t - (float)14.3;
    w_w = (float)0.81 * w_t + (float)0.01 * w_h * w_w + (float)46.3;
    w_out = w_w + (float)0.5; //不快指数を整数で丸める      
    if (w_out > 100) strcpy(w_buf, "100");
    else sprintf(w_buf, "%d", w_out);
    strcpy(p_2, w_buf);
    strcpy(p_3, "");  //不快指数の単位はない
  } else {
    //その他は空白
    strcpy(p_1, "syubetu");
    w_temp = g_dataSyubetu;
    sprintf(w_buf, "%d", w_temp);
    strcpy(p_2, w_buf);
    strcpy(p_3, "ERROR");  //単位欄
  }
}
//
void sub_fw_timerset() {
  //20msec毎
  TC.startTimer(20000, sub_timer_callback);
}
//2カウント=約40msecでイベントを上げる
// rc true =event on
boolean sub_fw_event(uint8_t p_count) {
  boolean w_trriger;

  if (p_count > 100) return 0; //ERROR check  
  w_trriger = false;
  // g_i>50で約一秒。g_i>0で約20msec
  if (g_i >= p_count) {
    g_i = 0;
    w_trriger = true;
  }
  return w_trriger;
}
//
//瞬間のSW押下中か否かの判定
uint8_t sub_fw_isSWON(uint8_t p_swpin) {
  boolean w_sw;
  
  w_sw = digitalRead(p_swpin); // =0がSWを押したとき
  if (w_sw)  return ON;
  else return OFF;
}
//
//SWが押されたかのチェック。二度呼ばれて、連続して押されていたらONを返す。
// rc 0:OFF 1:立ち上がり 2:立下り
uint8_t sub_fw_SWcheck(uint8_t p_swpin) {
  static boolean s_pre_sw = 1; // =1はOFFを意味する
  static uint8_t s_first_sw = 0;
  boolean w_sw;

  w_sw = digitalRead(p_swpin); // =0がSWを押したとき
  //前回がOFF->ONの時、連続か？
  if (s_first_sw == 1) {
    s_first_sw = 0;
    s_pre_sw = w_sw;  //次回のために
    if (w_sw == 0) return 1; //ON->ON つまり連続して押された
    else return 0;           // ON->OFFだった。
  //前回がON->OFFの時、連続か？
  } else if (s_first_sw == 2) {
    s_first_sw = 0;
    s_pre_sw = w_sw;  //次回のために
    if (w_sw == 1) return 2; //OFF->OFF つまり連続して離れた
    else return 0;           // OFF->ONだった。
  }
  //OFFからONを検知する。
  if ((s_pre_sw == 1) && (w_sw == 0)) {
    s_first_sw = 1;
  }
  //ONからOFFを検知する。
  if ((s_pre_sw == 0) && (w_sw == 1)) {
    s_first_sw = 3;
  }
  s_pre_sw = w_sw;  //次回のために
  return 0;
}
//p_timesは最大255とする
void sub_fw_Blink(uint8_t p_ledpin, byte p_times , int p_wait){
  for (byte i=0; i< p_times; i++){
    digitalWrite(p_ledpin, HIGH);
    delay (p_wait);
    digitalWrite(p_ledpin, LOW); //消灯で終わる
    delay (p_wait);
  }
}
/*
 * 学習モード
 * 　定時間隔で動き続ける
 *   EEPROMのサイズを　1M(1024) -> 3M(3072)に拡張。chは２つ。
 */
#include "myFlashAsEEPROM.h"

void sub_learningMode() {
  u1 x;
  u1 w_ch;
  static u1 s_sw = OFF;
  
  if(Serial.available()){
    x=Serial.read();
    switch(x){
      case 's':
        sub_sCommand();
        break;
      case 'r':
        sub_rCommand();
        break;
      case 'p':
        //* p1 EEROM保管要求ch付。Serial Serila からデータを流して、EEPROMに保存。
        //* q1 EEPROM送信要求ch付き。EEPROMの内容を赤外線に出力。
        w_ch = sub_getChannel();
        if (w_ch == 0) {
          //エラー
          Serial.println("p command channel number ERROR!!");
          break;
        }
        sub_pCommand(w_ch);
        break;
      case 'q':
        w_ch = sub_getChannel();
        if (w_ch == 0) {
          //エラー
          Serial.println("q command channel number ERROR!!");
          break;
        }
        sub_qCommand(w_ch);
        break;
      default:
        break;
    }
  }
  //
  delay(10);
}
/*
 * 赤外線学習モード機能 
 * 
 * 機能分解
 * ・①赤外線受信 sub_getIR
 * ・④Serial 出力 sub_putSerial
 * ・②Serial　読む x2 sub_getSerial
 * ・⑤赤外線送信 x2 sub_putIR
 * ・⑥EEPROMに保存  sub_putEEPROM
 * ・③EEPROMから読む sub_getEEPROM
 * 
 * 組み合わせ
 * ・r①ー④　s②－⑤　p②－⑥　q③－⑤
 * 
 * グローバル変数をもって、必ずそこを経由する。今もそうしている。
 */
//

#define TIMEOUT_RECV_NOSIGNAL  (50000)
#define TIMEOUT_RECV           (5000000)
#define TIMEOUT_SEND           (2000000)

#define STATE_NONE             (-1)
#define STATE_OK               (0)
#define STATE_TIMEOUT          (1)
#define STATE_OVERFLOW         (2)
// 3M を 2CH で使う。1chは1536バイト -> １つのdataは２バイトつかうので、768個が最大
#define DATA_SIZE              (767)  //768 - 1(status用)
#define DATA_MAXLENGTH         (1536)  
//1チャンネルの文字列を保管しておく領域
u2 g_data[DATA_SIZE]; 
u2 g_dataCount; 
//
//
//
void sub_sCommand(){
  Serial.println("send command start");
  s1 state;

  state = sub_getSerial();
  Serial.printf("sub_getSerial: count : %d\n", g_dataCount);
  if (state == STATE_OK){
    sub_putIR();
    Serial.printf("sub_putIR: count : %d\n", g_dataCount);
  } else {
    Serial.print("NG:");
    Serial.println(state);
  }
}
void sub_rCommand(){
  s1 state;
  
  Serial.println("reseive command start");

  state = sub_getIR();
  Serial.printf("sub_getIR: count : %d\n", g_dataCount);
  if (state == STATE_OK){ 
    sub_putSerial();
    Serial.printf("sub_putSerial: count : %d\n", g_dataCount);
  } else {
    Serial.print("NG:");
    Serial.println(state);
  }
  
}
void sub_pCommand(int p_ch){
  s1 state;
  
  Serial.printf("p%d command start\n", p_ch);

  state = sub_getSerial();
  Serial.printf("sub_getSerial: count : %d\n", g_dataCount);
  if (state == STATE_OK){    
    sub_putEEPROM(p_ch);
  } else {
    Serial.print("NG:");
    Serial.println(state);
  }
}
void sub_qCommand(int p_ch){
  Serial.printf("q%d command start\n", p_ch);

  sub_getEEPROM(p_ch);
  sub_putIR();
  Serial.printf("sub_putIR: count : %d\n", g_dataCount);

}
//
//
//
u1 sub_getChannel() {
  u1 x;
  u1 w_ch;

  w_ch = 0;
  x = Serial.read();
  //ogg if(x>='1' && x<='4') {
  if(x>='1' && x<='2') {
        w_ch = x - '0';
  }
  return w_ch;
}
//
//
//①赤外線受信
s1 sub_getIR() {
  u1 pre_value = HIGH;
  u1 now_value = HIGH;
  u1 wait_flag = TRUE;
  s1 state = STATE_NONE;
  u4 pre_us = micros();
  u4 now_us = 0;
  u4 index = 0;
  
  while(state == STATE_NONE){
    now_value = digitalRead(IR_IN);
    if(pre_value != now_value){
      now_us = micros();
      if(!wait_flag){
        g_data[index++] = now_us - pre_us;
      }
      wait_flag = FALSE;
      pre_value = now_value;
      pre_us = now_us;
    }
    
    if(wait_flag){
      if((micros() - pre_us) > TIMEOUT_RECV){
        state = STATE_TIMEOUT;
        break;
      }
    } else {
      if((micros() - pre_us) > TIMEOUT_RECV_NOSIGNAL){
        state = STATE_OK;
        break;
      }
    }
    //IR通信ではデータが必要だが、EEPROMに保管できるサイズか判定
    if (index >= DATA_SIZE) {
      state = STATE_OVERFLOW;
      break;
    }
  }
  g_dataCount = index;
  return state;
}
//④Serial 出力 
void sub_putSerial() {
  u4 i = 0;

  Serial.print("s,");
  for(i = 0; i<g_dataCount; i++){
    Serial.print(g_data[i]);
    Serial.print(',');
  }
  Serial.println("0,");
  Serial.println("");
  /*
  //DEBUG
  Serial.printf("g_data0 : %d %x\n", g_data[0], g_data[0]);
  Serial.printf("g_data1 : %d %x\n", g_data[1], g_data[1]);
  Serial.printf("g_data2 : %d %x\n", g_data[2], g_data[2]);
  Serial.printf("receive data ch :  count : %d\n", g_dataCount);
  */
}
//②Serial　読む x2 
s1 sub_getSerial(){
  s1 state = STATE_NONE;
  u1 x;
  u4 tmp = 0;
  u4 index = 0;
  u4 us = 0;
  
  us = micros();
  
  while(state == STATE_NONE){
    if(Serial.available() == 0){
      if((micros() - us) > TIMEOUT_SEND){
        state = STATE_TIMEOUT;
        break;
      }
    } else {
      x = Serial.read();
      if(x>='0' && x<='9'){
        /* 数字を受信した場合 */
        tmp *= 10;
        tmp += x - '0';
      } else {
        /* 数字以外を受信した場合 */
        if((tmp == 0) && (index == 0)){
          /* 最初の一文字目は読み飛ばす */
        } else {
          g_data[index] = (u2)tmp;
          if(tmp == 0){
            state = STATE_OK;
            break;
          } else if(index >= DATA_SIZE){
            state = STATE_OVERFLOW;
            break;
          }
          index++;
        }
        tmp = 0;
      }
    }
  }

  g_dataCount = index;
  return state;
}
//⑤赤外線送信 x2 
void sub_putIR() {
  u2 time = 0;
  u4 count = 0;
  u4 us = 0;
  u4 w_start = 0;
  char w_buf[32];

  w_start = micros();
  for(count = 0; count < g_dataCount; count++){
    time = g_data[count];
    us = micros();
    do {
      digitalWrite(IR_OUT, !(count&1));
      delayMicroseconds(8);
      digitalWrite(IR_OUT, 0);
      delayMicroseconds(7);
    }while(s4(us + time - micros()) > 0);
  }
  sprintf(w_buf,"OK micros (%ld)usec" ,micros()-w_start);
  Serial.printf("%s\n" ,w_buf);
}
//⑥EEPROMに保存  
void sub_putEEPROM(u1 p_ch) {
  Serial.printf("sub_putEEPROM ch(%d)\n" ,p_ch);
  /*
  //DEBUG
  Serial.printf("g_dataCount : %d\n", g_dataCount);
  Serial.printf("g_data0 : %d %x\n", g_data[0], g_data[0]);
  Serial.printf("g_data1 : %d %x\n", g_data[1], g_data[1]);
  Serial.printf("g_data2 : %d %x\n", g_data[2], g_data[2]);
  */
  //DEBUG Serial.printf("sub_putEEPROM DEBUG DUMMY!!!\n");
  
  int wh;
  int wl;
  int w_ch = p_ch;
  int16_t w_len = g_dataCount;  //バイト数ではなくデータ個数。*2でバイト数。
  int w_start = (DATA_MAXLENGTH*(p_ch-1));
  Serial.printf("sub_putEEPROM w_ch(%d) w_len(%d) w_start(%d) \n",
     w_ch, w_len, w_start);

  if (w_len > DATA_SIZE) {
    Serial.printf("sub_putEEPROM w_len(%d) > DATA_SIZE ERROR!!!\n", w_len);
    return;
  }

#if 1
  //len を書く
  wh = w_len / 256;  //上位バイト
  wl = w_len % 256;  //下位バイト
  EEPROM.write(w_start++, wh);
  EEPROM.write(w_start++, wl);
#else
  w_start++;
  w_start++;
#endif
  int i;
  for (i=0; i<w_len; i++) {
    //一回に2バイト書く
    wh = g_data[i] / 256;  //上位バイト
    wl = g_data[i] % 256;  //下位バイト
#if 1
    EEPROM.write(w_start++, wh);
    EEPROM.write(w_start++, wl);
#else
    //wh,wl の計算検証OK
    // 2,45 = 2*256+45=557
    // 35,207 = 35*256+207=8960+207=9167  9167
    Serial.printf("i(%d) wh(%d) wl(%d)\n" ,i ,wh, wl);
#endif
  }
  //注意）　commit数を節約すること。MAX10000回でハード制約を超える。
#if 1
  EEPROM.commit();
  Serial.printf("sub_putEEPROM commit i(%d)\n" ,i);
#else
  Serial.printf("sub_putEEPROM end i(%d)\n" ,i);
#endif

}
//
//③EEPROMから読む 
void sub_getEEPROM(u1 p_ch) {
  Serial.printf("sub_getEEPROM ch(%d)\n" ,p_ch);

 // EEPROM address 0 - 1023
 // p_Ch=1 0,1:len 2-1536:data
 // p_Ch=2 1536,1537:len 1538-3072:data 1536*(ch-1)-1
 // w_start = (1536*(p_ch-1) + 2);
  Serial.printf("sub_getEEPROM DEBUG DUMMY!!!\n");
 
  int wh = EEPROM.read(DATA_MAXLENGTH*(p_ch-1));
  int wl = EEPROM.read(DATA_MAXLENGTH*(p_ch-1)+1);
  int16_t w_len = wh * 256 + wl;
  int w_start = (DATA_MAXLENGTH*(p_ch-1) + 2);
  Serial.printf("sub_getEEPROM w_ch(%d) w_len(%d) w_start(%d) \n",
     p_ch, w_len, w_start);
  
  if ((p_ch < 1) || (p_ch > 2)) {
    //EEPROMが保存されていない チャンネルは 1－2
    Serial.printf("sub_getEEPROM w_ch(%d) ERROR!!! \n", p_ch);
    g_dataCount = 0;
    return;
  }
  if (w_len > DATA_SIZE) {
    //EEPROMのデータ長が不正。w_lenは 0も許す。最大767を取りうる。
    Serial.printf("sub_getEEPROM w_len(%d 0x%x) ERROR!!! \n", w_len, w_len);
    g_dataCount = 0;
    return;
  }

  int w_count;
  int w_len2 = w_len * 2;
  Serial.printf("p%d,",p_ch);  //Serial 出力した文字列がコピペで使えるように。
  w_count = 0;
  for (int i=w_start; i<(w_start + w_len2); i++) {
    //2バイトを一つの数値として取り出す
    // i=2 HIGH i=3 LOW ...
    if ((i & 0x01) == 0 ) {
      //偶数
      wh = EEPROM.read(i);
    } else {
      //奇数
      wl = EEPROM.read(i);
      g_data[w_count++] = wh * 256 + wl;
      Serial.printf("%d,", g_data[w_count-1]);
    }
  }
  g_dataCount = w_len;  //データ個数
  Serial.printf("0,");
  // g_dataCount == w_count なら正常
  Serial.printf("\nsub_getEEPROM g_dataCount(%d) w_count(%d)\n", g_dataCount, w_count);
 
}
/*
 * EEPROMの内部を2分割する。DATA_MAXLENGTH(1536)x2
 *  0,1:データ件数 --- バイト数ではない。最大767
 *  2-1536: データ最大767件。(データ件数+1)x2以降は無効。
 */
