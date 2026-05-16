#include <stdio.h>
#include <string.h>
#include <nds.h>

// ---------- 基础宏定义 ----------
#define UNBCD(x) (((x) & 0xF) + (((x) >> 4) * 10))
#define TOBCD(x) ((((x) / 10) << 4) | ((x) % 10))

#define RTC_DATA   ((vu16 *)0x080000C4)
#define RTC_RW     ((vu16 *)0x080000C6)
#define RTC_ENABLE ((vu16 *)0x080000C8)
#define CART_NAME  ((vu8 *)0x080000A0)

#define RTC_CMD_READ(x)  (((x)<<1) | 0x61)
#define RTC_CMD_WRITE(x) (((x)<<1) | 0x60)

#define _YEAR   0
#define _MONTH  1
#define _DAY    2
#define _WKD    3
#define _HOUR   4
#define _MIN    5
#define _SEC    6

#define GBA_DELAY() swiDelay(30)

// 颜色定义（使用 libnds 的 RGB15）
#define COLOR_DARK_BG  RGB15(5, 5, 8)
#define COLOR_WHITE    RGB15(31, 31, 31)

// ---------- 辅助函数 ----------
void printf_center(const char* str) {
    int len = strlen(str);
    int spaces = (30 - len) / 2;
    if (spaces < 0) spaces = 0;
    for (int i = 0; i < spaces; i++) putchar(' ');
    printf("%s", str);
}

void waitForVBlankNoIrq() {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

// ---------- 底层 RTC 通信（已验证稳定）----------
static int check_val = 0;

void rtc_cmd(int v) {
    int l; u16 b; v = v << 1;
    for(l = 7; l >= 0; l--) {
        b = (v >> l) & 0x2;
        *RTC_DATA = b | 4;  GBA_DELAY();
        *RTC_DATA = b | 4;  GBA_DELAY();
        *RTC_DATA = b | 4;  GBA_DELAY();
        *RTC_DATA = b | 5;  GBA_DELAY();
    }
}

void rtc_data(int v) {
    int l; u16 b; v = v << 1;
    for(l = 0; l < 8; l++) {
        b = (v >> l) & 0x2;
        *RTC_DATA = b | 4;  GBA_DELAY();
        *RTC_DATA = b | 4;  GBA_DELAY();
        *RTC_DATA = b | 4;  GBA_DELAY();
        *RTC_DATA = b | 5;  GBA_DELAY();
    }
}

int rtc_read(void) {
    int j, l; u16 b; int v = 0;
    for(l = 0; l < 8; l++) {
        for(j = 0; j < 5; j++) {
            *RTC_DATA = 4; GBA_DELAY();
        }
        *RTC_DATA = 5; GBA_DELAY();
        b = *RTC_DATA;
        v = v | ((b & 2) << l);
    }
    return v >> 1;
}

void rtc_enable(void) {
    *RTC_ENABLE = 1;
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_DATA = 5; GBA_DELAY();
    *RTC_RW = 7; GBA_DELAY();
    rtc_cmd(RTC_CMD_READ(1));
    *RTC_RW = 5; GBA_DELAY();
    check_val = rtc_read();
    *RTC_DATA = 1; GBA_DELAY();
}

int rtc_check(void) {
    return (check_val & 0x40);
}

// ⚠️ 修正版：连续读取 7 个字节，无中途操作
void rtc_get(u8 *data) {
    int i;
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_DATA = 5; GBA_DELAY();
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_RW   = 7; GBA_DELAY();
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_DATA = 5; GBA_DELAY();
    rtc_cmd(RTC_CMD_READ(2));
    *RTC_RW = 5; GBA_DELAY();
    // 连续读取全部 7 个字节
    for(i = 0; i < 7; i++)
        data[i] = (u8)rtc_read();
    *RTC_DATA = 1; GBA_DELAY();
}

void rtc_set(u8 *data) {
    int i;
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_DATA = 5; GBA_DELAY();
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_RW   = 7; GBA_DELAY();
    *RTC_DATA = 1; GBA_DELAY();
    *RTC_DATA = 5; GBA_DELAY();
    rtc_cmd(RTC_CMD_WRITE(2));
    for(i = 0; i < 7; i++)
        rtc_data(data[i]);
    *RTC_RW = 5; GBA_DELAY();
    *RTC_DATA = 1; GBA_DELAY();
}

void getGameString(u8 *gametitle) {
    for(int i = 0; i < 12; i++)
        gametitle[i] = CART_NAME[i];
    gametitle[12] = '\0';
}

// ---------- 界面绘制 ----------
void drawMainScreen(u8 *gamename, u8 *datetime) {
    printf("\x1b[2J");
    printf("\n Real Time Clock Reader\n ----------------------------\n\n Cart: %s\n", gamename);
    if (check_val & 0x80) {
        printf("\n !! Power flag raised !!\n Battery probably dead.\n");
    } else {
        printf("\n Date (dd/mm/yyyy)\n\n");
        printf("  %02d / %02d / 20%02d  wkd: %d\n\n",
                UNBCD(datetime[2] & 0x3F), UNBCD(datetime[1]),
                UNBCD(datetime[0]), UNBCD(datetime[3]));
        printf(" Time (hh:mm:ss)\n\n");
        printf("  %02d : %02d : %02d\n\n",
                UNBCD(datetime[4] & 0x3F), UNBCD(datetime[5]), UNBCD(datetime[6]));
    }
    printf(" Power:%u  12/24:%u  IntAE:%u\n",
            (check_val & 0x80) >> 7, (check_val & 0x40) >> 6, (check_val & 0x20) >> 5);
    printf(" IntME:%u  IntFE:%u\n",
            (check_val & 0x08) >> 3, (check_val & 0x02) >> 1);
    // 提示推到底部
    printf("\n\n\n\n\n");
    printf_center("SELECT:Edit");
}

void drawEditScreen(u8 *edit_datetime, int edit_pos) {
    printf("\x1b[2J\n Edit Real-Time Clock\n ----------------------------\n\n Date (dd/mm/yyyy)\n\n  ");
    printf("%s   %s     %s       %s",
            (edit_pos == 0 ? "--" : "  "), (edit_pos == 1 ? "--" : "  "),
            (edit_pos == 2 ? "--" : "  "), (edit_pos == 3 ? "-" : " "));
    printf("\n  %02d / %02d / 20%02d  wkd: %d\n  ",
            edit_datetime[_DAY], edit_datetime[_MONTH], edit_datetime[_YEAR], edit_datetime[_WKD]);
    printf("%s   %s     %s       %s",
            (edit_pos == 0 ? "--" : "  "), (edit_pos == 1 ? "--" : "  "),
            (edit_pos == 2 ? "--" : "  "), (edit_pos == 3 ? "-" : " "));
    printf("\n\n Time (hh:mm:ss)\n\n  ");
    printf("%s   %s   %s",
            (edit_pos == 4 ? "--" : "  "), (edit_pos == 5 ? "--" : "  "), (edit_pos == 6 ? "--" : "  "));
    printf("\n  %02d : %02d : %02d\n  ",
            edit_datetime[_HOUR], edit_datetime[_MIN], edit_datetime[_SEC]);
    printf("%s   %s   %s",
            (edit_pos == 4 ? "--" : "  "), (edit_pos == 5 ? "--" : "  "), (edit_pos == 6 ? "--" : "  "));
    // 操作提示推到底部
    printf("\n\n\n\n\n\n");
    printf("  START:Save    SELECT:Cancel");
}

// ---------- 主程序 ----------
int main() {
    // 彻底禁用所有可屏蔽中断
    *((vu32*)0x04000208) = 0;
    *((vu32*)0x04000210) = 0;
    *((vu32*)0x04000214) = 0;

    consoleDemoInit();
    sysSetCartOwner(1);

    BG_PALETTE_SUB[0] = COLOR_DARK_BG;
    BG_PALETTE_SUB[1] = COLOR_WHITE;

    int keys_pressed;
    u8 gamename[13];
    int gamestate = -1;          // -1:初始  0:主界面  1:编辑
    u8 edit_pos = 0;
    u8 datetime[7], edit_datetime[7];
    u8 last_sec = 0xFF;

    // 初始界面
    printf("\n Real Time Clock Reader\n ----------------------------\n\n");
    printf(" 1. Insert PKMN R/S/E Cart\n");
    printf(" 2. Press START\n");
    printf("\n\n\n\n\n\n\n\n\n\n\n");
    printf_center("Developed by WonderCat\n");
    printf_center("http://furlocks-forest.net");

    while(1) {
        waitForVBlankNoIrq();
        scanKeys();
        keys_pressed = keysDown();

        // ----- 状态 -1：初始，等待 START -----
        if (gamestate == -1) {
            if (keys_pressed & KEY_START) {
                getGameString(gamename);
                rtc_enable();
                rtc_get(datetime);
                last_sec = UNBCD(datetime[_SEC]);
                drawMainScreen(gamename, datetime);
                gamestate = 0;
            }
        }

        // ----- 状态 0：主界面，每帧读取、秒变即刷 -----
        else if (gamestate == 0) {
            // 每帧读取 RTC
            rtc_get(datetime);
            u8 sec = UNBCD(datetime[_SEC]);
            if (sec != last_sec) {
                last_sec = sec;
                drawMainScreen(gamename, datetime);
            }

            // 按 SELECT 进入编辑（power flag 未置位）
            if ((keys_pressed & KEY_SELECT) && !(check_val & 0x80)) {
                rtc_enable();
                rtc_get(datetime);
                edit_datetime[_HOUR]  = UNBCD(datetime[_HOUR] & 0x3F);
                edit_datetime[_MIN]   = UNBCD(datetime[_MIN]);
                edit_datetime[_SEC]   = UNBCD(datetime[_SEC]);
                edit_datetime[_DAY]   = UNBCD(datetime[_DAY] & 0x3F);
                edit_datetime[_MONTH] = UNBCD(datetime[_MONTH]);
                edit_datetime[_YEAR]  = UNBCD(datetime[_YEAR]);
                edit_datetime[_WKD]   = UNBCD(datetime[_WKD]);
                gamestate = 1;
                edit_pos = 0;
                drawEditScreen(edit_datetime, edit_pos);
                keys_pressed = 0;   // 防止残留按键误触发编辑
            }
        }

        // ----- 状态 1：编辑界面 -----
        if (gamestate == 1 && keys_pressed) {
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
                // 保存并退出编辑
                u8 bcd[7];
                for(int i=0; i<7; i++) bcd[i] = TOBCD(edit_datetime[i]);
                rtc_set(bcd);
                waitForVBlankNoIrq();
                rtc_get(datetime);
                last_sec = UNBCD(datetime[_SEC]);
                gamestate = 0;
                drawMainScreen(gamename, datetime);
            } else if (keys_pressed & KEY_SELECT) {
                // 取消编辑
                gamestate = 0;
                rtc_get(datetime);
                last_sec = UNBCD(datetime[_SEC]);
                drawMainScreen(gamename, datetime);
            }
        }
    }
    return 0;
}
