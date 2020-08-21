/*
 * lib_keyboard.h  v1.1
 */
#ifndef __LIB_KEYBOARD_H__
#define __LIB_KEYBOARD_H__

#include <Keyboard.h>
#define WCS_DELAY_T1 150 //T1 ブラウザー応答時間150
#define WCS_DELAY_MOJI 10 //文字間隔

/* プロトタイプ宣言 */ 
void sub_kbd_begin(int8_t);
void sub_moji_tab(char *);
void sub_kbd_print(char *);
void sub_kbd_strok(uint8_t);
void sub_kbd_preURL(void);
void sub_kbd_withmodifire(char, char);
void sub_kbd_toCommandF();

#endif /* __LIB_KEYBOARD_H__ */
