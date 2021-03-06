#include "MCL_impl.h"
#define S_PAGE 3

void GridSavePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  trig_interface.send_md_leds(TRIGLED_OVERLAY);
  trig_interface.on();
  note_interface.state = true;
  grid_page.reload_slot_models = false;
  draw_popup();
}

void GridSavePage::draw_popup() {
  char str[16];
  strcpy(str,"GROUP SAVE");

  if (!show_track_type) {
    strcpy(str, "SAVE TO  ");
    str[8] = 'A' + proj.get_grid();
  }
  mcl_gui.draw_popup(str, true, 28);
}

void GridSavePage::loop() {

  // prevent encoder 0 from changing when sequencer is running
  if (encoders[0]->hasChanged() && MidiClock.state == 2) {
    encoders[0]->cur = encoders[0]->old;
  }
}
void GridSavePage::display() {

  draw_popup();

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  const uint64_t slide_mask = 0;
  const uint64_t mute_mask = 0;
  if (show_track_type) {
    mcl_gui.draw_track_type_select(36, MCLGUI::s_menu_y + 12,
                                   mcl_cfg.track_type_select);
  } else {
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 21, 0, 0, 0, 16,
                       mute_mask, slide_mask);

    char modestr[7];
    get_modestr(modestr);

    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4,
                              "MODE", modestr);

    char step[4] = {'\0'};
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (64 *
         ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
    itoa(step_count, step, 10);

    // mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 26,
    // MCLGUI::s_menu_y + 4, "STEP", step);

    oled_display.setFont(&TomThumb);
    // draw data flow in the center
    constexpr uint8_t data_x = 56;
    if (MidiClock.state != 2 && encoders[0]->cur == SAVE_MERGE) {
      oled_display.setCursor(data_x + 2, MCLGUI::s_menu_y + 12);
      oled_display.print("MD");
      oled_display.setCursor(data_x, MCLGUI::s_menu_y + 19);
      oled_display.print("SEQ");

      oled_display.drawFastHLine(data_x + 13, MCLGUI::s_menu_y + 8, 2, WHITE);
      oled_display.drawFastHLine(data_x + 13, MCLGUI::s_menu_y + 15, 2, WHITE);
      oled_display.drawFastVLine(data_x + 15, MCLGUI::s_menu_y + 8, 8, WHITE);
      mcl_gui.draw_horizontal_arrow(data_x + 16, MCLGUI::s_menu_y + 12, 5);
    } else {
      if (encoders[0]->cur == SAVE_SEQ || MidiClock.state == 2) {
        oled_display.setCursor(data_x, MCLGUI::s_menu_y + 12);
        oled_display.print("SND");
        oled_display.setCursor(data_x, MCLGUI::s_menu_y + 19);
        oled_display.print("SEQ");

        oled_display.drawFastHLine(data_x + 13, MCLGUI::s_menu_y + 8, 2, WHITE);
        oled_display.drawFastHLine(data_x + 13, MCLGUI::s_menu_y + 15, 2,
                                   WHITE);
        oled_display.drawFastVLine(data_x + 15, MCLGUI::s_menu_y + 8, 8, WHITE);
        mcl_gui.draw_horizontal_arrow(data_x + 16, MCLGUI::s_menu_y + 12, 5);
      } else {
        oled_display.setCursor(data_x, MCLGUI::s_menu_y + 15);
        oled_display.print(modestr);
        mcl_gui.draw_horizontal_arrow(data_x + 13, MCLGUI::s_menu_y + 12, 8);
      }
    }

    oled_display.setCursor(data_x + 24, MCLGUI::s_menu_y + 15);
    oled_display.print("GRID");
  }
  oled_display.display();
  oled_display.setFont(oldfont);
}

void GridSavePage::save() {
  char modestr[7];
  get_modestr(modestr);
  oled_display.textbox("SAVE SLOTS: ", modestr);
  oled_display.display();

  uint8_t save_mode = encoders[0]->cur;
  if (MidiClock.state == 2) {
    encoders[0]->cur = SAVE_SEQ;
    encoders[0]->old = SAVE_SEQ;
    save_mode = SAVE_SEQ;
  }

  uint8_t track_select_array[NUM_SLOTS] = {0};

  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    if (note_interface.notes[n] > 0) {
      SET_BIT32(track_select, n + proj.get_grid() * 16);
    }
  }

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (IS_BIT_SET32(track_select, n)) {
      track_select_array[n] = 1;
    }
  }

  GUI.setPage(&grid_page);
  trig_interface.off();
  mcl_actions.store_tracks_in_mem(0, grid_page.encoders[1]->getValue(),
                                  track_select_array, save_mode);
}

void GridSavePage::get_modestr(char *modestr) {

  strcpy(modestr, "SEQ");
  if (MidiClock.state != 2) {
    if (encoders[0]->cur == SAVE_MD) {
      strcpy(modestr, "MD");
    }
    if (encoders[0]->cur == SAVE_MERGE) {
      strcpy(modestr, "MERGE");
    }
  }
}

bool GridSavePage::handleEvent(gui_event_t *event) {
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (show_track_type) {
        if (track < 4) {
          TOGGLE_BIT16(mcl_cfg.track_type_select, track);
          MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
        }
      } else {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
      }
    } else {
      if (!show_track_type) {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
        if (note_interface.notes_all_off()) {
          if (BUTTON_DOWN(Buttons.BUTTON2)) {
            return true;
          } else {
            save();
          }
        }
      }
    }

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    show_track_type = true;
    MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    trig_interface.off();
    uint8_t offset = proj.get_grid() * 16;

    uint8_t track_select_array[NUM_SLOTS] = {0};

    track_select_array_from_type_select(track_select_array);

    char modestr[7];
    get_modestr(modestr);
    oled_display.textbox("SAVE GROUPS: ", modestr);
    oled_display.display();

    uint8_t save_mode = encoders[0]->cur;
    if (MidiClock.state == 2) {
      encoders[0]->cur = SAVE_SEQ;
      encoders[0]->old = SAVE_SEQ;
      save_mode = SAVE_SEQ;
    }

    mcl_actions.store_tracks_in_mem(grid_page.encoders[0]->getValue(),
                                    grid_page.encoders[1]->getValue(),
                                    track_select_array, save_mode);
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}
