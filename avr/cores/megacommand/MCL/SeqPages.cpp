#include "MCL_impl.h"

MCLEncoder seq_param1(0, 3, ENCODER_RES_SEQ);
MCLEncoder seq_param2(0, 4, ENCODER_RES_SEQ);
MCLEncoder seq_param3(0, 10, ENCODER_RES_SEQ);
MCLEncoder seq_param4(0, 64, ENCODER_RES_SEQ);

MCLEncoder seq_lock1(0, 127, ENCODER_RES_PARAM);
MCLEncoder seq_lock2(0, 127, ENCODER_RES_PARAM);

MCLEncoder ptc_param_oct(0, 8, ENCODER_RES_SEQ);
MCLEncoder ptc_param_finetune(0, 64, ENCODER_RES_SEQ);
MCLEncoder ptc_param_len(0, 64, ENCODER_RES_SEQ);
MCLEncoder ptc_param_scale(0, 23, ENCODER_RES_SEQ);

SeqParamPage seq_param_page[NUM_PARAM_PAGES];
SeqStepPage seq_step_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);

#ifdef EXT_TRACKS
uint8_t last_ext_track;
SeqExtStepPage seq_extstep_page(&seq_param1, &seq_param2, &seq_param3,
                                &seq_param4);
#endif

SeqPtcPage seq_ptc_page(&ptc_param_oct, &ptc_param_finetune, &ptc_param_len, &ptc_param_scale);

ArpPage arp_page(&arp_und, &arp_mode, &arp_speed, &arp_oct);

const menu_t<19> seq_menu_layout PROGMEM = {
    "SEQ",
    {
        {"TRACK SEL:", 1, 17, 0, (uint8_t *)&opt_trackid, (Page *)NULL, opt_trackid_handler, 0},
        {"EDIT:", 0,4, 4, (uint8_t *)&SeqPage::mask_type, (Page *) NULL, opt_mask_handler, 47},
        {"EDIT:", 0, 1 + NUM_LOCKS, 1, (uint8_t *)&SeqPage::pianoroll_mode, (Page *) NULL, NULL, 53},
        {"CC:", 0, 131, 3, (uint8_t *)&SeqPage::param_select, (Page *)NULL, NULL, 54},
        {"SLIDE:", 0, 2, 2, (uint8_t *)&SeqPage::slide, (Page *)NULL, NULL, 24},
        {"ARPEGGIATOR", 0, 0, 0, (uint8_t *)NULL, (Page *) &arp_page, NULL, 0},
        {"TRANSPOSE:", 0, 12, 0, (uint8_t *)&seq_ptc_page.key, (Page *) NULL, NULL, 0},
        {"VEL:", 0, 128, 0, (uint8_t *)&SeqPage::velocity, (Page *)NULL, NULL, 0},
        {"COND:", 1, NUM_TRIG_CONDITIONS, NUM_TRIG_CONDITIONS, (uint8_t *)&SeqPage::cond, (Page *)NULL, NULL, 57},
        {"SPEED:", 0, 7, 7, (uint8_t *)&opt_speed, (Page *)NULL, opt_speed_handler, 40},
        {"LENGTH:", 1, 129, 0, (uint8_t *)&opt_length, (Page *)NULL, opt_length_handler, 0},
        {"CHANNEL:", 1, 17, 0, (uint8_t *)&opt_channel, (Page *)NULL, opt_channel_handler, 0},
        {"COPY:  ", 0, 3, 3, (uint8_t *)&opt_copy, (Page *)NULL, opt_copy_track_handler, 26},
        {"CLEAR:", 0, 3, 3, (uint8_t *)&opt_clear, (Page *)NULL, opt_clear_track_handler, 26},
        {"CLEAR:", 0, 3, 3, (uint8_t *)&opt_clear, (Page *)NULL, opt_clear_locks_handler, 29},
        {"PASTE:", 0, 3, 3, (uint8_t *)&opt_paste, (Page *)NULL, opt_paste_track_handler, 26},
        {"SHIFT:", 0, 5, 5, (uint8_t *)&opt_shift, (Page *)NULL, opt_shift_track_handler, 34},
        {"REVERSE:", 0, 3, 3, (uint8_t *)&opt_reverse, (Page *)NULL, opt_reverse_track_handler, 26},
        {"POLYPHONY", 0, 0 ,0, (uint8_t *)NULL, (Page *) &poly_page, NULL, 0},
    },
    seq_menu_handler,
};

MCLEncoder seq_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder seq_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<19> seq_menu_page(&seq_menu_layout, &seq_menu_value_encoder, &seq_menu_entry_encoder);

const menu_t<4> step_menu_layout PROGMEM = {
    "STP",
    {
        {"CLEAR:", 0, 2, 2, (uint8_t *)&opt_clear_step, (Page *)NULL, opt_clear_step_locks_handler, 29},
        {"COPY STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_copy_step_handler, 0},
        {"PASTE STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_paste_step_handler, 0},
        {"MUTE STEP", 0, 0, 0, (uint8_t *) NULL, (Page *)NULL, opt_mute_step_handler, 0},
    },
    step_menu_handler,
};

MCLEncoder step_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder step_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<4> step_menu_page(&step_menu_layout, &step_menu_value_encoder, &step_menu_entry_encoder);


//SeqLFOPage seq_lfo_page[NUM_LFO_PAGES];
