#include "MCL_impl.h"

void GridWritePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  encoders[1]->cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);

  ((MCLEncoder *)encoders[3])->max = 6;
  encoders[3]->cur = 4;
  ((MCLEncoder *)encoders[2])->max = 1;

  // MD.requestKit(MD.currentKit);
  note_interface.init_notes();
  trig_interface.send_md_leds(TRIGLED_OVERLAY);
  trig_interface.on();

  note_interface.state = true;
  // GUI.display();
  curpage = W_PAGE;
}

void GridWritePage::draw_popup() {
  char str[16];
  strcpy(str, "GROUP LOAD");

  if (!show_track_type) {
    strcpy(str, "LOAD FROM  ");
    str[10] = 'A' + proj.get_grid();
  }
  mcl_gui.draw_popup(str, true, 28);
}

void GridWritePage::display() {

  draw_popup();

  const uint64_t mute_mask = 0, slide_mask = 0;

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  if (show_track_type) {
    mcl_gui.draw_track_type_select(36, MCLGUI::s_menu_y + 12,
                                   mcl_cfg.track_type_select);
  } else {
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 21, 0, 0, 0, 16,
                       mute_mask, slide_mask);

    char K[4] = {'\0'};

    // draw step count
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (64 *
         ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
    itoa(step_count, K, 10);
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4,
                              "STEP", K);

    // draw quantize
    strcpy(K, "---");
    if ((encoders[3]->getValue() < 7) && (encoders[3]->getValue() > 0)) {
      uint8_t x = 1 << encoders[3]->getValue();
      itoa(x, K, 10);
    }
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 26,
                              MCLGUI::s_menu_y + 4, "QUANT", K);

    oled_display.setFont(&TomThumb);
    // draw data flow in the center
    oled_display.setCursor(48, MCLGUI::s_menu_y + 12);
    oled_display.print("SND");
    oled_display.setCursor(46, MCLGUI::s_menu_y + 19);
    oled_display.print("GRID");

    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 8, 5);
    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 15, 5);

    oled_display.setCursor(74, MCLGUI::s_menu_y + 12);
    oled_display.print("MD");
    oled_display.setCursor(74, MCLGUI::s_menu_y + 19);
    oled_display.print("SEQ");
  }
  oled_display.display();
  oled_display.setFont(oldfont);
}
void GridWritePage::chain() {
  oled_display.textbox("LOAD SLOTS", "");
  oled_display.display();
  /// !Note, note_off_event has reentry issues, so we have to first set
  /// the page to avoid driving this code path again.

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
  mcl_actions.write_tracks(0, grid_page.encoders[1]->getValue(),
                           track_select_array);
}

bool GridWritePage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }
  DEBUG_DUMP(event->source);
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
          DEBUG_PRINTLN(F("notes all off"));
          if (BUTTON_DOWN(Buttons.BUTTON2)) {
            return true;
          } else {
            chain();
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

  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {

    //    if (!note_interface.notes_all_off()) { return true; }
    //   chain();
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    //  write the whole row

    trig_interface.off();
    uint8_t offset = proj.get_grid() * 16;

    uint8_t track_select_array[NUM_SLOTS] = {0};

    track_select_array_from_type_select(track_select_array);
    //   write_tracks_to_md(-1);
    oled_display.textbox("LOAD GROUPS", "");
    oled_display.display();
    mcl_actions.write_original = 1;
    GUI.setPage(&grid_page);
    mcl_actions.write_tracks(0, grid_page.encoders[1]->getValue(),
                             track_select_array);
    curpage = 0;
    return true;
  }
}
