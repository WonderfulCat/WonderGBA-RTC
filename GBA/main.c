#include <gba.h>
#include <gba_console.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <stdio.h>
#include <string.h>

#define UNBCD(x) (((x) & 0xF) + (((x) >> 4) * 10))
#define TOBCD(x) ((((x) / 10) << 4) | ((x) % 10))
#define RTC_DATA ((vu16 *)0x080000C4)
#define RTC_RW ((vu16 *)0x080000C6)
#define RTC_ENABLE ((vu16 *)0x080000C8)
#define CART_NAME ((vu8 *)0x080000A0)

#define _YEAR	0
#define _MONTH	1
#define _DAY	2
#define _WKD	3
#define _HOUR	4
#define _MIN	5
#define _SEC	6

#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))
#define COLOR_DARK_BG   RGB15(5, 5, 8)    
#define COLOR_WHITE     RGB15(31, 31, 31) 

void iprintf_center(const char* str) {
    int len = strlen(str);
    int spaces = (30 - len) / 2; 
    if (spaces < 0) spaces = 0;
    for (int i = 0; i < spaces; i++) putchar(' ');
    iprintf("%s", str);
}

// ---------- 保持原本稳定的底层时序 ----------
void rtc_cmd(int v) {
	int l; u16 b; v = v << 1;
	for(l = 7; l >= 0; l--) {
		b = (v >> l) & 0x2;
		*RTC_DATA = b | 4; *RTC_DATA = b | 4; *RTC_DATA = b | 4; *RTC_DATA = b | 5;
	}
}
void rtc_data(int v) {
	int l; u16 b; v = v << 1;
	for(l = 0; l < 8; l++) {
		b = (v >> l) & 0x2;
		*RTC_DATA = b | 4; *RTC_DATA = b | 4; *RTC_DATA = b | 4; *RTC_DATA = b | 5;
	}
}
int rtc_read(void) {
	int j, l; u16 b; int v = 0;
	for(l = 0; l < 8; l++) {
		for(j = 0; j < 5; j++) *RTC_DATA = 4;
		*RTC_DATA = 5; b = *RTC_DATA;
		v = v | ((b & 2) << l);
	}
	return v >> 1;
}
static int check_val = 0;
void rtc_enable(void) {
	*RTC_ENABLE = 1; *RTC_DATA = 1; *RTC_DATA = 5; *RTC_RW = 7;
	rtc_cmd(0x63); *RTC_RW = 5; check_val = rtc_read();
}
int rtc_get(u8 *data) {
	int i; *RTC_DATA = 1; *RTC_RW = 7; *RTC_DATA = 1; *RTC_DATA = 5;
	rtc_cmd(0x65); *RTC_RW = 5;
	for(i = 0; i < 4; i++) data[i] = (u8)rtc_read();
	*RTC_RW = 5;
	for(i = 4; i < 7; i++) data[i] = (u8)rtc_read();
	return 0;
}
void rtc_set(u8 *data) {
	int i; *RTC_ENABLE = 1; *RTC_DATA = 1; *RTC_DATA = 5; *RTC_RW = 7;
	rtc_cmd(0x64);
	for(i = 0; i < 7; i++) rtc_data(data[i]);
	*RTC_RW = 5;
}

void getGameString(u8 *gametitle) {
	for(int i = 0; i < 12; i++) gametitle[i] = CART_NAME[i];
	gametitle[12] = '\0';
}

// ---------- 界面绘制 ----------
void drawMainScreen(u8 *gamename, u8 *datetime) {
	iprintf("\x1b[2J");
	iprintf("\n Real Time Clock Reader\n ----------------------------\n\n Cart: %s\n", gamename);
	if (check_val & 0x80) {
		iprintf("\n !! Power flag raised !!\n Battery probably dead.\n");
	} else {
		iprintf("\n Date (dd/mm/yyyy)\n\n"); 
		iprintf("  %02d / %02d / 20%02d  wkd: %d\n\n", UNBCD(datetime[2] & 0x3F), UNBCD(datetime[1]), UNBCD(datetime[0]), UNBCD(datetime[3]));
		iprintf(" Time (hh:mm:ss)\n\n");
		iprintf("  %02d : %02d : %02d\n\n", UNBCD(datetime[4] & 0x3F), UNBCD(datetime[5]), UNBCD(datetime[6]));
	}
	iprintf(" Power:%u  12/24:%u  IntAE:%u\n", (check_val & 0x80) >> 7, (check_val & 0x40) >> 6, (check_val & 0x20) >> 5);
	iprintf(" IntME:%u  IntFE:%u\n", (check_val & 0x08) >> 3, (check_val & 0x02) >> 1);
	iprintf("\n");
	iprintf_center("SELECT:Edit"); // 排版居中
}

void drawEditScreen(u8 *edit_datetime, int edit_pos) {
	iprintf("\x1b[2J\n Edit Real-Time Clock\n ----------------------------\n\n Date (dd/mm/yyyy)\n\n  ");
	iprintf("%s   %s     %s       %s", (edit_pos == 0 ? "--" : "  "), (edit_pos == 1 ? "--" : "  "), (edit_pos == 2 ? "--" : "  "), (edit_pos == 3 ? "-" : " "));
	iprintf("\n  %02d / %02d / 20%02d  wkd: %d\n  ", edit_datetime[_DAY], edit_datetime[_MONTH], edit_datetime[_YEAR], edit_datetime[_WKD]);
	iprintf("%s   %s     %s       %s", (edit_pos == 0 ? "--" : "  "), (edit_pos == 1 ? "--" : "  "), (edit_pos == 2 ? "--" : "  "), (edit_pos == 3 ? "-" : " "));
	iprintf("\n\n Time (hh:mm:ss)\n\n  ");
	iprintf("%s   %s   %s", (edit_pos == 4 ? "--" : "  "), (edit_pos == 5 ? "--" : "  "), (edit_pos == 6 ? "--" : "  "));
	iprintf("\n  %02d : %02d : %02d\n  ", edit_datetime[_HOUR], edit_datetime[_MIN], edit_datetime[_SEC]);
	iprintf("%s   %s   %s", (edit_pos == 4 ? "--" : "  "), (edit_pos == 5 ? "--" : "  "), (edit_pos == 6 ? "--" : "  "));
	iprintf("\n\n START:Save    SELECT:Cancel");
}

// ---------- 主程序 ----------
int main() {
	int keys_pressed;
	u8 gamename[13];
	int gamestate = -1; 
	u8 edit_pos = 0;
	u8 datetime[7], edit_datetime[7];
	u8 last_sec = 0xFF;
	int frame_count = 0;

	irqInit();
	irqEnable(IRQ_VBLANK);
	consoleDemoInit();

	BG_PALETTE[0] = COLOR_DARK_BG;   
	BG_PALETTE[1] = COLOR_WHITE;     

	iprintf("\n Real Time Clock Reader\n ----------------------------\n\n 1. Remove Flash Cart\n 2. Insert PKMN R/S/E Cart\n 3. Press START");
	iprintf("\n\n\n\n\n\n\n\n\n\n\n");
	iprintf_center("Developed by WonderCat\n");
	iprintf_center("http://furlocks-forest.net");

	while(1) {
		VBlankIntrWait(); 
		scanKeys();
		keys_pressed = keysDown();

		if (gamestate == -1) {
			if (keys_pressed & KEY_START) {
				getGameString(gamename);
				rtc_enable();
				rtc_get(datetime);
				last_sec = datetime[_SEC];
				drawMainScreen(gamename, datetime);
				gamestate = 0; 
			}
		} 
		else if (gamestate == 0) {
			frame_count++;
			if (frame_count >= 10) { 
				rtc_get(datetime);
				frame_count = 0;
				if (datetime[_SEC] != last_sec) {
					last_sec = datetime[_SEC];
					drawMainScreen(gamename, datetime);
				}
			}

			if ((keys_pressed & KEY_SELECT) && !(check_val & 0x80)) {
				rtc_enable(); rtc_get(datetime);
				edit_datetime[_HOUR] = UNBCD(datetime[_HOUR] & 0x3F);
				edit_datetime[_MIN]  = UNBCD(datetime[_MIN]);
				edit_datetime[_SEC]  = UNBCD(datetime[_SEC]);
				edit_datetime[_DAY]  = UNBCD(datetime[_DAY] & 0x3F);
				edit_datetime[_MONTH]= UNBCD(datetime[_MONTH]);
				edit_datetime[_YEAR] = UNBCD(datetime[_YEAR]);
				edit_datetime[_WKD]  = UNBCD(datetime[_WKD]);
				gamestate = 1;
				edit_pos = 0;                         // 光标归位
				drawEditScreen(edit_datetime, edit_pos); // 立即绘制编辑界面
				keys_pressed = 0;                     // 清空按键，防止误触发
			}
		}

		if (gamestate == 1 && keys_pressed) {
			// 处理按键修改
			if (keys_pressed & KEY_UP) {
				switch(edit_pos) {
					case 0: if(edit_datetime[_DAY]>=31) edit_datetime[_DAY]=1; else edit_datetime[_DAY]++; break;
					case 1: if(edit_datetime[_MONTH]==12) edit_datetime[_MONTH]=1; else edit_datetime[_MONTH]++; break;
					case 2: if(edit_datetime[_YEAR]==99) edit_datetime[_YEAR]=0; else edit_datetime[_YEAR]++; break;
					case 3: if(edit_datetime[_WKD]==6) edit_datetime[_WKD]=0; else edit_datetime[_WKD]++; break;
					case 4: if(edit_datetime[_HOUR]==23) edit_datetime[_HOUR]=0; else edit_datetime[_HOUR]++; break;
					case 5: if(edit_datetime[_MIN]==59) edit_datetime[_MIN]=0; else edit_datetime[_MIN]++; break;
					case 6: if(edit_datetime[_SEC]==59) edit_datetime[_SEC]=0; else edit_datetime[_SEC]++; break;
				}
				drawEditScreen(edit_datetime, edit_pos);
			} else if (keys_pressed & KEY_DOWN) {
				switch(edit_pos) {
					case 0: if(edit_datetime[_DAY]<=1) edit_datetime[_DAY]=31; else edit_datetime[_DAY]--; break;
					case 1: if(edit_datetime[_MONTH]==1) edit_datetime[_MONTH]=12; else edit_datetime[_MONTH]--; break;
					case 2: if(edit_datetime[_YEAR]==0) edit_datetime[_YEAR]=99; else edit_datetime[_YEAR]--; break;
					case 3: if(edit_datetime[_WKD]==0) edit_datetime[_WKD]=6; else edit_datetime[_WKD]--; break;
					case 4: if(edit_datetime[_HOUR]==0) edit_datetime[_HOUR]=23; else edit_datetime[_HOUR]--; break;
					case 5: if(edit_datetime[_MIN]==0) edit_datetime[_MIN]=59; else edit_datetime[_MIN]--; break;
					case 6: if(edit_datetime[_SEC]==0) edit_datetime[_SEC]=59; else edit_datetime[_SEC]--; break;
				}
				drawEditScreen(edit_datetime, edit_pos);
			} else if (keys_pressed & KEY_RIGHT) {
				edit_pos = (edit_pos == 6) ? 0 : edit_pos + 1;
				drawEditScreen(edit_datetime, edit_pos);
			} else if (keys_pressed & KEY_LEFT) {
				edit_pos = (edit_pos == 0) ? 6 : edit_pos - 1;
				drawEditScreen(edit_datetime, edit_pos);
			} else if (keys_pressed & KEY_START) {
				u8 bcd[7];
				for(int i=0; i<7; i++) bcd[i] = TOBCD(edit_datetime[i]);
				rtc_set(bcd); 
				VBlankIntrWait(); 
				rtc_get(datetime); 
				last_sec = datetime[_SEC];
				gamestate = 0; 
				frame_count = 0;
				drawMainScreen(gamename, datetime);
			} else if (keys_pressed & KEY_SELECT) {
				gamestate = 0; 
				rtc_get(datetime);
				last_sec = datetime[_SEC];
				frame_count = 0;
				drawMainScreen(gamename, datetime);
			}
		}
	}
	return 0;
}
