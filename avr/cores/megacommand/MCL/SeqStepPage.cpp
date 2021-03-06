#include "MCL_impl.h"

#define MIDI_OMNI_MODE 17
#define NUM_KEYS 24

void SeqStepPage::setup() { SeqPage::setup(); }
void SeqStepPage::config() {
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].length;
  tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
  seq_param4.cur = 0;
  seq_param4.old = 0;
  if (tuning) {
    seq_param4.max = tuning->len - 1;
  } else {
    seq_param4.max = 1;
  }
  // config info labels
  const char *str1 = getMDMachineNameShort(MD.kit.get_model(last_md_track), 1);
  const char *str2 = getMDMachineNameShort(MD.kit.get_model(last_md_track), 2);

  constexpr uint8_t len1 = sizeof(info1);

  char buf[len1] = {'\0'};
  m_strncpy_p(buf, str1, len1);
  strncpy(info1, buf, len1);
  strncat(info1, ">", len1);
  m_strncpy_p(buf, str2, len1);
  strncat(info1, buf, len1);

  config_mask_info();
  config_encoders();
  // config menu
  config_as_trackedit();
}

void SeqStepPage::config_encoders() {
  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].length;
  seq_param2.cur = timing_mid;
  seq_param2.old = timing_mid;
  seq_param2.max = timing_mid * 2 - 1;
}

void SeqStepPage::init() {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("init seqstep"));
  SeqPage::init();
  seq_menu_page.menu.enable_entry(SEQ_MENU_MASK, true);
  SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);

  seq_param1.max = NUM_TRIG_CONDITIONS * 2;
  seq_param2.min = 1;
  seq_param2.old = 12;
  seq_param1.cur = 0;
  seq_param3.max = 64;
  seq_param3.min = 1;
  midi_events.setup_callbacks();
  curpage = SEQ_STEP_PAGE;
  trig_interface.on();
  config();
  note_interface.state = true;
}

void SeqStepPage::cleanup() {
  midi_events.remove_callbacks();
  SeqPage::cleanup();
  if (MidiClock.state != 2) {
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
  }
}

void SeqStepPage::display() {
  if (recording && MidiClock.state == 2) {
    if (!redisplay) {
      return;
    }
  }

  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();
  draw_knob_frame();

  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();

  draw_knob_conditional(seq_param1.getValue());
  draw_knob_timing(seq_param2.getValue(), timing_mid);

  char K[4];
  itoa(seq_param3.getValue(), K, 10);
  draw_knob(2, "LEN", K);

  tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
  if (show_pitch) {
    if (tuning != NULL) {
      strcpy(K, "--");
      if (seq_param4.cur != 0) {
        uint8_t base = tuning->base;
        uint8_t notenum = seq_param4.cur + base;
        MusicalNotes number_to_note;
        uint8_t oct = notenum / 12 - 1;
        uint8_t note = notenum - 12 * (notenum / 12);
        strcpy(K, number_to_note.notes_upper[note]);
        K[2] = oct + '0';
        K[3] = 0;
      }
      draw_knob(3, "PTC", K);
    }
  }
  if (mcl_gui.show_encoder_value(&seq_param4) && (seq_param4.cur > 0) &&
      (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
      (!show_step_menu) && (tuning != NULL)) {
    uint64_t note_mask = 0;
    uint8_t note = seq_param4.cur + tuning->base;
    SET_BIT64(note_mask, note - 24 * (note / 24));
    mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, note_mask);
    SeqPage::display();
  }

  else {
    draw_lock_mask((page_select * 16), DEVICE_MD);
    draw_mask((page_select * 16), DEVICE_MD);
    SeqPage::display();
    if (mcl_gui.show_encoder_value(&seq_param2) &&
        (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
        (!show_step_menu)) {

      mcl_gui.draw_microtiming(mcl_seq.md_tracks[last_md_track].speed,
                               seq_param2.cur);
    }
  }
  oled_display.display();
  oled_display.setFont(oldfont);
}

void SeqStepPage::loop() {
  SeqPage::loop();

  if (recording)
    return;

  if (seq_param1.hasChanged() || seq_param2.hasChanged() ||
      seq_param4.hasChanged()) {
    tuning_t const *tuning = MD.getKitModelTuning(last_md_track);

    MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

    for (uint8_t n = 0; n < 16; n++) {

      if (note_interface.notes[n] == 1) {
        uint8_t step = n + (page_select * 16);
        if (step < active_track.length) {

          uint8_t utiming = (seq_param2.cur + 0);
          bool cond_plock;
          uint8_t condition =
              translate_to_step_conditional(seq_param1.cur, &cond_plock);

          active_track.steps[step].cond_id = condition;
          active_track.steps[step].cond_plock = cond_plock;
          active_track.timing[step] = utiming;

          switch (mask_type) {
          case MASK_LOCK:
            active_track.enable_step_locks(step);
            break;
          case MASK_PATTERN:
            active_track.steps[step].trig = true;
            break;
          }

          if ((seq_param4.cur > 0) && (last_md_track < NUM_MD_TRACKS) &&
              (tuning != NULL)) {
            uint8_t base = tuning->base;
            uint8_t note_num = seq_param4.cur;
            uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[note_num]);
            active_track.set_track_pitch(step, machine_pitch);
          }
        }
      }
    }
    seq_param1.old = seq_param1.cur;
    seq_param2.old = seq_param2.cur;
    seq_param4.old = seq_param4.cur;
  }
}

bool SeqStepPage::handleEvent(gui_event_t *event) {

  if ((!recording || EVENT_PRESSED(event, Buttons.BUTTON2)) &&
      SeqPage::handleEvent(event)) {
    if (show_seq_menu) {
      redisplay = true;
      return true;
    }
    return true;
  }

  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    if (device != &MD) {
      return true;
    }

    uint8_t step = track + (page_select * 16);

    if (recording) {
      if (event->mask == EVENT_BUTTON_PRESSED) {

        config_encoders();
        MD.triggerTrack(track, 127);
        last_rec_event = REC_EVENT_TRIG;
        // Don't allow display_refresh if last_md_track != MD.currentTrack
        MD.currentTrack = track;
        last_md_track = MD.currentTrack;

        if (MidiClock.state == 2)
          mcl_seq.md_tracks[track].record_track(127);
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
        return true;
      }
      if (event->mask == EVENT_BUTTON_RELEASED) {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
        return true;
      }
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      mcl_seq.midi_events.update_params = false;
      MD.midi_events.disable_live_kit_update();

      if (MidiClock.state != 2) {
        active_track.send_parameter_locks(step, true);
      }
      show_pitch = true;

      if (step >= active_track.length) {
        return true;
      }

      seq_param2.max =
          mcl_seq.md_tracks[last_md_track].get_timing_mid() * 2 - 1;
      int8_t utiming = active_track.timing[step];
      uint8_t pitch = active_track.get_track_lock(step, 0);
      // Cond
      uint8_t condition =
          translate_to_knob_conditional(active_track.steps[step].cond_id,
                                        active_track.steps[step].cond_plock);
      seq_param1.cur = condition;
      if (pitch != active_track.locks_params_orig[0]) {
        uint8_t note_num = 255;
        tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
        if (tuning) {
          for (uint8_t i = 0; i < tuning->len && note_num == 255; i++) {
            uint8_t ccStored = pgm_read_byte(&tuning->tuning[i]);
            if (ccStored >= pitch) {
              note_num = i;
            }
          }
          if (note_num == 255) {
            seq_param4.cur = 0;
          } else {
            seq_param4.cur = note_num;
          }
          seq_param4.old = seq_param4.cur;
        }
      }
      // Micro
      //      if (note_interface.notes_count_on() <= 1) {
      if (utiming == 0) {
        utiming = mcl_seq.md_tracks[last_md_track].get_timing_mid();
      }
      seq_param2.cur = utiming;
      seq_param2.old = utiming;
      if (!active_track.get_step(step, mask_type)) {
        bool cond_plock;
        active_track.steps[step].cond_id =
            translate_to_step_conditional(condition, &cond_plock);
        active_track.steps[step].cond_plock = cond_plock;
        active_track.timing[step] = utiming;
        CLEAR_BIT64(active_track.oneshot_mask, step);
        active_track.set_step(step, mask_type, true);
        note_interface.ignoreNextEvent(track);
      }
      //      }
    } else if (event->mask == EVENT_BUTTON_RELEASED) {

      if (last_md_track < 15) {
        show_pitch = false;
      }
      if (step >= active_track.length) {
        return true;
      }

      if (note_interface.notes_all_off_md()) {
        mcl_gui.init_encoders_used_clock();
        active_track.reset_params();
        mcl_seq.midi_events.update_params = true;
        MD.midi_events.enable_live_kit_update();
      }
      if (active_track.get_step(step, mask_type)) {
        DEBUG_PRINTLN(F("clear step"));

        if (clock_diff(note_interface.note_hold[port], slowclock) <
            TRIG_HOLD_TIME) {
          active_track.set_step(step, mask_type, false);
          if (mask_type == MASK_PATTERN) {
            active_track.steps[step].cond_id = 0;
            active_track.steps[step].cond_plock = false;
            active_track.timing[step] = active_track.get_timing_mid();
          }
        }
      }
      return true;
    }
    return true;
  } // end TI events

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES:
        if (!note_interface.notes_count_on())
          return true;
        MD.triggerTrack(last_md_track, 127);
        break;
      }
    }
    return true;
  }

  if (recording) {
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      switch (last_rec_event) {
      case REC_EVENT_TRIG:
        if (BUTTON_DOWN(Buttons.BUTTON3)) {
          oled_display.textbox("CLEAR ", "TRACKS");
          for (uint8_t n = 0; n < 16; ++n) {
            mcl_seq.md_tracks[n].clear_track();
          }
        } else {
          oled_display.textbox("CLEAR ", "TRACK");
          active_track.clear_track();
        }
        break;
      case REC_EVENT_CC:
        oled_display.textbox("CLEAR ", "LOCK");
        active_track.clear_param_locks(last_param_id);
        if (BUTTON_DOWN(Buttons.BUTTON3)) {
          oled_display.textbox("CLEAR ", "LOCKS");
          for (uint8_t c = 0; c < NUM_LOCKS; c++) {
            if (active_track.locks_params[c] > 0) {
              active_track.clear_param_locks(active_track.locks_params[c] - 1);
            }
          }
        }
        break;
      }
      queue_redraw();
      return true;
    }
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    recording = !recording;
    if (recording) {
      oled_display.textbox("REC", "");
      setLed2();
    } else {
      clearLed2();
    }
    queue_redraw();
    return true;
  }

  return false;
}

void SeqStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);

  tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
  uint8_t note_num = msg[1] - tuning->base;
  if (note_num < tuning->len) {
    uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[note_num]);
    if (MidiClock.state != 2) {
      MD.setTrackParam(last_md_track, 0, machine_pitch);
      MD.triggerTrack(last_md_track, 127);
    }
    seq_param4.cur = note_num;
  }
}

void SeqStepMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];
  uint8_t step;
  if (track_param > 23) {
    return;
  }
  if (SeqPage::recording) {
    // Record CC
    seq_step_page.last_rec_event = REC_EVENT_CC;
    if (MidiClock.state != 2) {
      return;
    }

    seq_step_page.last_param_id = track_param;
    last_md_track = track;
    last_md_track = MD.currentTrack;
    // ignore level
    if (track_param > 31) {
      return;
    }
    seq_step_page.config_encoders();

    mcl_seq.md_tracks[track].update_param(track_param, value);

    MD.kit.params[track][track_param] = value;
    mcl_seq.md_tracks[track].record_track_locks(track_param, value);
    return;
  }

  uint8_t store_lock = 255;
  for (int i = 0; i < 16; i++) {
    if ((note_interface.notes[i] == 1)) {
      step = i + (SeqPage::page_select * 16);
      if (active_track.set_track_locks(step, track_param, value)) {
        store_lock = 0;
      } else {
        store_lock = 1;
      }

      active_track.enable_step_locks(step);
      if (seq_step_page.mask_type == MASK_PATTERN) {
        uint8_t utiming = (seq_param2.cur + 0);
        bool cond_plock;
        uint8_t condition = seq_step_page.translate_to_step_conditional(
            seq_param1.cur, &cond_plock);

        active_track.steps[step].trig = true;
        active_track.steps[step].cond_id = condition;
        active_track.steps[step].cond_plock = cond_plock;
        active_track.timing[step] = utiming;

      } else {
        // SET_BIT64_P(mask, step);
      }
    }
  }
  if (store_lock == 0) {
    char str[5] = "--  ";
    char str2[4] = "-- ";
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.get_model(last_md_track), track_param);
    if (modelname != NULL) {
      m_strncpy_p(str, modelname, 3);
      if (strlen(str) == 2) {
        str[2] = ' ';
        str[3] = '\0';
      }
    }
    itoa(value, str2, 10);
#ifdef OLED_DISPLAY
    oled_display.textbox(str, str2);
#endif
  }
  if (store_lock == 1) {
#ifdef OLED_DISPLAY
    oled_display.textbox("LOCK PARAMS ", "FULL");
#endif
  }
}

void SeqStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqStepMidiEvents::onNoteOnCallback_Midi2);

  state = true;
}

void SeqStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqStepMidiEvents::onNoteOnCallback_Midi2);
  state = false;
}
