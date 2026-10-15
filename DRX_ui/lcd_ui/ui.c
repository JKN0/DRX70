/***************************************************************************

 ui.c

 LCD/encoder-based UI driver for DRX-70.

 15.5.2021  - First version

 ***************************************************************************/

/* --------------------------------------------------------------------------

---------------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <datatypes.h>
#include <compiler_defs.h>
#include <C8051F340_defs.h>
#include "../DRX_ui.h"
#include "../portdefs.h"
#include "../silabs_gen.h"
#include "../patchman.h"
#include "../synth_com/parcom_master.h"
#include "lcd.h"
#include "pages.h"
#include "ui.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define RTC_TIME            5

#define TIME_50MS           (50/RTC_TIME)
#define TIME_100MS          (100/RTC_TIME)
#define TIME_150MS          (150/RTC_TIME)
#define TIME_200MS          (200/RTC_TIME)
#define TIME_300MS          (300/RTC_TIME)
#define TIME_500MS          (500/RTC_TIME)
#define TIME_1S             (1000/RTC_TIME)
#define TIME_3S             (3000/RTC_TIME)

#define OK                   0
#define FAIL                 1

#define BUTTON_NONE          0
#define BUTTON_PF_PRESS      1
#define BUTTON_VAL_PRESS     2
#define BUTTON_EDIT          3
#define BUTTON_EDIT_SHORT    4
#define BUTTON_EDIT_LONG     5
#define BUTTON_IGNORE        6

#define UNDERFLOW            0xF0
#define UNDERFLOW_WORD       0xFFF0
#define VAL_PRESSED          127

#define EMPTY_NAME           "          "

#define NONE                 0xFF

// Graphics chars for drawing the algorithm pictures
#define GC_CORNER_LU         0x00
#define GC_CORNER_RU         0x01
#define GC_CORNER_RD         0x02
#define GC_CORNER_LD         0x03
#define GC_CROSS             0x04
#define GC_FB                0x05
#define GC_INV_T             0x06
#define GC_HBAR              0x2D
#define GC_ARROW             0x7E

#define TESTCLEAR(s,d)       if (s) {s=0; d=1;} else d=0;

#define STEP_BY_ENC_PF(v)    { DISABLE(EX0); v += enc_pf_ctr; enc_pf_ctr = 0; ENABLE(EX0); }
#define STEP_BY_ENC_VAL(v)   { DISABLE(EX1); v += enc_val_ctr; enc_val_ctr = 0; ENABLE(EX1); }

#define GET_ENC_PF_CTR(v)    { DISABLE(EX0); v = enc_pf_ctr; enc_pf_ctr = 0; ENABLE(EX0); }
#define GET_ENC_VAL_CTR(v)   { DISABLE(EX1); v = enc_val_ctr; enc_val_ctr = 0; ENABLE(EX1); }

#define printf               printf_fast

/* =======================================================================
------------------------ I/O ------------------------------------------ */

#define ENC_PF_A             P0_0
#define ENC_PF_B             P0_1
#define ENC_VAL_A            P0_2
#define ENC_VAL_B            P0_3

/* =====================================================================
------------------------ Structures --------------------------------- */

typedef struct mfield_t {
    BYTE r;
    BYTE c;
    BYTE w;
} MFIELD_T;

/* =====================================================================
------------------------  Global variables  ------------------------- */

extern BIT tick;

volatile INT8 enc_pf_ctr = 0;
volatile BIT enc_pf_changed = FALSE;
volatile INT8 enc_val_ctr = 0;
volatile BIT enc_val_changed = FALSE;
BYTE button_pressed = BUTTON_NONE;

XDATA PATCH_T patch;
XDATA DRUMKIT_T drumkit;
XDATA SETTINGS_T settings;
XDATA PATCH_T patch_save_buf;
XDATA DRUMKIT_T drumkit_save_buf;

IDATA int cur_patch = 0;
IDATA INT8 cur_drumkit = 0;

IDATA BYTE edit_mode = MODE_ORIGINAL;

IDATA BYTE cur_mode = MODE_MAIN;
IDATA BYTE cur_page = 1;
IDATA BYTE cur_field = 0;
IDATA BYTE prev_page = 0;
IDATA BYTE prev_mode = MODE_ORIGINAL;
IDATA BYTE cur_main_field = 0;

IDATA BYTE page_max = 0;
IDATA BYTE cur_char = 0;
IDATA int cur_save_pos = 0;

XDATA char cur_save_name[11];
XDATA char old_save_name[11];

IDATA BYTE confirm_save = 0;
BIT scrolling_bank = FALSE;

BIT saving_from_ui = TRUE;

PDATA BYTE tbuf[5];

// Operator types: Modulator or Carrier
// addressed: op_types[algorithm][(page-3)&0x03]
CODE char * CODE op_types[9][4] = {
// op:    1    2    3    4
// op int:3    2    1    0
// page: 3/7  4/8  5/9  6/10
    {    "C", "M", "M", "M" },  // alg 1 (int 0)
    {    "C", "M", "M", "M" },  // alg 2 (int 1)
    {    "C", "M", "M", "M" },  // alg 3 (int 2)
    {    "C", "M", "M", "M" },  // alg 4 (int 3)
    {    "C", "C", "M", "M" },  // alg 5 (int 4)
    {    "C", "C", "C", "M" },  // alg 6 (int 5)
    {    "C", "C", "C", "M" },  // alg 7 (int 6)
    {    "C", "C", "M", "M" },  // alg 8 (int 7)
};

CODE MFIELD_T main_fields[] = {
    // r  c  w
    {  1, 3, 3 },   // Sound pos
    {  2, 3, 3 },   // Drum pos
    {  3, 3, 3 },   // Settings
};
#define MAIN_FIELD_CNT   3

/* =====================================================================
------------------------ Function prototypes ------------------------ */

BYTE ui_mode_main( BIT restart );
BYTE ui_mode_sound( BIT restart );
BYTE ui_mode_drums( BIT restart );
BYTE ui_mode_settings( BIT restart );
BIT ui_page_handler( PAGE_T *page, BIT restart );
void update_name( BYTE row, BYTE col, BYTE *name ) __reentrant;
//BIT save_page_handler( PAGE_T *page, BIT restart );
void ui_alg_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void ui_int_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void ui_choice_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void ui_hex_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void ui_name_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void ui_savepos_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void ui_confirm_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant;
void format_patch_nr( WORD patch_nr, PDATA BYTE *buf );
void show_edit_mode( void );

/* =======================================================================
Page definitions
----------------------------------------------------------------------- */

#include "pages.c"

/* =======================================================================
UI task initialization.
----------------------------------------------------------------------- */

void ui_init( void )
{
    LCD_init();
}

/* =======================================================================
UI main task:
- calls mode specific task functions according to current mode
- keeps patch/drumkit names on the display updated
----------------------------------------------------------------------- */

void ui_task( void )
{
    static BIT restart = TRUE;
    static BYTE tick_ctr = 0;
    BYTE next_mode;

    // Call mode specific functions according to current mode
    // Each function returns the next mode to go
    switch (cur_mode)
    {
    case MODE_MAIN:     next_mode = ui_mode_main(restart);     break;
    case MODE_SOUND:    next_mode = ui_mode_sound(restart);    break;
    case MODE_DRUMS:    next_mode = ui_mode_drums(restart);    break;
    case MODE_SETTINGS: next_mode = ui_mode_settings(restart); break;
    default:            next_mode = MODE_MAIN;                 break;
    }

    // When mode changes, set restart
    restart = (next_mode != cur_mode);
    if (restart)
        prev_mode = cur_mode;

    cur_mode = next_mode;

    // If synth communication fails
    if (pc_error)
    {
        pc_error = FALSE;
        LCD_write_text(0,0,"---- Synth comm error! ----       ");
    }

    // Various patch and drumkit names on the display are updated regularly
    // (20 ms interval) because it takes time to fetch them from EEPROM
    // when user scrolls the patches/drumkits.

    // Run the name updates only on every 4th tick
    if (!tick) return;

    tick_ctr = (tick_ctr + 1) & 0x03;
    if (tick_ctr != 0) return;

    // Update fields according to current mode
    switch (cur_mode)
    {
    case MODE_MAIN:
        // --- Main display: update patch and drumkit names

        // Update cur_patch nr (e.g. A01) on the display
        format_patch_nr(cur_patch,tbuf);
        LCD_field_text(1,3,tbuf);

        // Update patch name on the display
        update_name(1,12,patch.name);

        // If bank scrolling on, set the cursor to bank letter position
        if (scrolling_bank)
            LCD_write_char_cursor(1,4,0);

        // Update cur_drumkit nr on the display
        format_patch_nr(0x1C0|cur_drumkit, tbuf);   // drumkits = bank 7
        LCD_field_text(2,3,tbuf);

        // Update drumkit name on the display
        update_name(2,12,drumkit.name);
        break;

    case MODE_SOUND:    
        // --- Sound editing, only on last page: update save position

        if (cur_page == SOUND_PAGE_CNT) // save page is the last page
        {
            // We are on the save page, update the save position nr
            format_patch_nr(cur_save_pos,tbuf);
            LCD_field_text(2,15,tbuf);

            // Update the current patch name in the save position
            update_name(2,21,old_save_name);

            // If bank scrolling on, set the cursor to bank letter position
            if (scrolling_bank)
                LCD_write_char_cursor(2,16,0);
        }
        break;

    case MODE_DRUMS:    
        // --- Drumkit editing, only on last page: update save position

        if (cur_page == DRUM_PAGE_CNT) // save page is the last page
        {
            // We are on the save page, update the save position nr
            format_patch_nr(0x1C0|cur_save_pos,tbuf);
            LCD_field_text(2,15,tbuf);

            // Update the current drumkit name in the save position
            update_name(2,21,old_save_name);
        }
        break;

    default:
        break;
    }
}

/* =======================================================================
Update patch or drumkit name on the display
----------------------------------------------------------------------- */

void update_name( BYTE row, BYTE col, BYTE *name ) __reentrant
{
    if (name[0] == 0xFF)
        LCD_write_text(row,col,"- empty - ");
    else
        LCD_write_text(row,col,name);
}

/* =======================================================================
Format patch/drumkit nr from patch nr. Drumkits are in bank 7.
----------------------------------------------------------------------- */

void format_patch_nr( WORD patch_nr, PDATA BYTE *buf )
{
    BYTE pnr;

    // Bank letter: 0...6 = A...G, 7 = R
    buf[0] = (patch_nr >> 6) + 'A';
    if (buf[0] == 'H') buf[0] = 'R';

    // Patch nr in bank (0...63, shown as 1...64)
    pnr = (patch_nr & 0x3F) + 1;
    buf[1] = (pnr / (BYTE)10) | 0x30;
    buf[2] = (pnr % (BYTE)10) | 0x30;

    buf[3] = '\0';
}

/* =======================================================================
Main mode (main screen) handler.
'restart' tells, are we restarting the mode after being in another mode,
or is this just a routine call while in the mode.

Returns mode we are going next.
----------------------------------------------------------------------- */

BYTE ui_mode_main( BIT restart )
{
    INT8 vdelta;

    if (restart)
    {
        // --- Mode restart: draw the whole screen

        // Clear screen
        LCD_command(LCDC_BOTH,LCD_CMD_CLEAR);

        // Header row
        LCD_write_text(0,10,"*** DRX70 ***");

        // Fields
        LCD_field_text(main_fields[2].r,main_fields[2].c,"Set");
        // other fields are handled separately

        // Set focus to active field
        LCD_field_focus(main_fields[cur_main_field].r,
                        main_fields[cur_main_field].c,
                        main_fields[cur_main_field].w,
                        FOCUS_ON);
        
        scrolling_bank = FALSE;
    }

    // --- Handle user actions

    // If the PAGE/FIELD encoder has changed, shift focus to next field
    if (enc_pf_changed)
    {
        enc_pf_changed = FALSE;

        LCD_field_focus(main_fields[cur_main_field].r,
                        main_fields[cur_main_field].c,
                        main_fields[cur_main_field].w,
                        FOCUS_OFF);

        STEP_BY_ENC_PF(cur_main_field);

        if (cur_main_field > UNDERFLOW)
            cur_main_field = 0;
        else if (cur_main_field >= MAIN_FIELD_CNT)
            cur_main_field = MAIN_FIELD_CNT-1;

        LCD_field_focus(main_fields[cur_main_field].r,
                        main_fields[cur_main_field].c,
                        main_fields[cur_main_field].w,
                        FOCUS_ON);
    }

    // If the VALUE encoder has changed, step the patch/drumkit nr
    // (ui_task function will update the change to display)
    if (enc_val_changed)
    {
        enc_val_changed = FALSE;

        switch (cur_main_field)
        {
        case 0:
            // Step the patch nr
            GET_ENC_VAL_CTR(vdelta);

            if (scrolling_bank)
                cur_patch += vdelta * 64;
            else
                cur_patch += vdelta;

            if (cur_patch < 0)
                cur_patch = 0;
            else if (cur_patch > MAX_PATCH)
                cur_patch = MAX_PATCH;

            cur_save_pos = cur_patch;
            break;

        case 1:
            // Step the drumkit nr
            STEP_BY_ENC_VAL(cur_drumkit);

            if (cur_drumkit < 0)
                cur_drumkit = 0;
            else if (cur_drumkit > MAX_DRUMKIT)
                cur_drumkit = MAX_DRUMKIT;

            cur_save_pos = cur_drumkit;
            break;

        default:
            enc_val_ctr = 0;
            break;
        }
    }

    if (button_pressed != BUTTON_NONE)
    {
        BYTE ret = MODE_MAIN;

        if (button_pressed == BUTTON_EDIT_SHORT)
        {
            // Edit button short press: change to one of the edit modes
            switch (cur_main_field)
            {
            case 0: ret = MODE_SOUND;    break;
            case 1: ret = MODE_DRUMS;    break;
            case 2: ret = MODE_SETTINGS; break;
            }

            LCD_hide_cursor(LCDC_BOTH);
        }
        else if (button_pressed == BUTTON_VAL_PRESS)
        {
            // Val encoder button press: toggle bank scrolling

            // Accept only in patch field
            if (cur_main_field == 0)
            {
                scrolling_bank = !scrolling_bank;

                if (!scrolling_bank)
                    LCD_hide_cursor(LCDC_BOTH);
            }
        }

        button_pressed = BUTTON_NONE;

        return ret;
    }

    return MODE_MAIN;
}

/* =======================================================================
Sound mode (sound editing screen) handler.
'restart' tells, are we restarting the mode after being in another mode,
or is this just a routine call while in the mode.

Returns mode we are going next.
----------------------------------------------------------------------- */

BYTE ui_mode_sound( BIT mode_restart )
{
    BIT page_restart,exit_mode;

    // When mode entered, start from page 1, field 0
    if (mode_restart)
    {
        cur_page = 1;
        cur_field = 0;
        edit_mode = MODE_ORIGINAL;

        if (patch.name[0] == 0xFF)
            strcpy(patch.name,"- empty - ");

        strcpy(cur_save_name,patch.name);
        strcpy(old_save_name,patch.name);
    }

    // Do page restart, if:
    //   - page changed
    //   - edit mode changed and previous mode was not ORIGINAL 
    //        (we don't want to restart after doing the very first edit)
    page_restart = (cur_page != prev_page) || ((edit_mode != prev_mode) && (prev_mode != MODE_ORIGINAL));
    prev_page = cur_page;
    prev_mode = edit_mode;

    if (page_restart)
    {
        // Clear screen and draw the header row

        LCD_command(LCDC_BOTH,LCD_CMD_CLEAR);

        // Header row: "A01 Op 1(M) envelope   EDITED    Pg  7 "
        format_patch_nr(cur_patch,tbuf);
        LCD_write_text(0,0,tbuf);
        LCD_write_text(0,5,sound_pages[cur_page]->caption);

        // '$' is a placeholder for operator type char 'M' or 'C'
        if (sound_pages[cur_page]->caption[5] == '$')
            LCD_write_text(0,10,op_types[patch.algorithm][(cur_page-3)&0x03]);

        show_edit_mode();
        LCD_write_text(0,34,"Pg");
        LCD_write_pagenr(cur_page,FOCUS_ON);        // Always focus to pagenr after entering page

        page_max = SOUND_PAGE_CNT;
        scrolling_bank = FALSE;
    }

    // Handle page fields
    exit_mode = ui_page_handler(sound_pages[cur_page],page_restart);

    // If exit, return to MAIN mode, otherwise stay in SOUND editing mode
    if (exit_mode)
    {
        cur_page = 0;
        prev_page = 0;
        prev_mode = MODE_ORIGINAL;
        return MODE_MAIN;
    }
    else
        return MODE_SOUND;
}

/* =======================================================================
Drum mode (drumkit editing screen) handler.
'restart' tells, are we restarting the mode after being in another mode,
or is this just a routine call while in the mode.

Returns mode we are going next.
----------------------------------------------------------------------- */

BYTE ui_mode_drums( BIT mode_restart )
{
    BIT page_restart,exit_mode;

    // When mode entered, start from page 1
    if (mode_restart)
    {
        cur_page = 1;
        cur_field = 0;
        edit_mode = MODE_ORIGINAL;

        if (drumkit.name[0] == 0xFF)
            strcpy(drumkit.name,"- empty - ");

        strcpy(cur_save_name,drumkit.name);
        strcpy(old_save_name,drumkit.name);
    }

    // Do page restart, if:
    //   - page changed
    //   - edit mode changed and previous mode was not ORIGINAL 
    //        (we don't want to restart after doing the very first edit)
    page_restart = (cur_page != prev_page) || ((edit_mode != prev_mode) && (prev_mode != MODE_ORIGINAL));
    prev_page = cur_page;
    prev_mode = edit_mode;

    if (page_restart)
    {
        // Clear screen and draw the header row

        LCD_command(LCDC_BOTH,LCD_CMD_CLEAR);

        // Header row
        format_patch_nr(0x1C0|cur_drumkit,tbuf);
        LCD_write_text(0,0,tbuf);
        LCD_write_text(0,5,drum_pages[cur_page]->caption);

        show_edit_mode();
        LCD_write_text(0,34,"Pg");
        LCD_write_pagenr(cur_page,FOCUS_ON);        // Always focus to pagenr after entering page

        page_max = DRUM_PAGE_CNT;
        scrolling_bank = FALSE;
    }

    // Handle page fields
    exit_mode = ui_page_handler(drum_pages[cur_page],page_restart);

    // If exit, return to MAIN mode, otherwise stay in DRUM editing mode
    if (exit_mode)
    {
        cur_page = 0;
        prev_page = 0;
        prev_mode = MODE_ORIGINAL;
        return MODE_MAIN;
    }
    else
        return MODE_DRUMS;
}

/* =======================================================================
Settings mode (settings editing screen) handler.
'restart' tells, are we restarting the mode after being in another mode,
or is this just a routine call while in the mode.

Returns mode we are going next.
----------------------------------------------------------------------- */

BYTE ui_mode_settings( BIT mode_restart )
{
    BIT exit_mode;

    // When mode entered, write the header row
    if (mode_restart)
    {
        cur_page = 1;
        cur_field = 0;
        edit_mode = MODE_ORIGINAL;

        // Clear screen
        LCD_command(LCDC_BOTH,LCD_CMD_CLEAR);

        // Header row
        LCD_write_text(0,0,settings_pages[cur_page]->caption);

        LCD_write_text(0,34,"Pg");
        LCD_write_pagenr(cur_page,FOCUS_ON);        // Always focus to pagenr after entering page

        page_max = SETTINGS_PAGE_CNT;
        scrolling_bank = FALSE;
    }

    // Handle page fields
    exit_mode = ui_page_handler(settings_pages[cur_page],mode_restart);

    // If exit, return to MAIN mode, otherwise stay in SOUND editing mode
    if (exit_mode)
        return MODE_MAIN;
    else
        return MODE_SETTINGS;
}

/* =======================================================================
Generic display page handler, used for all edit modes.
Return true, if user wants to exit current mode.
----------------------------------------------------------------------- */

BIT ui_page_handler( PAGE_T *page, BIT restart )
{
    static BIT scrolling_pages = TRUE;
    BYTE i;
    INT8 enc_delta;
    BIT exit_mode = FALSE;
    BYTE offs;

    // --- When page entered, draw all labels and fields
    if (restart)
    {
        scrolling_pages = TRUE;
        cur_char = NONE;
        confirm_save = 0;

        // Make sure cur_field is not outside this page's fields
        // (possible when previous page had more fields than this page)
        if (cur_field >= page->field_cnt)
            cur_field = 0;

        for (i = 0; i < page->field_cnt; i++)
        {
            // Draw field label, if exists
            if (page->fields[i].label != NULL)
                LCD_write_text(page->fields[i].label->r,
                               page->fields[i].label->c,
                               page->fields[i].label->caption);

            // Draw field value, if exists
            if (page->fields[i].value != NULL)
            {
                // Calling value handler function with delta=0 (no change to current value) 
                // just redraws the contents

                offs = page->fields[i].op_inst;
                // indirect call to field value handler function
                page->fields[i].value->handler(page->fields[i].value,offs,0);   
            }
        }

        LCD_write_pagenr(cur_page,FOCUS_ON);
    }

    // --- Handle encoders and buttons

    // If the PAGE/FIELD encoder has changed, go to next/previous page or field
    if (enc_pf_changed)
    {
        enc_pf_changed = FALSE;
        GET_ENC_PF_CTR(enc_delta);

        if (scrolling_pages)
        {
            // Page scrolling mode: change to next/previous page
            cur_page += enc_delta;

            if (cur_page < 1)
                cur_page = 1;
            else if (cur_page > page_max)
                cur_page = page_max;
        }
        else
        {
            // Field scrolling mode: move focus to next/previous field
            LCD_field_focus(page->fields[cur_field].value->r,
                            page->fields[cur_field].value->c,
                            page->fields[cur_field].value->w,
                            FOCUS_OFF);

            // Loop skips the valueless (label-only) fields
            // Don't put a valueless field as first or last field on page!
            do {
                cur_field += enc_delta;

                if (cur_field > UNDERFLOW)
                    cur_field = 0;
                else if (cur_field >= page->field_cnt)
                    cur_field = page->field_cnt-1;
            }
            while (page->fields[cur_field].value == NULL);

            LCD_field_focus(page->fields[cur_field].value->r,
                            page->fields[cur_field].value->c,
                            page->fields[cur_field].value->w,
                            FOCUS_ON);

            cur_char = NONE;
        }
    }

    // If the VALUE encoder has changed, change current field's value
    if (enc_val_changed)
    {
        enc_val_changed = FALSE;
        GET_ENC_VAL_CTR(enc_delta);

        // Changing field value is allowed when focus is not in pagenr
        if (!scrolling_pages)
        {
            // Update the value of the current field
            offs = page->fields[cur_field].op_inst;
            page->fields[cur_field].value->handler(page->fields[cur_field].value,offs,enc_delta);   // indirect call to handler
            
            // Mark sound/drumkit edited and allow compare mode
            // (don't count changes on save page as editions)
            if (edit_mode == MODE_ORIGINAL && cur_mode != MODE_SETTINGS && cur_page != page_max)
            { 
                edit_mode = MODE_EDITED;
                show_edit_mode();
            }
        }
    }

    // Handle button presses
    if (button_pressed != BUTTON_NONE)
    {
        switch (button_pressed)
        {
        case BUTTON_PF_PRESS:
            // Page/field encoder press: toggle focus between page nr and current field
            
            // Disallow field edits in compare mode
            if (edit_mode != MODE_COMPARE)
                scrolling_pages = !scrolling_pages;

            LCD_write_pagenr(cur_page,
                             scrolling_pages ? FOCUS_ON : FOCUS_OFF);

            LCD_field_focus(page->fields[cur_field].value->r,
                            page->fields[cur_field].value->c,
                            page->fields[cur_field].value->w,
                            scrolling_pages ? FOCUS_OFF : FOCUS_ON);
            break;

        case BUTTON_VAL_PRESS:
            // Value encoder press: this may have functionality in some of the field handlers
            if (!scrolling_pages)
            {
                // Update the value of the current field
                offs = page->fields[cur_field].op_inst;
                page->fields[cur_field].value->handler(page->fields[cur_field].value,offs,VAL_PRESSED);   // indirect call to handler
            }
            break;

         case BUTTON_EDIT_SHORT:
            // Short press of edit button: toggle between compare/edited modes
            if (cur_mode == MODE_SOUND || cur_mode == MODE_DRUMS)
            {
                if (edit_mode == MODE_EDITED)
                {
                    // Change to COMPARE mode: copy current (edited) patch to save buffer
                    // and command patch manager to reload the original patch from EEPROM and send to synth
                    if (cur_mode == MODE_SOUND)
                    {
                        memcpy(&patch_save_buf,&patch,sizeof(PATCH_T));
                        reload_patch = TRUE;
                    }
                    else
                    {
                        memcpy(&drumkit_save_buf,&drumkit,sizeof(DRUMKIT_T));
                        reload_drumkit = TRUE;
                    }
                    // Patch manager changes the edit mode _after_ reloading 
                }
                else if (edit_mode == MODE_COMPARE)
                {
                    // Change to EDITED mode: copy the saved patch to current patch
                    // and send to synth
                    if (cur_mode == MODE_SOUND)
                    {
                        memcpy(&patch,&patch_save_buf,sizeof(PATCH_T));
                        PC_send_patch_to_synth();
                    }
                    else
                    {
                        memcpy(&drumkit,&drumkit_save_buf,sizeof(DRUMKIT_T));
                        PC_send_drumkit_to_synth();
                    }
                    edit_mode = MODE_EDITED;
                }
                // else MODE_ORIGINAL -> compare functionality disabled
                show_edit_mode();
            }
            break;

         case BUTTON_EDIT_LONG:
            // Long press of edit button: discard edits and exit to main screen
            switch (cur_mode)
            {
            case MODE_SOUND:    reload_patch = TRUE;    break;
            case MODE_DRUMS:    reload_drumkit = TRUE;  break;
            case MODE_SETTINGS: reload_settings = TRUE; break;
            }

            exit_mode = TRUE;   // exit to main screen
            break;
        }

        button_pressed = BUTTON_NONE;
    }

    // Update "Saved" text to display after save to EEPROM done
    if (saving_from_ui && saving_state == SAVE_DONE)
    {
        LCD_write_text(3,21,"Saved!");
        saving_state = SAVE_IDLE;
    }

    return exit_mode;
}

/* =======================================================================
Field handlers for different field types
======================================================================= */
/* =======================================================================
Handler used only for for algorithm field. 
Algorithm numbering 1...8 on display, 0...7 internally.
Draws a simple character graphics picture of the selected algorithm.
----------------------------------------------------------------------- */

void ui_alg_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    offs;
    BYTE targval = patch.algorithm + 1;

    if (vdelta == VAL_PRESSED) return;  // No functionality for value encoder press
    
    targval += vdelta;

    if (targval < 1)
        targval = 1;
    else if (targval > 8)
        targval = 8;

    patch.algorithm = targval - 1;

    LCD_field_int1(fval->r,fval->c,targval);

    // update change to synth
    if (vdelta != 0)
        PC_send_byte_param_to_synth(SCMD_SET_ALGORITHM,0,targval-1);

    // Draw algorithm picture
    LCD_write_text(1,30,"          ");
    LCD_write_text(2,30,"          ");
    LCD_write_text(3,30,"          ");

    switch (patch.algorithm)
    {
    case 0:
        LCD_write_char(2,30,GC_FB);
        LCD_write_char(2,31,'4');
        LCD_write_char(2,32,GC_HBAR);
        LCD_write_char(2,33,'3');
        LCD_write_char(2,34,GC_HBAR);
        LCD_write_char(2,35,'2');
        LCD_write_char(2,36,GC_HBAR);
        LCD_write_char(2,37,'1');
        LCD_write_char(2,38,GC_HBAR);
        LCD_write_char(2,39,GC_ARROW);
        break;

    case 1:
        LCD_write_char(1,32,GC_FB);
        LCD_write_char(1,33,'4');
        LCD_write_char(1,34,GC_CORNER_RU);
        LCD_write_char(2,33,'3');
        LCD_write_char(2,34,GC_INV_T);
        LCD_write_char(2,35,'2');
        LCD_write_char(2,36,GC_HBAR);
        LCD_write_char(2,37,'1');
        LCD_write_char(2,38,GC_HBAR);
        LCD_write_char(2,39,GC_ARROW);
        break;

    case 2:
        LCD_write_char(1,34,GC_FB);
        LCD_write_char(1,35,'4');
        LCD_write_char(1,36,GC_CORNER_RU);
        LCD_write_char(2,33,'3');
        LCD_write_char(2,34,GC_HBAR);
        LCD_write_char(2,35,'2');
        LCD_write_char(2,36,GC_INV_T);
        LCD_write_char(2,37,'1');
        LCD_write_char(2,38,GC_HBAR);
        LCD_write_char(2,39,GC_ARROW);
        break;

     case 3:
        LCD_write_char(1,35,'2');
        LCD_write_char(1,36,GC_CORNER_RU);
        LCD_write_char(2,32,GC_FB);
        LCD_write_char(2,33,'4');
        LCD_write_char(2,34,GC_HBAR);
        LCD_write_char(2,34,GC_HBAR);
        LCD_write_char(2,35,'3');
        LCD_write_char(2,36,GC_INV_T);
        LCD_write_char(2,37,'1');
        LCD_write_char(2,38,GC_HBAR);
        LCD_write_char(2,39,GC_ARROW);
        break;

     case 4:
        LCD_write_char(1,34,GC_FB);
        LCD_write_char(1,35,'4');
        LCD_write_char(1,36,GC_HBAR);
        LCD_write_char(1,37,'2');
        LCD_write_char(1,38,GC_CORNER_RU);
        LCD_write_char(2,35,'3');
        LCD_write_char(2,36,GC_HBAR);
        LCD_write_char(2,37,'1');
        LCD_write_char(2,38,GC_INV_T);
        LCD_write_char(2,39,GC_ARROW);
        break;

     case 5:
        LCD_write_char(1,36,GC_CORNER_LU);
        LCD_write_char(1,37,'1');
        LCD_write_char(1,38,GC_CORNER_RU);
        LCD_write_char(2,34,GC_FB);
        LCD_write_char(2,35,'4');
        LCD_write_char(2,36,GC_CROSS);
        LCD_write_char(2,37,'2');
        LCD_write_char(2,38,GC_CROSS);
        LCD_write_char(2,39,GC_ARROW);
        LCD_write_char(3,36,GC_CORNER_LD);
        LCD_write_char(3,37,'3');
        LCD_write_char(3,38,GC_CORNER_RD);
        break;

     case 6:
        LCD_write_char(1,37,'1');
        LCD_write_char(1,38,GC_CORNER_RU);
        LCD_write_char(2,37,'2');
        LCD_write_char(2,38,GC_CROSS);
        LCD_write_char(2,39,GC_ARROW);
        LCD_write_char(3,34,GC_FB);
        LCD_write_char(3,35,'4');
        LCD_write_char(3,36,GC_HBAR);
        LCD_write_char(3,37,'3');
        LCD_write_char(3,38,GC_CORNER_RD);
        break;

     case 7:
        LCD_write_char(2,32,GC_FB);
        LCD_write_char(2,33,'4');
        LCD_write_char(2,34,GC_HBAR);
        LCD_write_char(1,37,'2');
        LCD_write_char(1,38,GC_CORNER_RU);
        LCD_write_char(2,35,'3');
        LCD_write_char(2,36,GC_HBAR);
        LCD_write_char(2,37,'1');
        LCD_write_char(2,38,GC_INV_T);
        LCD_write_char(2,39,GC_ARROW);
        break;
   }
}

/* =======================================================================
Handler for integer fields. Field width may be 1...3 digits.
----------------------------------------------------------------------- */

void ui_int_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    BYTE targval = *((XDATA BYTE *)((BYTE *)(fval->target)+offs));
    BYTE min = fval->min;

    if (vdelta == VAL_PRESSED) return;  // No functionality for value encoder press

    targval += vdelta;

    // Adjust value to limits
    if (targval > UNDERFLOW)
        targval = 0;

    if (targval < min)
        targval = min;
    else if (targval > fval->max)
        targval = fval->max;

    // Store new value to target (patch/drumkit/settings struct)
    *((XDATA BYTE *)((BYTE *)(fval->target)+offs)) = targval;

    // Update change to synth
    if (vdelta != 0)
        PC_send_byte_param_to_synth(fval->scmd,offs,targval);

    // Update change to display
    // Width=3 is used only for detune field, use detune offsetting
    switch (fval->w)
    {
    case 1: LCD_field_int1(fval->r,fval->c,targval); break;
    case 2: LCD_field_int2(fval->r,fval->c,targval); break;
    case 3: LCD_field_signed2(fval->r,fval->c,targval-32); break; // offset: 0..32..63 -> -32..0..+31
    }
}

/* =======================================================================
Handler for choice fields. 
----------------------------------------------------------------------- */

void ui_choice_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    BYTE targval = *((XDATA BYTE *)((BYTE *)(fval->target)+offs));

    if (vdelta == VAL_PRESSED) return;  // No functionality for value encoder press

    targval += vdelta;

    if (targval > UNDERFLOW)    // assume min=0 always
        targval = 0;
    else if (targval > fval->max)
        targval = fval->max;

    *((XDATA BYTE *)((BYTE *)(fval->target)+offs)) = targval;

    // Update change to synth
    if (vdelta != 0)
        PC_send_byte_param_to_synth(fval->scmd,offs,targval);

    // Update change to display
    LCD_field_text(fval->r,fval->c,fval->clist[targval]);
}

/* =======================================================================
Handler for integer fields in range 0...0x1FF, displayed as 3 hex digits.
----------------------------------------------------------------------- */

void ui_hex_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    WORD targval = *((XDATA WORD *)((WORD *)(fval->target)+offs));

    if (vdelta == VAL_PRESSED) // Value encoder press: toggle cursor between 2nd and 3rd digit
    {
        if (cur_char == NONE)
            cur_char = 1;
        else
            cur_char = 1 - cur_char;

        // Set cursor at cur_char
        LCD_write_char_cursor(fval->r,fval->c+cur_char+2,0);
    }
    else if (vdelta == 0)   // Initial field value write
    {
        if (targval > 0x1FF)
            targval = 0x1FF;

        LCD_field_hex3(fval->r,fval->c,targval);
    }
    else // Normal encoder rotation
    {        
        if (cur_char == NONE)
            cur_char = 1;

        // Step the hex digit according to cursor position
        if (cur_char == 0)
            targval += (int)vdelta << 4;
        else    
            targval += vdelta;

        if (targval > 0xFF00)   // if underflow
            targval = 0;
        else if (targval > 0x1FF)
            targval = 0x1FF;

        *((XDATA WORD *)((WORD *)(fval->target)+offs)) = targval;

        // Update change to synth
        PC_send_word_param_to_synth(fval->scmd,offs,targval);

        // Update change to display
        LCD_field_hex3(fval->r,fval->c,targval);
        LCD_write_char_cursor(fval->r,fval->c+cur_char+2,0);
    }
}

/* =======================================================================
Handler for editable patch/drumkit name fields (save name).
----------------------------------------------------------------------- */

void ui_name_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    INT8 ch;

    offs;

    if (cur_save_name[0] == 0xFF)
        strcpy(cur_save_name,"          ");
    
    if (vdelta == VAL_PRESSED)  // Value encoder press: move cursor to next char
    {
        cur_char++;
        if (cur_char > fval->max)
            cur_char = 0;

        LCD_write_char_cursor(fval->r,fval->c+cur_char+1,cur_save_name[cur_char]);
    }
    else if (vdelta == 0)   // Initial field value write
    {
        LCD_field_text(fval->r,fval->c,cur_save_name);
    }
    else if (cur_char != NONE)
    {
        // Normal encoder rotation
        ch = cur_save_name[cur_char];
        ch += vdelta;

        if (ch < ' ')
            ch = ' ';
        else if (ch > 'z')
            ch = 'z';

        cur_save_name[cur_char] = ch;
        LCD_write_char_cursor(fval->r,fval->c+cur_char+1,ch);
    }
}

/* =======================================================================
Handler for patch/drumkit save position field.
----------------------------------------------------------------------- */

void ui_savepos_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    int max_pos = (cur_mode == MODE_SOUND) ? MAX_PATCH : MAX_DRUMKIT;

    fval; offs;
    
    if (vdelta == VAL_PRESSED)  // Value encoder press: start scrolling bank chars (A...G)
    {
        if (cur_mode == MODE_SOUND) // only for patches
        {
            scrolling_bank = !scrolling_bank;

            if (!scrolling_bank)
                LCD_hide_cursor(LCDC_BOTH);
        }
    }
    else  // Normal encoder rotation    
    {
        if (scrolling_bank)
            cur_save_pos += vdelta * 64;
        else
            cur_save_pos += vdelta;

        if (cur_save_pos < 0)
            cur_save_pos = 0;
        else if (cur_save_pos > max_pos)
            cur_save_pos = max_pos;
    }
}

/* =======================================================================
Handler for 'Confirm saving' field.
----------------------------------------------------------------------- */

void ui_confirm_fhandler( FVALUE_T *fval, BYTE offs, INT8 vdelta ) __reentrant
{
    offs;

    //  // Value encoder press: confirmation
    if (vdelta == VAL_PRESSED)
    {
        if (confirm_save == 1)  // accepted only when 'Yes' in the field
        {
            strcpy(old_save_name,cur_save_name);
            saving_from_ui = TRUE;

            switch (cur_mode)
            {
            case MODE_SOUND:
                saving_state = SAVE_PATCH;
                strcpy(patch.name,cur_save_name);
                cur_patch = cur_save_pos;
                break;

            case MODE_DRUMS:
                saving_state = SAVE_DRUMKIT;
                strcpy(drumkit.name,cur_save_name);
                cur_drumkit = cur_save_pos;
                break;

            case MODE_SETTINGS:
                saving_state = SAVE_SETTINGS;
                break;
            }
        }
    }
    else  // Normal encoder rotation (toggle between 'Yes' and 'No')
    {
        confirm_save += vdelta;

        if (confirm_save > UNDERFLOW)    // assume min=0 always
            confirm_save = 0;
        else if (confirm_save > fval->max)
            confirm_save = fval->max;

        LCD_field_text(fval->r,fval->c,fval->clist[confirm_save]);
    }
}

/* =======================================================================
Show current edit mode on header row
----------------------------------------------------------------------- */

void show_edit_mode( void )
{
    switch (edit_mode)
    {
    case MODE_ORIGINAL: LCD_write_text(0,24,"ORIGINAL"); break;
    case MODE_EDITED:   LCD_write_text(0,24,"EDITED  "); break;
    case MODE_COMPARE:  LCD_write_text(0,24,"COMPARE "); break;
    }
}

/* =================================================================== */
/* =======================================================================
- Button task -
Finds out which button has been pressed.
Out: button_pressed = number of pressed button
Buttons: 
  Encoder Page/Field = P0.4
  Encoder Value      = P0.5
  Edit/Compare       = P1.0
----------------------------------------------------------------------- */

#define IDLE            0
#define EDIT_IS_DOWN    1
#define WAIT_ALL_UP     2

#define LONG_PRESS_TIME (1000/RTC_TIME)   // 1 sec

void button_task( void )
{
    CODE BYTE button_lut[8] = {
                           // VPE
        BUTTON_NONE,       // 000
        BUTTON_EDIT,       // 001
        BUTTON_PF_PRESS,   // 010
        BUTTON_IGNORE,     // 011
        BUTTON_VAL_PRESS,  // 100
        BUTTON_IGNORE,     // 101
        BUTTON_IGNORE,     // 110
        BUTTON_IGNORE,     // 111
    };

    static PDATA BYTE but_filt_ctr = 4;
    static PDATA BYTE but_states = 0;
    static PDATA BYTE state = WAIT_ALL_UP;
    static PDATA BYTE edit_timer = 0;
    static BYTE act_but_nr = BUTTON_NONE;
    BYTE cur_but;

    // Run only at 5 ms intervals
    if (!tick) return;

    // --- Filter the buttons

    // Require button to be active 4 times (20 ms)
    cur_but = ~(((P0 >> 3) & 0x06) | (P1 & 0x01));     // buttons active low
    but_states &= cur_but;
    but_filt_ctr--;
    if (but_filt_ctr == 0)
    {
        // Convert button states to button number using lookup
        act_but_nr = button_lut[but_states & 0x07];

        but_states = cur_but;
        but_filt_ctr = 4;
    }

    // --- Handle the button states

    switch (state)
    {
    case IDLE:
        // Wait for any button down
        if (act_but_nr != BUTTON_NONE && act_but_nr != BUTTON_IGNORE)
        {
            if (act_but_nr == BUTTON_EDIT)
            {
                edit_timer = LONG_PRESS_TIME;
                state = EDIT_IS_DOWN;
            }
            else
            {
                button_pressed = act_but_nr;   // --> ui_task
                state = WAIT_ALL_UP;
            }
        }
        break;

    case EDIT_IS_DOWN:
        // Count time while EDIT button is down
        edit_timer--;
        if (edit_timer == 0)
        {
            button_pressed = BUTTON_EDIT_LONG;   // --> ui_task
            state = WAIT_ALL_UP;
        }
        else if (act_but_nr != BUTTON_EDIT)
        {
            button_pressed = BUTTON_EDIT_SHORT;   // --> ui_task
            state = WAIT_ALL_UP;
        }
        break;

    case WAIT_ALL_UP:
        if (act_but_nr == BUTTON_NONE)
            state = IDLE;
        break;
    }
}

/* =======================================================================
INT0-interrupt, caused by falling edge of page/field encoder channel A.
Increment or decrement enc_pf_ctr.
----------------------------------------------------------------------- */

void enc_pf_a_isr(void) __interrupt(INTERRUPT_INT0) __using (1)
{
    // Make sure A is still down
    if (ENC_PF_A == 0)
    {
        // If channel B up, encoder is rotating to + direction,
        // otherwise to - direction
        if (ENC_PF_B == 1)
            enc_pf_ctr++;
        else
            enc_pf_ctr--;
    }

    // flag to main task
    enc_pf_changed = TRUE;
}

/* =======================================================================
INT1-interrupt, caused by falling edge of value encoder channel A.
Increment or decrement enc_val_ctr.
----------------------------------------------------------------------- */

void enc_val_a_isr(void) __interrupt(INTERRUPT_INT1) __using (1)
{
    // Make sure A is still down
    if (ENC_VAL_A == 0)
    {
        // If channel B up, encoder is rotating to + direction,
        // otherwise to - direction
        if (ENC_VAL_B == 1)
            enc_val_ctr++;
        else
            enc_val_ctr--;
    }

    // flag to main task
    enc_val_changed = TRUE;
}

/* ============================ EOF ====================================== */
