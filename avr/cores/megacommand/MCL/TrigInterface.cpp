#include "MCL_impl.h"

void TrigInterface::start() {

}

void TrigInterface::send_md_leds(TrigLEDMode mode) {
    uint16_t led_mask = 0;
    for (uint8_t i = 0; i < 16; i++) {
      if (note_interface.notes[i] == 1) {
        SET_BIT16(led_mask, i);
      }
    }
    MD.set_trigleds(led_mask, mode);
}

bool TrigInterface::on() {

  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  cmd_key_state = 0;
  sysex->addSysexListener(this);
  state = true;
  DEBUG_PRINTLN(F("activating trig interface"));
  MD.activate_trig_interface();
  note_interface.notecount = 0;
  note_interface.init_notes();
  note_interface.note_proceed = true;
  return true;
}

bool TrigInterface::off() {
  sysex->removeSysexListener(this);
  note_interface.note_proceed = false;
  DEBUG_PRINTLN(F("deactiviating trig interface"));
  if (!state) {
    return false;
  }
  state = false;
  if (!MD.connected) {
    return false;
  }
  MD.deactivate_trig_interface();
  return true;
}

void TrigInterface::end() { }

void TrigInterface::end_immediate() {
  if (!state) {
    return;
  }
 if (sysex->getByte(0) != ids[0]) { return; }
 if (sysex->getByte(1) != ids[1]) { return; }

  uint8_t key = sysex->getByte(2);
  bool key_down = false;

  if (key >= 0x40) {
    key_down = true;
    key -= 0x40;
  }
  if (key < 16) {
    if (key_down) {
      note_interface.note_off_event(key, UART1_PORT);
    }
    else {
      note_interface.note_on_event(key, UART1_PORT);
    }
   return;
  }
  if (key_down) {
    CLEAR_BIT64(cmd_key_state, key);
  }
  else {
    SET_BIT64(cmd_key_state, key);
  }
  gui_event_t event;
  event.source = key + 64; //EVENT_CMD
  event.mask = key_down ? EVENT_BUTTON_RELEASED : EVENT_BUTTON_PRESSED;
  event.port = UART1_PORT;
  EventRB.putp(&event);

  return;
}

TrigInterface trig_interface;
