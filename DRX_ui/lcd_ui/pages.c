/***************************************************************************

 pages.c

 LCD/encoder based UI driver for DRX-70. Display page definitions.

 16.5.2021  - First version

 ***************************************************************************/

// ***************** Sound edit pages *******************

// ================ Page 1 ==================
// Basic params

// --- Fields

// Field: algorithm select
CODE FLABEL_T lbl_alg = { 
    .caption = "Alg:", .r = 1, .c = 0  
};
CODE FVALUE_T val_alg = {
    .r = 1, .c = 6, .w = 1,
    .min = 1, .max = 8,
    .handler = &ui_alg_fhandler,
    .target = &patch.algorithm,
    .scmd = SCMD_SET_ALGORITHM,
    .clist = { NULL }
};

// Field: feedback level
CODE FLABEL_T lbl_feedb = { 
    .caption = "Feedb:", .r = 2, .c = 0  
};
CODE FVALUE_T val_feedb = {   
    .r = 2, .c = 6, .w = 1,
    .min = 0, .max = 7,
    .handler = &ui_int_fhandler,
    .target = &patch.feedback,
    .scmd = SCMD_SET_FEEDBACK,
    .clist = { NULL }
};

// Field: output channel
CODE FLABEL_T lbl_outchan = { 
    .caption = "OutChan:", .r = 1, .c = 12 
};
CODE FVALUE_T val_outchan = {   
    .r = 1, .c = 20, .w = 6,
    .min = 0, .max = 3,
    .handler = &ui_choice_fhandler,
    .target = &patch.outchan,
    .scmd = SCMD_SET_OUTCHAN,
    .clist = { "none  ", "direct", "chorus", "both  "}
};

// Field: echo
CODE FLABEL_T lbl_echo = { 
    .caption = "Echo:", .r = 2, .c = 12 
};
CODE FVALUE_T val_echo = {   
    .r = 2, .c = 20, .w = 3,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = &patch.echo,
    .scmd = SCMD_SET_ECHO,
    .clist = { "off", "on "}
};

// Fields: Operator enable * 4
CODE FLABEL_T lbl_op_en = { 
    .caption = "Ops on:", .r = 3, .c = 0 
};
CODE FVALUE_T val_op_en1 = {   
    .r = 3, .c = 8, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.op_en,
    .scmd = SCMD_SET_OP_EN,
    .clist = { "-", "1"}
};
CODE FVALUE_T val_op_en2 = {
    .r = 3, .c = 11, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.op_en,
    .scmd = SCMD_SET_OP_EN,
    .clist = { "-", "2"}
};
CODE FVALUE_T val_op_en3 = {
    .r = 3, .c = 14, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.op_en,
    .scmd = SCMD_SET_OP_EN,
    .clist = { "-", "3"}
};
CODE FVALUE_T val_op_en4 = {
    .r = 3, .c = 17, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.op_en,
    .scmd = SCMD_SET_OP_EN,
    .clist = { "-", "4"}
};

// --- Page

CODE PAGE_T page_sound_basic = {
    .caption = "Basic params",
    .field_cnt = 8,
    .fields = {
        { .label = &lbl_alg,     .value = &val_alg,     .op_inst = 0 },
        { .label = &lbl_feedb,   .value = &val_feedb,   .op_inst = 0 },
        { .label = &lbl_outchan, .value = &val_outchan, .op_inst = 0 },
        { .label = &lbl_echo,    .value = &val_echo,    .op_inst = 0 },
        { .label = &lbl_op_en,   .value = &val_op_en1,  .op_inst = 3 },
        { .label = NULL,         .value = &val_op_en2,  .op_inst = 2 },
        { .label = NULL,         .value = &val_op_en3,  .op_inst = 1 },
        { .label = NULL,         .value = &val_op_en4,  .op_inst = 0 },
    }
};

// ================ Page 2 ==================
// LFO params

// --- Fields

// Field: LFO control
CODE FLABEL_T lbl_lfo_control = { 
    .caption = "Freq:", .r = 1, .c = 0  
};
CODE FVALUE_T val_lfo_control = {   
    .r = 1, .c = 5, .w = 3,
    .min = 0, .max = 8,
    .handler = &ui_choice_fhandler,
    .target = &patch.lfo_control,
    .scmd = SCMD_SET_LFO_CONTROL,
    .clist = { "  4", "  5", "  6", "  7", "  8", "  9", " 10", " 47", "off" }
};

// Field: LFO FM start delay
CODE FLABEL_T lbl_lfo_delay = { 
    .caption = "FM delay:", .r = 1, .c = 12  
};
CODE FVALUE_T val_lfo_delay = {   
    .r = 1, .c = 21, .w = 1,
    .min = 0, .max = 7,
    .handler = &ui_int_fhandler,
    .target = &patch.lfo_fm_delay,
    .scmd = SCMD_SET_LFO_FM_DELAY,
    .clist = { NULL }
};

// Field: LFO FM rise rate
CODE FLABEL_T lbl_lfo_rate = { 
    .caption = "FM rate:", .r = 2, .c = 12  
};
CODE FVALUE_T val_lfo_rate = {   
    .r = 2, .c = 21, .w = 1,
    .min = 0, .max = 7,
    .handler = &ui_int_fhandler,
    .target = &patch.lfo_fm_rate,
    .scmd = SCMD_SET_LFO_FM_RATE,
    .clist = { NULL }
};

// Field: LFO FM (vibrato) depth
CODE FLABEL_T lbl_lfo_fm_depth = { 
    .caption = "FM depth:", .r = 1, .c = 27
};
CODE FVALUE_T val_lfo_fm_depth = {   
    .r = 1, .c = 36, .w = 1,
    .min = 0, .max = 7,
    .handler = &ui_int_fhandler,
    .target = &patch.lfo_fm_depth,
    .scmd = SCMD_SET_LFO_FM_DEPTH,
    .clist = { NULL }
};

// Field: LFO AM (tremolo) depth
CODE FLABEL_T lbl_lfo_am_depth = { 
    .caption = "AM depth:", .r = 2, .c = 27  
};
CODE FVALUE_T val_lfo_am_depth = {   
    .r = 2, .c = 36, .w = 1,
    .min = 0, .max = 3,
    .handler = &ui_int_fhandler,
    .target = &patch.lfo_am_depth,
    .scmd = SCMD_SET_LFO_AM_DEPTH,
    .clist = { NULL }
};

// Field: LFO AM enable for operator * 4
CODE FLABEL_T lbl_lfo_am_en = { 
    .caption = "Op AM enable:", .r = 3, .c = 0  
};
CODE FVALUE_T val_lfo_am_en1 = {   
    .r = 3, .c = 13, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.lfo_am_en,
    .scmd = SCMD_SET_LFO_AM_EN,
    .clist = { "-", "1"}
};
CODE FVALUE_T val_lfo_am_en2 = {
    .r = 3, .c = 16, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.lfo_am_en,
    .scmd = SCMD_SET_LFO_AM_EN,
    .clist = { "-", "2"}
};
CODE FVALUE_T val_lfo_am_en3 = {
    .r = 3, .c = 19, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.lfo_am_en,
    .scmd = SCMD_SET_LFO_AM_EN,
    .clist = { "-", "3"}
};
CODE FVALUE_T val_lfo_am_en4 = {
    .r = 3, .c = 22, .w = 1,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = patch.lfo_am_en,
    .scmd = SCMD_SET_LFO_AM_EN,
    .clist = { "-", "4"}
};

// --- Page

CODE PAGE_T page_sound_lfo = {
    .caption = "LFO params",
    .field_cnt = 9,
    .fields = {
        { .label = &lbl_lfo_control,  .value = &val_lfo_control,  .op_inst = 0 },
        { .label = &lbl_lfo_delay,    .value = &val_lfo_delay,    .op_inst = 0 },
        { .label = &lbl_lfo_rate,     .value = &val_lfo_rate,     .op_inst = 0 },
        { .label = &lbl_lfo_fm_depth, .value = &val_lfo_fm_depth, .op_inst = 0 },
        { .label = &lbl_lfo_am_depth, .value = &val_lfo_am_depth, .op_inst = 0 },
        { .label = &lbl_lfo_am_en,    .value = &val_lfo_am_en1,   .op_inst = 3 },
        { .label = NULL,              .value = &val_lfo_am_en2,   .op_inst = 2 },
        { .label = NULL,              .value = &val_lfo_am_en3,   .op_inst = 1 },
        { .label = NULL,              .value = &val_lfo_am_en4,   .op_inst = 0 },
    }
};

// ================ Pages 3...6 ==================
// Operator specific general parameters

// --- Fields

// Field: Multiplier
CODE FLABEL_T lbl_multip = { 
    .caption = "Multiplier:", .r = 1, .c = 0  
};
CODE FVALUE_T val_multip = {   
    .r = 1, .c = 12, .w = 3,
    .min = 0, .max = 15,
    .handler = &ui_choice_fhandler,
    .target = &patch.factor,
    .scmd = SCMD_SET_FACTOR,
    .clist = { "0.5", "  1", "  2", "  3", "  4", "  5", "  6", "  7", "  8", "  9", " 10", " 12", " 15", " 16", " 17", " 18" }
};

// Field: Waveform
CODE FLABEL_T lbl_waveform = { 
    .caption = "Waveform:", .r = 2, .c = 0  
};
CODE FVALUE_T val_waveform = {   
    .r = 2, .c = 12, .w = 4,
    .min = 0, .max = 1,
    .handler = &ui_choice_fhandler,
    .target = &patch.waveform,
    .scmd = SCMD_SET_WAVEFORM,
    .clist = { "Sin ", "Hsin" }
};

// Field: Velocity Scaling for Level curve select
CODE FLABEL_T lbl_vsl_sel = { 
    .caption = "Vel sens:", .r = 3, .c = 0  
};
CODE FVALUE_T val_vsl_sel = {   
    .r = 3, .c = 12, .w = 2,
    .min = 0, .max = 7,
    .handler = &ui_int_fhandler,
    .target = &patch.vsl_sel,
    .scmd = SCMD_SET_VSL_SEL,
    .clist = { NULL }
};

// Field: Key Scaling for Envelope
CODE FLABEL_T lbl_kse = { 
    .caption = "Key sens envel:", .r = 1, .c = 20  
};
CODE FVALUE_T val_kse = {   
    .r = 1, .c = 37, .w = 1,
    .min = 0, .max = 3,
    .handler = &ui_int_fhandler,
    .target = &patch.kse,
    .scmd = SCMD_SET_KSE,
    .clist = { NULL }
};

// Field: Key Scaling for Level, upper half, curve select
CODE FLABEL_T lbl_ksl_hi_sel = { 
    .caption = "Key sens lvl U:", .r = 2, .c = 20  
};
CODE FVALUE_T val_ksl_hi_sel = {   
    .r = 2, .c = 36, .w = 2,
    .min = 0, .max = 21,
    .handler = &ui_int_fhandler,
    .target = &patch.ksl_hi_sel,
    .scmd = SCMD_SET_KSL_HI_SEL,
    .clist = { NULL }
};

// Field: Key Scaling for Level, lower half, curve select
CODE FLABEL_T lbl_ksl_lo_sel = { 
    .caption = "Key sens lvl L:", .r = 3, .c = 20  
};
CODE FVALUE_T val_ksl_lo_sel = {   
    .r = 3, .c = 36, .w = 2,
    .min = 0, .max = 12,
    .handler = &ui_int_fhandler,
    .target = &patch.ksl_lo_sel,
    .scmd = SCMD_SET_KSL_LO_SEL,
    .clist = { NULL }
};

// --- Page 3: Op 1

CODE PAGE_T page_sound_op_par1 = {
    .caption = "Op 1($) params",
    .field_cnt = 6,
    .fields = {
        { .label = &lbl_multip,     .value = &val_multip,     .op_inst = 3 },
        { .label = &lbl_waveform,   .value = &val_waveform,   .op_inst = 3 },
        { .label = &lbl_vsl_sel,    .value = &val_vsl_sel,    .op_inst = 3 },
        { .label = &lbl_kse,        .value = &val_kse,        .op_inst = 3 },
        { .label = &lbl_ksl_hi_sel, .value = &val_ksl_hi_sel, .op_inst = 3 },
        { .label = &lbl_ksl_lo_sel, .value = &val_ksl_lo_sel, .op_inst = 3 },
    }
};

// --- Page 4: Op 2

CODE PAGE_T page_sound_op_par2 = {
    .caption = "Op 2($) params",
    .field_cnt = 6,
    .fields = {
        { .label = &lbl_multip,     .value = &val_multip,     .op_inst = 2 },
        { .label = &lbl_waveform,   .value = &val_waveform,   .op_inst = 2 },
        { .label = &lbl_vsl_sel,    .value = &val_vsl_sel,    .op_inst = 2 },
        { .label = &lbl_kse,        .value = &val_kse,        .op_inst = 2 },
        { .label = &lbl_ksl_hi_sel, .value = &val_ksl_hi_sel, .op_inst = 2 },
        { .label = &lbl_ksl_lo_sel, .value = &val_ksl_lo_sel, .op_inst = 2 },
    }
};

// --- Page 5: Op 3

CODE PAGE_T page_sound_op_par3 = {
    .caption = "Op 3($) params",
    .field_cnt = 6,
    .fields = {
        { .label = &lbl_multip,     .value = &val_multip,     .op_inst = 1 },
        { .label = &lbl_waveform,   .value = &val_waveform,   .op_inst = 1 },
        { .label = &lbl_vsl_sel,    .value = &val_vsl_sel,    .op_inst = 1 },
        { .label = &lbl_kse,        .value = &val_kse,        .op_inst = 1 },
        { .label = &lbl_ksl_hi_sel, .value = &val_ksl_hi_sel, .op_inst = 1 },
        { .label = &lbl_ksl_lo_sel, .value = &val_ksl_lo_sel, .op_inst = 1 },
    }
};

// --- Page 6: Op 4

CODE PAGE_T page_sound_op_par4 = {
    .caption = "Op 4($) params",
    .field_cnt = 6,
    .fields = {
        { .label = &lbl_multip,     .value = &val_multip,     .op_inst = 0 },
        { .label = &lbl_waveform,   .value = &val_waveform,   .op_inst = 0 },
        { .label = &lbl_vsl_sel,    .value = &val_vsl_sel,    .op_inst = 0 },
        { .label = &lbl_kse,        .value = &val_kse,        .op_inst = 0 },
        { .label = &lbl_ksl_hi_sel, .value = &val_ksl_hi_sel, .op_inst = 0 },
        { .label = &lbl_ksl_lo_sel, .value = &val_ksl_lo_sel, .op_inst = 0 },
    }
};

// ================ Pages 7...10 ==================
// Operator specific envelope parameters

// --- Fields

// Field: Level
CODE FLABEL_T lbl_level = { 
    .caption = "Level:", .r = 1, .c = 0  
};
CODE FVALUE_T val_level = {   
    .r = 1, .c = 8, .w = 2,
    .min = 0, .max = 99,
    .handler = &ui_int_fhandler,
    .target = &patch.level,
    .scmd = SCMD_SET_LEVEL,
    .clist = { NULL }
};

// Field: Detune
CODE FLABEL_T lbl_detune = { 
    .caption = "Detune:", .r = 2, .c = 0  
};
CODE FVALUE_T val_detune = {   
    .r = 2, .c = 7, .w = 3,     // w=3 causes special handling in ui_int_fhandler() !
    .min = 1, .max = 63,        // min=1 => -31...+31
    .handler = &ui_int_fhandler,
    .target = &patch.detune,
    .scmd = SCMD_SET_DETUNE,
    .clist = { NULL }
};

// Field: Attack
CODE FLABEL_T lbl_attack = { 
    .caption = "AttR:", .r = 1, .c = 14  
};
CODE FVALUE_T val_attack = {   
    .r = 1, .c = 21, .w = 2,
    .min = 0, .max = 31,
    .handler = &ui_int_fhandler,
    .target = &patch.attack,
    .scmd = SCMD_SET_ATTACK,
    .clist = { NULL }
};

// Field: Decay
CODE FLABEL_T lbl_decay = { 
    .caption = "DecR:", .r = 2, .c = 14
};
CODE FVALUE_T val_decay = {   
    .r = 2, .c = 21, .w = 2,
    .min = 0, .max = 31,
    .handler = &ui_int_fhandler,
    .target = &patch.decay,
    .scmd = SCMD_SET_DECAY,
    .clist = { NULL }
};

// Field: Sustain
CODE FLABEL_T lbl_sustain = { 
    .caption = "SusR:", .r = 3, .c = 14
};
CODE FVALUE_T val_sustain = {   
    .r = 3, .c = 21, .w = 2,
    .min = 0, .max = 31,
    .handler = &ui_int_fhandler,
    .target = &patch.sustain,
    .scmd = SCMD_SET_SUSTAIN,
    .clist = { NULL }
};

// Field: Sustain level
CODE FLABEL_T lbl_sus_level = { 
    .caption = "SusL:", .r = 1, .c = 28
};
CODE FVALUE_T val_sus_level = {   
    .r = 1, .c = 34, .w = 2,
    .min = 0, .max = 15,
    .handler = &ui_int_fhandler,
    .target = &patch.sus_level,
    .scmd = SCMD_SET_SUS_LEVEL,
    .clist = { NULL }
};

// Field: Release
CODE FLABEL_T lbl_release = { 
    .caption = "RelR:", .r = 2, .c = 28
};
CODE FVALUE_T val_release = {   
    .r = 2, .c = 34, .w = 2,
    .min = 0, .max = 15,
    .handler = &ui_int_fhandler,
    .target = &patch.release,
    .scmd = SCMD_SET_RELEASE,
    .clist = { NULL }
};

// --- Page 7: Op 1

CODE PAGE_T page_sound_op_env1 = {
    .caption = "Op 1($) envelope",
    .field_cnt = 7,
    .fields = {
        { .label = &lbl_level,      .value = &val_level,      .op_inst = 3 },
        { .label = &lbl_detune,     .value = &val_detune,     .op_inst = 3 },
        { .label = &lbl_attack,     .value = &val_attack,     .op_inst = 3 },
        { .label = &lbl_decay,      .value = &val_decay,      .op_inst = 3 },
        { .label = &lbl_sustain,    .value = &val_sustain,    .op_inst = 3 },
        { .label = &lbl_sus_level,  .value = &val_sus_level,  .op_inst = 3 },
        { .label = &lbl_release,    .value = &val_release,    .op_inst = 3 },
    }
};

// --- Page 8: Op 2

CODE PAGE_T page_sound_op_env2 = {
    .caption = "Op 2($) envelope",
    .field_cnt = 7,
    .fields = {
        { .label = &lbl_level,      .value = &val_level,      .op_inst = 2 },
        { .label = &lbl_detune,     .value = &val_detune,     .op_inst = 2 },
        { .label = &lbl_attack,     .value = &val_attack,     .op_inst = 2 },
        { .label = &lbl_decay,      .value = &val_decay,      .op_inst = 2 },
        { .label = &lbl_sustain,    .value = &val_sustain,    .op_inst = 2 },
        { .label = &lbl_sus_level,  .value = &val_sus_level,  .op_inst = 2 },
        { .label = &lbl_release,    .value = &val_release,    .op_inst = 2 },
    }
};

// --- Page 9: Op 3

CODE PAGE_T page_sound_op_env3 = {
    .caption = "Op 3($) envelope",
    .field_cnt = 7,
    .fields = {
        { .label = &lbl_level,      .value = &val_level,      .op_inst = 1 },
        { .label = &lbl_detune,     .value = &val_detune,     .op_inst = 1 },
        { .label = &lbl_attack,     .value = &val_attack,     .op_inst = 1 },
        { .label = &lbl_decay,      .value = &val_decay,      .op_inst = 1 },
        { .label = &lbl_sustain,    .value = &val_sustain,    .op_inst = 1 },
        { .label = &lbl_sus_level,  .value = &val_sus_level,  .op_inst = 1 },
        { .label = &lbl_release,    .value = &val_release,    .op_inst = 1 },
    }
};

// --- Page 10: Op 4

CODE PAGE_T page_sound_op_env4 = {
    .caption = "Op 4($) envelope",
    .field_cnt = 7,
    .fields = {
        { .label = &lbl_level,      .value = &val_level,      .op_inst = 0 },
        { .label = &lbl_detune,     .value = &val_detune,     .op_inst = 0 },
        { .label = &lbl_attack,     .value = &val_attack,     .op_inst = 0 },
        { .label = &lbl_decay,      .value = &val_decay,      .op_inst = 0 },
        { .label = &lbl_sustain,    .value = &val_sustain,    .op_inst = 0 },
        { .label = &lbl_sus_level,  .value = &val_sus_level,  .op_inst = 0 },
        { .label = &lbl_release,    .value = &val_release,    .op_inst = 0 },
    }
};

// ================ Page 11 ==================

// --- Fields

// Field: New name for saved sound/drum kit
CODE FLABEL_T lbl_save_name = { 
    .caption = "Name:", .r = 1, .c = 0  
};
CODE FVALUE_T val_save_name = {   
    .r = 1, .c = 15, .w = 10,
    .min = 0, .max = 9,
    .handler = &ui_name_fhandler,
    .target = NULL,
    .clist = { NULL }
};

// Field: Position to save
CODE FLABEL_T lbl_save_pos = { 
    .caption = "Position:", .r = 2, .c = 0  
};
CODE FVALUE_T val_save_pos = {   
    .r = 2, .c = 15, .w = 3,
    .min = 0, .max = 1,
    .handler = &ui_savepos_fhandler,
    .target = NULL,
    .clist = { NULL }
};

// Field: Confirm saving
CODE FLABEL_T lbl_save_conf = { 
    .caption = "Confirm saving:", .r = 3, .c = 0  
};
CODE FVALUE_T val_save_conf = {   
    .r = 3, .c = 15, .w = 3,
    .min = 0, .max = 1,
    .handler = &ui_confirm_fhandler,
    .target = NULL,
    .clist = { "No ", "Yes" }
};

// --- Page

CODE PAGE_T page_sound_save = {
    .caption = "Save sound",
    .field_cnt = 3,
    .fields = {
        { .label = &lbl_save_name, .value = &val_save_name, .op_inst = 0 },
        { .label = &lbl_save_pos,  .value = &val_save_pos,  .op_inst = 0 },
        { .label = &lbl_save_conf, .value = &val_save_conf, .op_inst = 0 },
    }
};

// --- Sound edit page list

CODE PAGE_T * CODE sound_pages[] = {
    NULL,
    &page_sound_basic,            // Pg 1
    &page_sound_lfo,              // Pg 2
    &page_sound_op_par1,          // Pg 3
    &page_sound_op_par2,          // Pg 4
    &page_sound_op_par3,          // Pg 5
    &page_sound_op_par4,          // Pg 6
    &page_sound_op_env1,          // Pg 7
    &page_sound_op_env2,          // Pg 8
    &page_sound_op_env3,          // Pg 9
    &page_sound_op_env4,          // Pg 10
    &page_sound_save,             // Pg 11
    NULL
};

#define SOUND_PAGE_CNT  11


// *****************************************************
// ***************** Drum edit pages *******************

// --- Fields

// - Instrument name labels, no corresponding value field
// Page 1
CODE FLABEL_T lbl_BD = { 
    .caption = "BD     >", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_Rimshot = { 
    .caption = "Rimshot>", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_SD_Lo = { 
    .caption = "SD Lo  >", .r = 3, .c = 0  
};

// Page 2
CODE FLABEL_T lbl_Claps = { 
    .caption = "Claps  >", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_SD_Hi = { 
    .caption = "SD Hi  >", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_Tom_3 = { 
    .caption = "Tom 3  >", .r = 3, .c = 0  
};

// Page 3
CODE FLABEL_T lbl_HH_cl = { 
    .caption = "HH cl  >", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_Tom_2 = { 
    .caption = "Tom 2  >", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_HH_op = { 
    .caption = "HH op  >", .r = 3, .c = 0  
};

// Page 4
CODE FLABEL_T lbl_Tom_1 = { 
    .caption = "Tom 1  >", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_Ride = { 
    .caption = "Ride   >", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_Bongo_1 = { 
    .caption = "Bongo1 >", .r = 3, .c = 0  
};

// Page 5
CODE FLABEL_T lbl_Bongo_2 = { 
    .caption = "Bongo2 >", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_Bongo_3 = { 
    .caption = "Bongo3 >", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_Timb_Hi = { 
    .caption = "Timb Hi>", .r = 3, .c = 0  
};

// Page 6
CODE FLABEL_T lbl_Timb_Lo = { 
    .caption = "Timb Lo>", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_AgogoHi = { 
    .caption = "AgogoHi>", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_AgogoLo = { 
    .caption = "AgogoLo>", .r = 3, .c = 0  
};

// Page 7
CODE FLABEL_T lbl_Claves = { 
    .caption = "Claves >", .r = 1, .c = 0  
};
CODE FLABEL_T lbl_CuicaHi = { 
    .caption = "CuicaHi>", .r = 2, .c = 0  
};
CODE FLABEL_T lbl_CuicaLo = { 
    .caption = "CuicaLo>", .r = 3, .c = 0  
};

// - Edit fields, common to all pages

// Pan
CODE FLABEL_T lbl_pan1 = { 
    .caption = "Pan:", .r = 1, .c = 9  
};
CODE FVALUE_T val_pan1 = {
    .r = 1, .c = 13, .w = 2,
    .min = 0, .max = 15,
    .handler = &ui_choice_fhandler,
    .target = &drumkit.pan,
    .scmd = SCMD_SET_PAN,
    .clist = { "M ","L7","L6","L5","L4","L3","L2","L1","C ","R1","R2","R3","R4","R5","R6","R7" }
};

CODE FLABEL_T lbl_pan2 = { 
    .caption = "Pan:", .r = 2, .c = 9  
};
CODE FVALUE_T val_pan2 = {
    .r = 2, .c = 13, .w = 2,
    .min = 0, .max = 15,
    .handler = &ui_choice_fhandler,
    .target = &drumkit.pan,
    .scmd = SCMD_SET_PAN,
    .clist = { "M ","L7","L6","L5","L4","L3","L2","L1","C ","R1","R2","R3","R4","R5","R6","R7" }
};

CODE FLABEL_T lbl_pan3 = { 
    .caption = "Pan:", .r = 3, .c = 9  
};
CODE FVALUE_T val_pan3 = {
    .r = 3, .c = 13, .w = 2,
    .min = 0, .max = 15,
    .handler = &ui_choice_fhandler,
    .target = &drumkit.pan,
    .scmd = SCMD_SET_PAN,
    .clist = { "M ","L7","L6","L5","L4","L3","L2","L1","C ","R1","R2","R3","R4","R5","R6","R7" }
};

// Sample start
CODE FLABEL_T lbl_s_start1 = { 
    .caption = "S st:", .r = 1, .c = 18  
};
CODE FVALUE_T val_s_start1 = {
    .r = 1, .c = 23, .w = 3,
    .min = 0, .max = 2,
    .handler = &ui_hex_fhandler,
    .target = &drumkit.s_start,
    .scmd = SCMD_SET_S_START,
    .clist = { NULL }
};

CODE FLABEL_T lbl_s_start2 = { 
    .caption = "S st:", .r = 2, .c = 18  
};
CODE FVALUE_T val_s_start2 = {
    .r = 2, .c = 23, .w = 3,
    .min = 0, .max = 2,
    .handler = &ui_hex_fhandler,
    .target = &drumkit.s_start,
    .scmd = SCMD_SET_S_START,
    .clist = { NULL }
};

CODE FLABEL_T lbl_s_start3 = { 
    .caption = "S st:", .r = 3, .c = 18  
};
CODE FVALUE_T val_s_start3 = {
    .r = 3, .c = 23, .w = 3,
    .min = 0, .max = 2,
    .handler = &ui_hex_fhandler,
    .target = &drumkit.s_start,
    .scmd = SCMD_SET_S_START,
    .clist = { NULL }
};

// Sample end
CODE FLABEL_T lbl_s_end1 = { 
    .caption = "S end:", .r = 1, .c = 29  
};
CODE FVALUE_T val_s_end1 = {
    .r = 1, .c = 35, .w = 3,
    .min = 0, .max = 2,
    .handler = &ui_hex_fhandler,
    .target = &drumkit.s_end,
    .scmd = SCMD_SET_S_END,
    .clist = { NULL }
};

CODE FLABEL_T lbl_s_end2 = { 
    .caption = "S end:", .r = 2, .c = 29  
};
CODE FVALUE_T val_s_end2 = {
    .r = 2, .c = 35, .w = 3,
    .min = 0, .max = 2,
    .handler = &ui_hex_fhandler,
    .target = &drumkit.s_end,
    .scmd = SCMD_SET_S_END,
    .clist = { NULL }
};

CODE FLABEL_T lbl_s_end3 = { 
    .caption = "S end:", .r = 3, .c = 29  
};
CODE FVALUE_T val_s_end3 = {
    .r = 3, .c = 35, .w = 3,
    .min = 0, .max = 2,
    .handler = &ui_hex_fhandler,
    .target = &drumkit.s_end,
    .scmd = SCMD_SET_S_END,
    .clist = { NULL }
};


// --- Pages

CODE PAGE_T page_drum1 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst = 0  },   // BD
        { .label = &lbl_BD,        .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst = 0  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst = 0  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst = 1  },   // Rimshot
        { .label = &lbl_Rimshot,   .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst = 1  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst = 1  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst = 2  },   // SD Lo
        { .label = &lbl_SD_Lo,     .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst = 2  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst = 2  },
    }
};

CODE PAGE_T page_drum2 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst = 3  },   // Claps
        { .label = &lbl_Claps,     .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst = 3  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst = 3  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst = 4  },   // SD Hi
        { .label = &lbl_SD_Hi,     .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst = 4  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst = 4  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst = 5  },   // Tom 3
        { .label = &lbl_Tom_3,     .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst = 5  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst = 5  },
    }
};


CODE PAGE_T page_drum3 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst = 6  },   // HH cl
        { .label = &lbl_HH_cl,     .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst = 6  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst = 6  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst = 7  },   // Tom 2
        { .label = &lbl_Tom_2,     .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst = 7  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst = 7  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst = 8  },   // HH op
        { .label = &lbl_HH_op,     .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst = 8  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst = 8  },
    }
};

CODE PAGE_T page_drum4 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst = 9  },   // Tom 1
        { .label = &lbl_Tom_1,     .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst = 9  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst = 9  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst =10  },   // Ride
        { .label = &lbl_Ride,      .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst =10  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst =10  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst =11  },   // Bongo 1
        { .label = &lbl_Bongo_1,   .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst =11  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst =11  },
    }
};

CODE PAGE_T page_drum5 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst =12  },   // Bongo 2
        { .label = &lbl_Bongo_2,   .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst =12  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst =12  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst =13  },   // Bongo 3
        { .label = &lbl_Bongo_3,   .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst =13  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst =13  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst =14  },   // Timbale Hi
        { .label = &lbl_Timb_Hi,   .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst =14  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst =14  },
    }
};

CODE PAGE_T page_drum6 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst =15  },   // Timbale Lo
        { .label = &lbl_Timb_Lo,   .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst =15  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst =15  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst =16  },   // Agogo Hi
        { .label = &lbl_AgogoHi,   .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst =16  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst =16  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst =17  },   // Agogo Lo
        { .label = &lbl_AgogoLo,   .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst =17  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst =17  },
    }
};

CODE PAGE_T page_drum7 = {
    .caption = "Drum params",
    .field_cnt = 12,
    .fields = {
        { .label = &lbl_pan1,      .value = &val_pan1,     .op_inst =18  },   // Claves
        { .label = &lbl_Claves,    .value = NULL                         },
        { .label = &lbl_s_start1,  .value = &val_s_start1, .op_inst =18  },
        { .label = &lbl_s_end1,    .value = &val_s_end1,   .op_inst =18  },

        { .label = &lbl_pan2,      .value = &val_pan2,     .op_inst =19  },   // Cuica Hi
        { .label = &lbl_CuicaHi,   .value = NULL                         },
        { .label = &lbl_s_start2,  .value = &val_s_start2, .op_inst =19  },
        { .label = &lbl_s_end2,    .value = &val_s_end2,   .op_inst =19  },

        { .label = &lbl_pan3,      .value = &val_pan3,     .op_inst =20  },   // Cuica Lo
        { .label = &lbl_CuicaLo,   .value = NULL                         },
        { .label = &lbl_s_start3,  .value = &val_s_start3, .op_inst =20  },
        { .label = &lbl_s_end3,    .value = &val_s_end3,   .op_inst =20  },
    }
};


CODE PAGE_T page_drum_save = {
    .caption = "Save drum kit",
    .field_cnt = 3,
    .fields = {
        { .label = &lbl_save_name, .value = &val_save_name, .op_inst = 0 },
        { .label = &lbl_save_pos,  .value = &val_save_pos,  .op_inst = 0 },
        { .label = &lbl_save_conf, .value = &val_save_conf, .op_inst = 0 },
    }
};


// --- Drum edit page list

CODE PAGE_T * CODE drum_pages[] = {
    NULL,
    &page_drum1,                // Pg 1
    &page_drum2,                // Pg 2
    &page_drum3,                // Pg 3
    &page_drum4,                // Pg 4
    &page_drum5,                // Pg 5
    &page_drum6,                // Pg 6
    &page_drum7,                // Pg 7
    &page_drum_save,            // Pg 8
    NULL
};

#define DRUM_PAGE_CNT  8


// *****************************************************
// *************** Settings edit pages *****************

// --- Fields

// Field: Sound channel
CODE FLABEL_T lbl_sound_chan = { 
    .caption = "Sound chan:", .r = 1, .c = 0  
};
CODE FVALUE_T val_sound_chan = {   
    .r = 1, .c = 11, .w = 3,
    .min = 0, .max = 16,
    .handler = &ui_choice_fhandler,
    .target = &settings.sound_ch,
    .scmd = SCMD_SET_SOUND_CH,
    .clist = { "  1", "  2", "  3", "  4", "  5", "  6", "  7", "  8", "  9", " 10", " 11", " 12", " 13", " 14", " 15", " 16", "any" }
};

// Field: Drum channel
CODE FLABEL_T lbl_drum_chan = { 
    .caption = "Drum chan:", .r = 2, .c = 0  
};
CODE FVALUE_T val_drum_chan = {   
    .r = 2, .c = 11, .w = 3,
    .min = 0, .max = 15,
    .handler = &ui_choice_fhandler,
    .target = &settings.drum_ch,
    .scmd = SCMD_SET_DRUM_CH,
    .clist = { "  1", "  2", "  3", "  4", "  5", "  6", "  7", "  8", "  9", " 10", " 11", " 12", " 13", " 14", " 15", " 16" }
};

// Field: Chorus type
CODE FLABEL_T lbl_chorus = { 
    .caption = "Chorus type:", .r = 1, .c = 19  
};
CODE FVALUE_T val_chorus = {   
    .r = 1, .c = 31, .w = 1,
    .min = 0, .max = 4,
    .handler = &ui_int_fhandler,
    .target = &settings.chorus_type,
    .scmd = SCMD_SET_CHORUS_TYPE,
    .clist = { NULL }
};


CODE PAGE_T page_settings = {
    .caption = "General settings",
    .field_cnt = 4,
    .fields = {
        { .label = &lbl_sound_chan, .value = &val_sound_chan, .op_inst = 0 },
        { .label = &lbl_drum_chan,  .value = &val_drum_chan,  .op_inst = 0 },
        { .label = &lbl_chorus,     .value = &val_chorus,     .op_inst = 0 },
        { .label = &lbl_save_conf,  .value = &val_save_conf,  .op_inst = 0 },
    }
};


// --- Settings edit page list

CODE PAGE_T * CODE settings_pages[] = {
    NULL,
    &page_settings,             // Pg 1
    NULL
};

#define SETTINGS_PAGE_CNT  1
