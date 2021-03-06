#include "MCL_impl.h"

constexpr auto sz_allowedchar = 71;

// idx -> chr
inline char _getchar(uint8_t i) {
  if (i >= sz_allowedchar)
    i = sz_allowedchar - 1;
  return i
      ["abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_&@-=! "];
}

// chr -> idx
uint8_t _findchar(char chr) {
  // Check to see that the character chosen is in the list of allowed
  // characters
  for (auto x = 0; x < sz_allowedchar; ++x) {
    if (chr == _getchar(x)) {
      return x;
    }
  }
  // Ensure the encoder does not go out of bounds, by resetting it to a
  // character within the allowed characters list
  return sz_allowedchar - 1;
}

void TextInputPage::setup() {}

void TextInputPage::init() {
#ifdef OLED_DISPLAY
  classic_display = false;
  GUI.lines[0].flashActive = false;
  GUI.lines[1].flashActive = false;
  oled_display.setTextColor(WHITE, BLACK);
#endif
}

void TextInputPage::init_text(char *text_, const char *title_, uint8_t len) {
  textp = text_;
  title = title_;
  length = len;
  max_length = len;
  m_strncpy(text, text_, len);
  cursor_position = 0;
  config_normal();
}

void TextInputPage::update_char() {
  auto chr = text[cursor_position];
  auto match = _findchar(chr);
  encoders[1]->old = encoders[1]->cur = match;
  encoders[0]->old = encoders[0]->cur;
}

// normal:
// E0 -> cursor
// E1 -> choose char
void TextInputPage::config_normal() {
  ((MCLEncoder *)encoders[0])->max = length - 1;
  ((MCLEncoder *)encoders[1])->max = sz_allowedchar - 1;
  normal_mode = true;
  // restore E0
  encoders[0]->cur = cursor_position;
  // restore E1
  update_char();
#ifdef OLED_DISPLAY
  // redraw popup body
  mcl_gui.draw_popup(title);
#endif
  // update clock
  last_clock = slowclock;
}

// charpane layout:
// TomThumb: 5x6
// Boundary: (1, 1, 126, 30)
// Dimension: (126, 30)
// draw each char in a 7x7 cell (padded) to fill the boundary.

constexpr auto charpane_w = 18;
constexpr auto charpane_h = 4;
constexpr auto charpane_padx = 1;
constexpr auto charpane_pady = 6;

// input: col and row
// output: coordinates on screen
static void calc_charpane_coord(uint8_t &x, uint8_t &y) {
  x = 2 + (x * 7);
  y = 2 + (y * 7);
}

// charpane:
// E0 -> x axis [0..17]
// E1 -> y axis [0..3]
void TextInputPage::config_charpane() {
#ifdef OLED_DISPLAY
  // char pane not supported on 1602 displays

  ((MCLEncoder *)encoders[0])->max = charpane_w - 1;
  ((MCLEncoder *)encoders[1])->max = charpane_h - 1;
  normal_mode = false;
  auto chridx = _findchar(text[cursor_position]);
  // restore E0
  encoders[0]->cur = chridx % charpane_w;
  encoders[0]->old = encoders[0]->cur;
  // restore E1
  encoders[1]->cur = chridx / charpane_w;
  encoders[1]->old = encoders[1]->cur;

  oled_display.setFont(&TomThumb);
  oled_display.clearDisplay();
  oled_display.drawRect(0, 0, 128, 32, WHITE);
  uint8_t chidx = 0;
  for (uint8_t y = 0; y < charpane_h; ++y) {
    for (uint8_t x = 0; x < charpane_w; ++x) {
      auto sx = x, sy = y;
      calc_charpane_coord(sx, sy);
      oled_display.setCursor(sx + charpane_padx, sy + charpane_pady);
      oled_display.print(_getchar(chidx));
      ++chidx;
    }
  }

  // initial highlight of selected char
  uint8_t sx = encoders[0]->cur, sy = encoders[1]->cur;
  calc_charpane_coord(sx, sy);
  oled_display.fillRect(sx, sy, 7, 7, INVERT);
  oled_display.display();
#endif
}

void TextInputPage::display_normal() {
  constexpr auto s_text_x = MCLGUI::s_menu_x + 8;
  constexpr auto s_text_y = MCLGUI::s_menu_y + 12;

  // update cursor position
  cursor_position = encoders[0]->getValue();

  // Check to see that the character chosen is in the list of allowed
  // characters
  if (encoders[0]->hasChanged()) {
    update_char();
    last_clock = slowclock;
  }

  if (encoders[1]->hasChanged()) {
    last_clock = slowclock;
    encoders[1]->old = encoders[1]->cur;
    text[cursor_position] = _getchar(encoders[1]->getValue());
  }
  auto time = clock_diff(last_clock, slowclock);

#ifdef OLED_DISPLAY
  // mcl_gui.clear_popup(); <-- E_TOOSLOW
  auto oldfont = oled_display.getFont();
  oled_display.fillRect(s_text_x, s_text_y, 6 * length, 8, BLACK);
  oled_display.setFont();
  oled_display.setCursor(s_text_x, s_text_y);
  oled_display.println(text);
  if (time < FLASH_SPEED) {
    // the default font is 6x8
    auto tx = s_text_x + 6 * cursor_position;
    oled_display.fillRect(tx, s_text_y, 6, 8, WHITE);
    oled_display.setCursor(s_text_x + 6 * cursor_position, s_text_y);
    oled_display.setTextColor(BLACK);
    oled_display.print(text[cursor_position]);
    oled_display.setTextColor(WHITE);
  }
  if (time > FLASH_SPEED * 2) {
    last_clock = slowclock;
  }
  oled_display.setFont(oldfont);
  oled_display.display();
#else
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, title);
  GUI.setLine(GUI.LINE2);
  char tmp_str[18];
  memcpy(tmp_str, &text[0], 18);
  if (time > FLASH_SPEED) {
    tmp_str[cursor_position] = (char)255;
  }
  if (time > FLASH_SPEED * 2) {
    last_clock = slowclock;
  }
  GUI.clearLine();
  GUI.put_string_at(0, tmp_str);
#endif
}

void TextInputPage::display_charpane() {
#ifdef OLED_DISPLAY
  if (encoders[0]->hasChanged() || encoders[1]->hasChanged()) {
    // clear old highlight
    uint8_t sx = encoders[0]->old, sy = encoders[1]->old;
    calc_charpane_coord(sx, sy);
    oled_display.fillRect(sx, sy, 7, 7, INVERT);
    // draw new highlight
    sx = encoders[0]->cur;
    sy = encoders[1]->cur;
    calc_charpane_coord(sx, sy);
    oled_display.fillRect(sx, sy, 7, 7, INVERT);
    // update text. in charpane mode, cursor_position remains constant
    uint8_t chridx = encoders[0]->cur + encoders[1]->cur * charpane_w;
    text[cursor_position] = _getchar(chridx);
    // mark encoders as unchanged
    encoders[0]->old = encoders[0]->cur;
    encoders[1]->old = encoders[1]->cur;
  }

  last_clock = slowclock;
  oled_display.display();
#endif
}

void TextInputPage::display() {
  if (normal_mode)
    display_normal();
  else
    display_charpane();
}

bool TextInputPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return true;
  }
  #ifdef OLED_DISPLAY
  // in char-pane mode, do not handle any events
  // except shift-release event.
  if (!normal_mode) {
    if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
      oled_display.clearDisplay();
      // before exiting charpane, advance current cursor to the next.
      ++cursor_position;
      if (cursor_position >= length) {
        cursor_position = length - 1;
      }
      // then, config normal input line
      config_normal();
      return true;
    }
    return false;
  }
  #endif
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    if (!no_escape) {
    return_state = false;
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    config_charpane();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {

    if (cursor_position == length - 1 && !isspace(text[cursor_position])) {
      // delete last
      text[cursor_position] = ' ';
    } else {
      if (cursor_position == 0) {
        // move the cursor, and delete first
        cursor_position = 1;
      }
      // backspace
      for (uint8_t i = cursor_position - 1; i < length - 1; ++i) {
        text[i] = text[i + 1];
      }
      text[length - 1] = ' ';
      --cursor_position;
    }
    config_normal();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    return_state = true;
    uint8_t cpy_len = length;
    for (uint8_t n = length - 1; n > 0 && text[n] == ' '; n--) {
      cpy_len -= 1;
    }
    m_strncpy(textp, text, cpy_len);
    textp[cpy_len] = '\0';
    GUI.ignoreNextEvent(event->source);
    GUI.popPage();
    return true;
  }

  // if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  // Toggle upper + lower case
  // if (encoders[1]->cur <= 25) {
  // encoders[1]->cur += 26;
  //} else if (encoders[1]->cur <= 51) {
  // encoders[1]->cur -= 26;
  //}
  // return true;
  //}

  // if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
  // Clear text
  // for (uint8_t n = 1; n < length; n++) {
  // text[n] = ' ';
  //}
  // text[0] = 'a';
  // encoders[0]->cur = 0;
  // DEBUG_PRINTLN(text);
  // update_char();
  // return true;
  //}

  return false;
}
