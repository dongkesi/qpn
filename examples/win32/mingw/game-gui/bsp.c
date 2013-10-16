/*****************************************************************************
* Product: BSP for "Fly 'n' Shoot" game example, Win32
* Last Updated for Version: 5.1.1
* Date of the Last Update:  Oct 11, 2013
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* Quantum Leaps Web sites: http://www.quantum-leaps.com
*                          http://www.state-machine.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#include "qpn_port.h"
#include "game.h"
#include "bsp.h"

#include "win32_gui.h"      /* Win32 GUI elements for embedded front panels */
#include "resource.h" /* GUI resource IDs generated by the resource editior */

#include <stdio.h>                                        /* for snprintf() */

Q_DEFINE_THIS_FILE

/* local variables ---------------------------------------------------------*/
static HINSTANCE l_hInst;                      /* this application instance */
static HWND      l_hWnd;                              /* main window handle */
static LPSTR     l_cmdLine;                      /* the command line string */

static GraphicDisplay   l_oled; /* the OLED display of the EK-LM3S811 board */
static SegmentDisplay   l_userLED;      /* USER LED of the EK-LM3S811 board */
static SegmentDisplay   l_scoreBoard;      /* segment display for the score */
static OwnerDrawnButton l_userBtn;   /* USER button of the EK-LM3S811 board */

/* (R,G,B) colors for the OLED display */
static BYTE const c_onColor [3] = { 255U, 255U,   0U };           /* yellow */
static BYTE const c_offColor[3] = {  15U,  15U,  15U };   /* very dark grey */

static BYTE l_ship_pos = GAME_SHIP_Y;        /* the current ship Y-position */

/* Local functions ---------------------------------------------------------*/
static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg,
                                WPARAM wParam, LPARAM lParam);

/*--------------------------------------------------------------------------*/
void QF_onStartup(void) {
    QF_setTickRate(BSP_TICKS_PER_SEC);
}
/*..........................................................................*/
void QF_onCleanup(void) {
}
/*..........................................................................*/
void QF_onClockTickISR(void) {
    QF_tickXISR(0U);           /* perform the QF-nano clock tick processing */

               /* post TIME_TICK events to all interested active objects... */
    QACTIVE_POST_ISR((QActive *)&AO_Tunnel,  TIME_TICK_SIG, 0U);
    QACTIVE_POST_ISR((QActive *)&AO_Ship,    TIME_TICK_SIG, 0U);
    QACTIVE_POST_ISR((QActive *)&AO_Missile, TIME_TICK_SIG, 0U);
}

/*..........................................................................*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
                   LPSTR cmdLine, int iCmdShow)
{
    HWND hWnd;
    MSG  msg;

    (void)hPrevInst;       /* avoid compiler warning about unused parameter */

    l_hInst   = hInst;                     /* save the application instance */
    l_cmdLine = cmdLine;                    /* save the command line string */

    /* create the main custom dialog window */
    hWnd = CreateCustDialog(hInst, IDD_APPLICATION, NULL,
                            &WndProc, "QP_APP");
    ShowWindow(hWnd, iCmdShow);                     /* show the main window */

    /* enter the message loop... */
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
/*..........................................................................*/
extern int_t main_gui(void);                   /* prototype for appThread() */

/* thread function for running the application main() */
static DWORD WINAPI appThread(LPVOID par) {
    (void)par;                                          /* unused parameter */
    return main_gui();                            /* run the QF application */
}
/*..........................................................................*/
static LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg,
                                WPARAM wParam, LPARAM lParam)
{
    switch (iMsg) {

        /* Perform initialization upon cration of the main dialog window
        * NOTE: Any child-windows are NOT created yet at this time, so
        * the GetDlgItem() function can't be used (it will return NULL).
        */
        case WM_CREATE: {
            l_hWnd = hWnd;                        /* save the window handle */

            /* initialize the owner-drawn buttons...
            * NOTE: must be done *before* the first drawing of the buttons,
            * so WM_INITDIALOG is too late.
            */
            OwnerDrawnButton_init(&l_userBtn,
                       LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_BTN_UP)),
                       LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_BTN_DWN)),
                       LoadCursor(NULL, IDC_HAND));
            return 0;
        }

        /* Perform initialization after all child windows have been created */
        case WM_INITDIALOG: {
            GraphicDisplay_init(&l_oled,
                       BSP_SCREEN_WIDTH,  2U,    /* scale horizontally by 2 */
                       BSP_SCREEN_HEIGHT, 2U,      /* scale vertically by 2 */
                       GetDlgItem(hWnd, IDC_LCD), c_offColor);

            SegmentDisplay_init(&l_userLED,
                                1U,         /* 1 "segment" (the LED itself) */
                                2U);   /* 2 bitmaps (for LED OFF/ON states) */
            SegmentDisplay_initSegment(&l_userLED,
                 0U, GetDlgItem(hWnd, IDC_LED));
            SegmentDisplay_initBitmap(&l_userLED,
                 0U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_LED_OFF)));
            SegmentDisplay_initBitmap(&l_userLED,
                 1U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_LED_ON)));

            SegmentDisplay_init(&l_scoreBoard,
                                4U,            /* 4 "segments" (digits 0-3) */
                                10U);        /* 10 bitmaps (for 0-9 states) */
            SegmentDisplay_initSegment(&l_scoreBoard,
                 0U, GetDlgItem(hWnd, IDC_SEG0));
            SegmentDisplay_initSegment(&l_scoreBoard,
                 1U, GetDlgItem(hWnd, IDC_SEG1));
            SegmentDisplay_initSegment(&l_scoreBoard,
                 2U, GetDlgItem(hWnd, IDC_SEG2));
            SegmentDisplay_initSegment(&l_scoreBoard,
                 3U, GetDlgItem(hWnd, IDC_SEG3));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 0U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG0)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 1U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG1)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 2U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG2)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 3U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG3)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 4U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG4)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 5U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG5)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 6U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG6)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 7U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG7)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 8U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG8)));
            SegmentDisplay_initBitmap(&l_scoreBoard,
                 9U, LoadBitmap(l_hInst, MAKEINTRESOURCE(IDB_SEG9)));

            BSP_updateScore(0U);

            /* --> QP: spawn the application thread to run main() */
            Q_ALLEGE(CreateThread(NULL, 0, &appThread, NULL, 0, NULL)
                     != (HANDLE)0);
            return 0;
        }

        case WM_DESTROY: {
            BSP_terminate(0);
            return 0;
        }

        /* commands from regular buttons and menus... */
        case WM_COMMAND: {
            SetFocus(hWnd);
            switch (wParam) {
                case IDOK:
                case IDCANCEL: {
                    BSP_terminate(0);
                    break;
                }
            }
            return 0;
        }

        /* owner-drawn buttons... */
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            switch (pdis->CtlID) {
                case IDC_USER: {                 /* USER owner-drawn button */
                    switch (OwnerDrawnButton_draw(&l_userBtn, pdis)) {
                        case BTN_DEPRESSED: {
                            BSP_playerTrigger();
                            SegmentDisplay_setSegment(&l_userLED, 0U, 1U);
                            break;
                        }
                        case BTN_RELEASED: {
                            SegmentDisplay_setSegment(&l_userLED, 0U, 0U);
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }
            }
            return 0;
        }

        /* mouse input... */
        case WM_MOUSEWHEEL: {
            if ((HIWORD(wParam) & 0x8000U) == 0U) {/* wheel turned forward? */
                BSP_moveShipUp();
            }
            else {                        /* the wheel was turned backwards */
                BSP_moveShipDown();
            }
            return 0;
        }

        /* keyboard input... */
        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_UP:
                    BSP_moveShipUp();
                    break;
                case VK_DOWN:
                    BSP_moveShipDown();
                    break;
                case VK_SPACE:
                    BSP_playerTrigger();
                    break;
            }
            return 0;
        }

    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam) ;
}

/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    char message[80];
    QF_stop();                                              /* stop ticking */
    snprintf(message, Q_DIM(message) - 1,
             "Assertion failed in module %s line %d", file, line);
    MessageBox(l_hWnd, message, "!!! ASSERTION !!!",
               MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    PostQuitMessage(-1);
}

/*..........................................................................*/
void BSP_init(void) {
}
/*..........................................................................*/
void BSP_terminate(int16_t result) {
    QF_stop();                        /* stop the main QF applicatin thread */
    PostQuitMessage(result);            /* post the Quit message to the GUI */

    GraphicDisplay_xtor(&l_oled);        /* cleanup the l_oled    resources */
    OwnerDrawnButton_xtor(&l_userBtn);   /* cleanup the l_userBtn resources */
    SegmentDisplay_xtor(&l_userLED);     /* cleanup the l_userLED resources */
    SegmentDisplay_xtor(&l_scoreBoard); /* cleanup the scoreBoard resources */
}
/*..........................................................................*/
void BSP_drawBitmap(uint8_t const *bitmap) {
    UINT x, y;
    /* map the EK-LM3S811 OLED pixels to the GraphicDisplay pixels... */
    for (y = 0; y < BSP_SCREEN_HEIGHT; ++y) {
        for (x = 0; x < BSP_SCREEN_WIDTH; ++x) {
            uint8_t bits = bitmap[x + (y/8U)*BSP_SCREEN_WIDTH];
            if ((bits & (1U << (y & 0x07U))) != 0U) {
                GraphicDisplay_setPixel(&l_oled, x, y, c_onColor);
            }
            else {
                GraphicDisplay_clearPixel(&l_oled, x, y);
            }
        }
    }
    GraphicDisplay_redraw(&l_oled);           /* redraw the updated display */
}
/*..........................................................................*/
void BSP_drawNString(uint8_t x, uint8_t y, char const *str) {
    static uint8_t const font5x7[95][5] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00 },                            /* ' ' */
        { 0x00, 0x00, 0x4F, 0x00, 0x00 },                              /* ! */
        { 0x00, 0x07, 0x00, 0x07, 0x00 },                              /* " */
        { 0x14, 0x7F, 0x14, 0x7F, 0x14 },                              /* # */
        { 0x24, 0x2A, 0x7F, 0x2A, 0x12 },                              /* $ */
        { 0x23, 0x13, 0x08, 0x64, 0x62 },                              /* % */
        { 0x36, 0x49, 0x55, 0x22, 0x50 },                              /* & */
        { 0x00, 0x05, 0x03, 0x00, 0x00 },                              /* ' */
        { 0x00, 0x1C, 0x22, 0x41, 0x00 },                              /* ( */
        { 0x00, 0x41, 0x22, 0x1C, 0x00 },                              /* ) */
        { 0x14, 0x08, 0x3E, 0x08, 0x14 },                              /* * */
        { 0x08, 0x08, 0x3E, 0x08, 0x08 },                              /* + */
        { 0x00, 0x50, 0x30, 0x00, 0x00 },                              /* , */
        { 0x08, 0x08, 0x08, 0x08, 0x08 },                              /* - */
        { 0x00, 0x60, 0x60, 0x00, 0x00 },                              /* . */
        { 0x20, 0x10, 0x08, 0x04, 0x02 },                              /* / */
        { 0x3E, 0x51, 0x49, 0x45, 0x3E },                              /* 0 */
        { 0x00, 0x42, 0x7F, 0x40, 0x00 },                              /* 1 */
        { 0x42, 0x61, 0x51, 0x49, 0x46 },                              /* 2 */
        { 0x21, 0x41, 0x45, 0x4B, 0x31 },                              /* 3 */
        { 0x18, 0x14, 0x12, 0x7F, 0x10 },                              /* 4 */
        { 0x27, 0x45, 0x45, 0x45, 0x39 },                              /* 5 */
        { 0x3C, 0x4A, 0x49, 0x49, 0x30 },                              /* 6 */
        { 0x01, 0x71, 0x09, 0x05, 0x03 },                              /* 7 */
        { 0x36, 0x49, 0x49, 0x49, 0x36 },                              /* 8 */
        { 0x06, 0x49, 0x49, 0x29, 0x1E },                              /* 9 */
        { 0x00, 0x36, 0x36, 0x00, 0x00 },                              /* : */
        { 0x00, 0x56, 0x36, 0x00, 0x00 },                              /* ; */
        { 0x08, 0x14, 0x22, 0x41, 0x00 },                              /* < */
        { 0x14, 0x14, 0x14, 0x14, 0x14 },                              /* = */
        { 0x00, 0x41, 0x22, 0x14, 0x08 },                              /* > */
        { 0x02, 0x01, 0x51, 0x09, 0x06 },                              /* ? */
        { 0x32, 0x49, 0x79, 0x41, 0x3E },                              /* @ */
        { 0x7E, 0x11, 0x11, 0x11, 0x7E },                              /* A */
        { 0x7F, 0x49, 0x49, 0x49, 0x36 },                              /* B */
        { 0x3E, 0x41, 0x41, 0x41, 0x22 },                              /* C */
        { 0x7F, 0x41, 0x41, 0x22, 0x1C },                              /* D */
        { 0x7F, 0x49, 0x49, 0x49, 0x41 },                              /* E */
        { 0x7F, 0x09, 0x09, 0x09, 0x01 },                              /* F */
        { 0x3E, 0x41, 0x49, 0x49, 0x7A },                              /* G */
        { 0x7F, 0x08, 0x08, 0x08, 0x7F },                              /* H */
        { 0x00, 0x41, 0x7F, 0x41, 0x00 },                              /* I */
        { 0x20, 0x40, 0x41, 0x3F, 0x01 },                              /* J */
        { 0x7F, 0x08, 0x14, 0x22, 0x41 },                              /* K */
        { 0x7F, 0x40, 0x40, 0x40, 0x40 },                              /* L */
        { 0x7F, 0x02, 0x0C, 0x02, 0x7F },                              /* M */
        { 0x7F, 0x04, 0x08, 0x10, 0x7F },                              /* N */
        { 0x3E, 0x41, 0x41, 0x41, 0x3E },                              /* O */
        { 0x7F, 0x09, 0x09, 0x09, 0x06 },                              /* P */
        { 0x3E, 0x41, 0x51, 0x21, 0x5E },                              /* Q */
        { 0x7F, 0x09, 0x19, 0x29, 0x46 },                              /* R */
        { 0x46, 0x49, 0x49, 0x49, 0x31 },                              /* S */
        { 0x01, 0x01, 0x7F, 0x01, 0x01 },                              /* T */
        { 0x3F, 0x40, 0x40, 0x40, 0x3F },                              /* U */
        { 0x1F, 0x20, 0x40, 0x20, 0x1F },                              /* V */
        { 0x3F, 0x40, 0x38, 0x40, 0x3F },                              /* W */
        { 0x63, 0x14, 0x08, 0x14, 0x63 },                              /* X */
        { 0x07, 0x08, 0x70, 0x08, 0x07 },                              /* Y */
        { 0x61, 0x51, 0x49, 0x45, 0x43 },                              /* Z */
        { 0x00, 0x7F, 0x41, 0x41, 0x00 },                              /* [ */
        { 0x02, 0x04, 0x08, 0x10, 0x20 },                              /* \ */
        { 0x00, 0x41, 0x41, 0x7F, 0x00 },                              /* ] */
        { 0x04, 0x02, 0x01, 0x02, 0x04 },                              /* ^ */
        { 0x40, 0x40, 0x40, 0x40, 0x40 },                              /* _ */
        { 0x00, 0x01, 0x02, 0x04, 0x00 },                              /* ` */
        { 0x20, 0x54, 0x54, 0x54, 0x78 },                              /* a */
        { 0x7F, 0x48, 0x44, 0x44, 0x38 },                              /* b */
        { 0x38, 0x44, 0x44, 0x44, 0x20 },                              /* c */
        { 0x38, 0x44, 0x44, 0x48, 0x7F },                              /* d */
        { 0x38, 0x54, 0x54, 0x54, 0x18 },                              /* e */
        { 0x08, 0x7E, 0x09, 0x01, 0x02 },                              /* f */
        { 0x0C, 0x52, 0x52, 0x52, 0x3E },                              /* g */
        { 0x7F, 0x08, 0x04, 0x04, 0x78 },                              /* h */
        { 0x00, 0x44, 0x7D, 0x40, 0x00 },                              /* i */
        { 0x20, 0x40, 0x44, 0x3D, 0x00 },                              /* j */
        { 0x7F, 0x10, 0x28, 0x44, 0x00 },                              /* k */
        { 0x00, 0x41, 0x7F, 0x40, 0x00 },                              /* l */
        { 0x7C, 0x04, 0x18, 0x04, 0x78 },                              /* m */
        { 0x7C, 0x08, 0x04, 0x04, 0x78 },                              /* n */
        { 0x38, 0x44, 0x44, 0x44, 0x38 },                              /* o */
        { 0x7C, 0x14, 0x14, 0x14, 0x08 },                              /* p */
        { 0x08, 0x14, 0x14, 0x18, 0x7C },                              /* q */
        { 0x7C, 0x08, 0x04, 0x04, 0x08 },                              /* r */
        { 0x48, 0x54, 0x54, 0x54, 0x20 },                              /* s */
        { 0x04, 0x3F, 0x44, 0x40, 0x20 },                              /* t */
        { 0x3C, 0x40, 0x40, 0x20, 0x7C },                              /* u */
        { 0x1C, 0x20, 0x40, 0x20, 0x1C },                              /* v */
        { 0x3C, 0x40, 0x30, 0x40, 0x3C },                              /* w */
        { 0x44, 0x28, 0x10, 0x28, 0x44 },                              /* x */
        { 0x0C, 0x50, 0x50, 0x50, 0x3C },                              /* y */
        { 0x44, 0x64, 0x54, 0x4C, 0x44 },                              /* z */
        { 0x00, 0x08, 0x36, 0x41, 0x00 },                              /* { */
        { 0x00, 0x00, 0x7F, 0x00, 0x00 },                              /* | */
        { 0x00, 0x41, 0x36, 0x08, 0x00 },                              /* } */
        { 0x02, 0x01, 0x02, 0x04, 0x02 },                              /* ~ */
    };
    UINT dx, dy;

    while (*str != '\0') {
        uint8_t const *ch = &font5x7[*str - ' '][0];
        for (dx = 0U; dx < 5U; ++dx) {
            for (dy = 0U; dy < 8U; ++dy) {
                if ((ch[dx] & (1U << dy)) != 0U) {
                    GraphicDisplay_setPixel(&l_oled, (UINT)(x + dx),
                                         (UINT)(y*8U + dy), c_onColor);
                }
                else {
                    GraphicDisplay_clearPixel(&l_oled, (UINT)(x + dx),
                                           (UINT)(y*8U + dy));
                }
            }
        }
        ++str;
        x += 6;
    }
    GraphicDisplay_redraw(&l_oled);           /* redraw the updated display */
}
/*..........................................................................*/
void BSP_updateScore(uint16_t score) {
    /* update the score in the l_scoreBoard SegmentDisplay */
    SegmentDisplay_setSegment(&l_scoreBoard, 0U, (UINT)(score % 10U));
    score /= 10U;
    SegmentDisplay_setSegment(&l_scoreBoard, 1U, (UINT)(score % 10U));
    score /= 10U;
    SegmentDisplay_setSegment(&l_scoreBoard, 2U, (UINT)(score % 10U));
    score /= 10U;
    SegmentDisplay_setSegment(&l_scoreBoard, 3U, (UINT)(score % 10U));
}
/*..........................................................................*/
void BSP_displayOn(void) {
    SegmentDisplay_setSegment(&l_userLED, 0U, 1U);
}
/*..........................................................................*/
void BSP_displayOff(void) {
    SegmentDisplay_setSegment(&l_userLED, 0U, 0U);
    GraphicDisplay_clear(&l_oled);
    GraphicDisplay_redraw(&l_oled);
}
/*..........................................................................*/
void BSP_playerTrigger(void) {
    QACTIVE_POST((QActive *)&AO_Ship,   PLAYER_TRIGGER_SIG, 0U);
    QACTIVE_POST((QActive *)&AO_Tunnel, PLAYER_TRIGGER_SIG, 0U);
}
/*..........................................................................*/
void BSP_moveShipUp(void) {
    if (l_ship_pos > 0U) {
        --l_ship_pos;
    }
    QACTIVE_POST((QActive *)&AO_Ship, PLAYER_SHIP_MOVE_SIG,
                 ((l_ship_pos << 8) | GAME_SHIP_X));
}
//............................................................................
void BSP_moveShipDown(void) {
    if (l_ship_pos < (GAME_SCREEN_HEIGHT - 3U)) {
        ++l_ship_pos;
    }
    QACTIVE_POST((QActive *)&AO_Ship, PLAYER_SHIP_MOVE_SIG,
                 ((l_ship_pos << 8) | GAME_SHIP_X));
}
