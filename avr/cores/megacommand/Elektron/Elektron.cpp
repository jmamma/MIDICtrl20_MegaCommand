#include "Elektron.h"

#define SYSEX_RETRIES 1

uint16_t ElektronDevice::sendRequest(uint8_t *data, uint8_t len, bool send) {
  uint8_t buf[256];

  if (send) {
    uint8_t i = 0;
    buf[i++] = 0xF0;
    for (uint8_t n = 0; n < sysex_protocol.header_size; n++) {
      buf[i++] = sysex_protocol.header[n];
    }
    for (uint8_t n = 0; n < len; n++) {
      buf[i++] = data[n] & 0x7F;
    }
    buf[i++] = 0xF7;
    uart->m_putc(buf, i);
  }
  return len + sysex_protocol.header_size + 2;
}

uint16_t ElektronDevice::sendRequest(uint8_t type, uint8_t param, bool send) {
  uint8_t data[] = {type, param};
  return sendRequest(data, 2, send);
}

bool ElektronDevice::get_fw_caps() {

  uint8_t data[2] = {0x70, 0x30};
  sendRequest(data, sizeof(data));

  uint8_t msgType = waitBlocking();

  fw_caps = 0;

  auto begin = sysex_protocol.header_size + 1;
  auto listener = getSysexListener();

  if (msgType == 0x72) {
    if (listener->sysex->getByte(begin) == 0x30) {
      ((uint8_t *)&(fw_caps))[0] = listener->sysex->getByte(begin+1);
      ((uint8_t *)&(fw_caps))[1] = listener->sysex->getByte(begin+2);
    }
    return true;
  }
  return false;
}

void ElektronDevice::activate_trig_interface() {
  uint8_t data[3] = {0x70, 0x31, 0x01};
  sendRequest(data, sizeof(data));
  waitBlocking();
}

void ElektronDevice::deactivate_trig_interface() {
  uint8_t data[3] = {0x70, 0x31, 0x00};
  sendRequest(data, sizeof(data));
  waitBlocking();
}

void ElektronDevice::activate_track_select() {
  uint8_t data[3] = {0x70, 0x32, 0x01};
  sendRequest(data, sizeof(data));
  waitBlocking();
}

void ElektronDevice::deactivate_track_select() {
  uint8_t data[3] = {0x70, 0x32, 0x00};
  sendRequest(data, sizeof(data));
  waitBlocking();
}

void ElektronDevice::undokit_sync() {
  uint8_t data[2] = {0x70, 0x42};
  sendRequest(data, sizeof(data));
}

void ElektronDevice::set_trigleds(uint16_t bitmask, TrigLEDMode mode) {
  uint8_t data[5] = {0x70, 0x35, 0x00, 0x00, 0x00};
  // trigleds[0..6]
  data[2] = bitmask & 0x7F;
  // trigleds[7..13]
  data[3] = (bitmask >> 7) & 0x7F;
  // trigleds[14..15]
  data[4] = (bitmask >> 14) | (mode << 2);
  sendRequest(data, sizeof(data));
  //waitBlocking();
}

uint8_t ElektronDevice::waitBlocking(uint16_t timeout) {
  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;
  auto listener = getSysexListener();
  listener->start();
  do {
    current_clock = read_slowclock();
    handleIncomingMidi();
  } while ((clock_diff(start_clock, current_clock) < timeout) &&
           (listener->msgType == 255));
  return listener->msgType;
}

void ElektronDevice::requestKit(uint8_t kit) { 
  sendRequest(sysex_protocol.kitrequest_id, kit); 
}

void ElektronDevice::requestPattern(uint8_t pattern) {
  sendRequest(sysex_protocol.patternrequest_id, pattern);
}

void ElektronDevice::requestSong(uint8_t song) {
  sendRequest(sysex_protocol.songrequest_id, song);
}

void ElektronDevice::requestGlobal(uint8_t global) {
  sendRequest(sysex_protocol.globalrequest_id, global);
}


uint8_t ElektronDevice::getBlockingStatus(uint8_t type, uint16_t timeout) {
  SysexCallback cb(type);

  auto listener = getSysexListener();

  listener->addOnStatusResponseCallback(&cb, (sysex_status_callback_ptr_t)&SysexCallback::onStatusResponse);
  sendRequest(sysex_protocol.statusrequest_id, type);
  connected = cb.waitBlocking(timeout);
  listener->removeOnStatusResponseCallback(&cb);

  return connected ? cb.value : 255;
}

bool ElektronDevice::getBlockingKit(uint8_t kit, uint16_t timeout) {
  SysexCallback cb;
  uint8_t count = SYSEX_RETRIES;
  auto listener = getSysexListener();
  while ((MidiClock.state == 2) &&
         ((MidiClock.mod12_counter > 6) || (MidiClock.mod12_counter == 0)))
    ;
  while (count) {
    listener->addOnKitMessageCallback(
        &cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
    requestKit(kit);
    connected = cb.waitBlocking(timeout);
    listener->removeOnKitMessageCallback(&cb);
    if (connected) {
      auto kit = getKit();
      if (kit != nullptr && kit->fromSysex(midi)) {
        return true;
      }
    }
    count--;
  }
  return false;
}

bool ElektronDevice::getBlockingPattern(uint8_t pattern, uint16_t timeout) {
  SysexCallback cb;
  uint8_t count = SYSEX_RETRIES;
  auto listener = getSysexListener();
  while ((MidiClock.state == 2) &&
         ((MidiClock.mod12_counter > 6) || (MidiClock.mod12_counter == 0)))
    ;
  while (count) {
    listener->addOnPatternMessageCallback(
        &cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
    requestPattern(pattern);
    connected = cb.waitBlocking(timeout);
    listener->removeOnPatternMessageCallback(&cb);
    if (connected) {
      auto pattern = getPattern();
      if (pattern != nullptr && pattern->fromSysex(midi)) {
        return true;
      }
    }
    count--;
  }
  return false;
}

bool ElektronDevice::getBlockingGlobal(uint8_t global, uint16_t timeout) {
  SysexCallback cb;
  auto listener = getSysexListener();
  listener->addOnGlobalMessageCallback(
      &cb, (sysex_callback_ptr_t)&SysexCallback::onSysexReceived);
  requestGlobal(global);
  connected = cb.waitBlocking(timeout);
  listener->removeOnGlobalMessageCallback(&cb);
  if (connected) {
    auto global = getGlobal();
    if (global != nullptr && global->fromSysex(midi)) {
      return true;
    }
  }

  return connected;
}

uint8_t ElektronDevice::getCurrentTrack(uint16_t timeout) {
  uint8_t value = getBlockingStatus(sysex_protocol.track_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentTrack = value;
    return value;
  }
}
uint8_t ElektronDevice::getCurrentKit(uint16_t timeout) {
  uint8_t value = getBlockingStatus(sysex_protocol.kit_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentKit = value;
    return value;
  }
}

uint8_t ElektronDevice::getCurrentPattern(uint16_t timeout) {
  uint8_t value = getBlockingStatus(sysex_protocol.pattern_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentPattern = value;
    return value;
  }
}

uint8_t ElektronDevice::getCurrentGlobal(uint16_t timeout) {
  uint8_t value = getBlockingStatus(sysex_protocol.global_index_request_id, timeout);
  if (value == 255) {
    return 255;
  } else {
    currentGlobal = value;
    return value;
  }
}

void ElektronDevice::setStatus(uint8_t id, uint8_t value) {
  uint8_t data[] = { sysex_protocol.status_set_id , (uint8_t)(id & 0x7F), (uint8_t)(value & 0x7F) };
  sendRequest(data, countof(data));
}

void ElektronDevice::setKitName(const char *name) {
  uint8_t buf[64];
  uint8_t i = 0;

  buf[i++] = 0xF0;

  for (uint8_t n = 0; n < sysex_protocol.header_size; n++) {
    buf[i++] = sysex_protocol.header[n];
  }

  buf[i++] = sysex_protocol.kitname_set_id;

  for (uint8_t n = 0; n < sysex_protocol.kitname_length; n++) {
    buf[i++] = name[i] & 0x7F;;
  }

  buf[i++] = 0xF7;

  uart->m_putc(buf,i);
}

uint8_t ElektronDevice::setTempo(float tempo, bool send) {
  uint16_t qtempo = tempo * 24;
  uint8_t data[3] = {sysex_protocol.tempo_set_id, (uint8_t)(qtempo >> 7), (uint8_t)(qtempo & 0x7F)};
  return sendRequest(data, countof(data), send);
}

void ElektronDevice::loadGlobal(uint8_t id) { 
  uint8_t data[] = {sysex_protocol.load_global_id, (uint8_t)(id & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::loadKit(uint8_t kit) {
  uint8_t data[] = {sysex_protocol.load_kit_id, (uint8_t)(kit & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::loadPattern(uint8_t pattern) {
  uint8_t data[] = {sysex_protocol.load_pattern_id, (uint8_t)(pattern & 0x7F)};
  sendRequest(data, countof(data));
}

void ElektronDevice::saveCurrentKit(uint8_t pos) {
  uint8_t data[2] = {sysex_protocol.save_kit_id, (uint8_t)(pos & 0x7F)};
  sendRequest(data, countof(data));
}

PGM_P getMachineNameShort(uint8_t machine, uint8_t type, const short_machine_name_t* table, size_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (pgm_read_byte(&table[i].id) == machine) {
      if (type == 1) {
        return table[i].name1;
      }
      else {
        return table[i].name2;
      }

    }
  }
}

