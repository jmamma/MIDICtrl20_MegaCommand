/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL_impl.h"

#define MD_KIT_LENGTH 0x4D0

void MCLActions::setup() {
  DEBUG_PRINTLN(F("mcl actions setup"));
  mcl_actions_callbacks.setup_callbacks();
  mcl_actions_midievents.setup_callbacks();
  for (uint8_t i = 0; i < NUM_SLOTS; i++) {
    next_transitions[i] = 0;
    transition_offsets[i] = 0;
    send_machine[i] = 0;
    transition_level[i] = 0;
  }
}

void MCLActions::kit_reload(uint8_t pattern) {
  DEBUG_PRINT_FN();
  if (mcl_actions.do_kit_reload != 255) {
    if (mcl_actions.writepattern == pattern) {
      auto dev1 =
          midi_active_peering.get_device(UART1_PORT)->asElektronDevice();
      auto dev2 =
          midi_active_peering.get_device(UART2_PORT)->asElektronDevice();
      if (dev1 != nullptr) {
        dev1->loadKit(mcl_actions.do_kit_reload);
      }
      if (dev2 != nullptr) {
        dev2->loadKit(mcl_actions.do_kit_reload);
      }
    }
    mcl_actions.do_kit_reload = 255;
  }
}

uint8_t MCLActions::get_grid_idx(uint8_t slot_number) {
  return slot_number / GRID_WIDTH;
}

GridDeviceTrack *MCLActions::get_grid_dev_track(uint8_t slot_number,
                                                uint8_t *track_idx,
                                                uint8_t *dev_idx) {
  uint8_t grid_idx = get_grid_idx(slot_number);
  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  // Find first device that is hosting this slot_number.
  for (uint8_t n = 0; n < 2; n++) {
    auto *p = &(devs[n]->grid_devices[grid_idx]);
    for (uint8_t i = 0; i < GRID_WIDTH; i++) {
      if (slot_number == p->tracks[i].get_slot_number()) {
        *track_idx = i;
        *dev_idx = n;
        return &(p->tracks[i]);
      }
    }
  }
  *track_idx = 255;
  *dev_idx = 255;
  return nullptr;
}

SeqTrack* __attribute__((noinline))  MCLActions::get_dev_slot_info(uint8_t slot_number, uint8_t *grid_idx,
                                        uint8_t *track_idx, uint8_t *track_type,
                                        uint8_t *dev_idx) {
  GridDeviceTrack *p = get_grid_dev_track(slot_number, track_idx, dev_idx);
  *grid_idx = get_grid_idx(slot_number);
  if (p) {
    *track_type = p->track_type;
    return p->seq_track;
  }
  *track_type = 255;
  return nullptr;
}

void MCLActions::store_tracks_in_mem(int column, int row,
                                     uint8_t *slot_select_array,
                                     uint8_t merge) {
  DEBUG_PRINT_FN();

  EmptyTrack empty_track;

  uint8_t readpattern = MD.currentPattern;

  patternswitch = PATTERN_STORE;

  uint8_t old_grid = proj.get_grid();

  bool save_grid_tracks[2] = {false, false};

  uint8_t i = 0;

  for (i = 0; i < NUM_SLOTS; i++) {
    if (slot_select_array[i] > 0) {
      uint8_t grid_idx = get_grid_idx(i);
      save_grid_tracks[grid_idx] = true;
    }
  }
#ifndef EXT_TRACKS
  save_grid_tracks[1] = false;
#endif

  if (MidiClock.state == 2) {
    merge = 0;
  }

  FOREACH_ELEKTRON_DEVICE([&](uint8_t i, auto elektron_dev){
    if (!save_grid_tracks[i]) { return; }
    if (merge > 0) {
      DEBUG_PRINTLN(F("fetching pattern"));
      if (elektron_dev->getBlockingPattern(readpattern)) {
        DEBUG_PRINTLN(F("could not receive pattern"));
        save_grid_tracks[i] = false;
        return;
      }
    }

    if (elektron_dev->canReadWorkspaceKit()) {
      if (!elektron_dev->getBlockingKit(0x7F)) {
        DEBUG_PRINTLN(F("could not receive kit"));
        save_grid_tracks[i] = false;
        return;
      }
    } else {
      auto kit = elektron_dev->getCurrentKit();
      elektron_dev->saveCurrentKit(kit);
      if (!elektron_dev->getBlockingKit(kit)) {
        DEBUG_PRINTLN(F("could not receive kit"));
        save_grid_tracks[i] = false;
        return;
      }
    }

    if (MidiClock.state == 2) {
      elektron_dev->updateKitParams();
    }
  });

  uint8_t first_note = 255;

  GridRowHeader row_headers[NUM_GRIDS];
  GridTrack grid_track;

  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.read_grid_row_header(&row_headers[n], grid_page.getRow());
  }

  DEBUG_DUMP(Analog4.connected);
  bool online;

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
    if (slot_select_array[i] == 0) { return; }
    if (first_note == 255) {
      first_note = i;
    }

    online = (elektron_dev != nullptr);
    DEBUG_DUMP(track_type);
    // If save_grid_tracks[grid_idx] turns false, it means getBlockingKit
    // has failed, so we just skip this device.
    if (!save_grid_tracks[grid_idx]) {
      return;
    }

    if (track_type != 255) {
      proj.select_grid(grid_idx);

      // Preserve existing chain settings before save.
      if (row_headers[grid_idx].track_type[track_idx] != EMPTY_TRACK_TYPE) {
        grid_track.load_from_grid(track_idx, row);
        empty_track.chain.loops = grid_track.chain.loops;
        empty_track.chain.row = grid_track.chain.row;
      } else {
        empty_track.chain.row = row;
        empty_track.chain.loops = 0;
      }
      DEBUG_DUMP(track_type);
      auto pdevice_track = empty_track.init_track_type(track_type);
      pdevice_track->store_in_grid(track_idx, grid_page.getRow(), seq_track, merge,
                                   online);
      row_headers[grid_idx].update_model(
          track_idx, pdevice_track->get_model(), track_type);
    }
  });

  // Only update row name if, the current row is not active.
  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    if (!row_headers[n].active) {
      for (uint8_t c = 0; c < 17; c++) {
        row_headers[n].name[c] = MD.kit.name[c];
      }
      row_headers[n].active = true;
    }
    proj.write_grid_row_header(&row_headers[n], grid_page.getRow(), n);
    proj.sync_grid(n);
  }

  proj.select_grid(old_grid);
}

void MCLActions::write_tracks(int column, int row, uint8_t *slot_select_array) {
  DEBUG_PRINT_FN();
  if ((mcl_cfg.chain_mode > 0) && (MidiClock.state == 2)) {
    FOREACH_ELEKTRON_DEVICE([](uint8_t i, auto elektron_dev) {
      if (elektron_dev->canReadWorkspaceKit()) {
        auto kit = elektron_dev->getKit();
        if (kit != nullptr &&
            elektron_dev->currentKit != kit->getPosition()) {
          elektron_dev->getBlockingKit(0x7F);
        }
      }
    });
    prepare_next_chain(row, slot_select_array);
    return;
  }
  FOREACH_ELEKTRON_DEVICE([](uint8_t i, auto elektron_dev) {
    if (elektron_dev->canReadWorkspaceKit() ) {
      elektron_dev->getBlockingKit(0x7F);
    }
  });

  send_tracks_to_devices(slot_select_array);
}

void MCLActions::prepare_next_chain(int row, uint8_t *slot_select_array) {
  DEBUG_PRINT_FN();
  EmptyTrack empty_track;
  uint8_t q;
  uint8_t old_grid = proj.get_grid();

  //  if (MidiClock.state != 2) {
  //  q = 0;
  //  } else {
  if (gridio_param4.cur == 0) {
    q = 4;
  } else {
    q = 1 << gridio_param4.cur;
  }
  if (q < 4) {
    q = 4;
  }

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
    proj.select_grid(grid_idx);
    if ((slot_select_array[i] == 0) || (track_type == 255)) {
      // Ignore slots that are not device supported.
      slot_select_array[i] = 0;
      return;
    }
    auto device_track = empty_track.load_from_grid(track_idx, row);
    if (device_track == nullptr || device_track->active != track_type) {
      empty_track.clear();
      device_track = empty_track.init_track_type(track_type);
      send_machine[i] = 1;
    } else {
      send_machine[i] = 0;
    }
    device_track->store_in_mem(track_idx);
  });

  uint16_t next_step;
  if (q > 0) {
    next_step = (MidiClock.div16th_counter / q) * q + q;
  } else {
    next_step = MidiClock.div16th_counter + 1;
  }
  DEBUG_PRINTLN(F("q"));
  DEBUG_PRINTLN(q);
  DEBUG_PRINTLN(F("write step"));
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(next_step);
  DEBUG_PRINTLN(F("setting transition"));
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {

    if (slot_select_array[n] > 0) {
      // transition_level[n] = gridio_param3.getValue();
      transition_level[n] = 0;
      next_transitions[n] = next_step;
      chains[n].row = row;
      chains[n].loops = 1;
      // if (grid_page.active_slots[n] < 0) {
      grid_page.active_slots[n] = 0x7FFF;
      // }
    }
  }
  calc_next_transition();
  calc_latency(&empty_track);

  proj.select_grid(old_grid);
}

void MCLActions::send_tracks_to_devices(uint8_t *slot_select_array) {
  DEBUG_PRINT_FN();

  uint8_t select_array[NUM_SLOTS];
  //Take a copy, because we call GUI.loop later.
  memcpy(&select_array, slot_select_array, NUM_SLOTS);

  EmptyTrack empty_track;
  EmptyTrack empty_track2;

  uint8_t first_note = 255;

  uint8_t mute_states[NUM_SLOTS];
  uint8_t send_masks[NUM_SLOTS] = {0};

  uint8_t old_grid = proj.get_grid();

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){

    if (seq_track) {
      mute_states[i] = seq_track->mute_state;
      seq_track->mute_state = SEQ_MUTE_ON;
    }

    if ((select_array[i] == 0) || (track_type == 255)) {
      // Ignore slots that are not device supported.
      select_array[i] = 0;
      return;
    }

    proj.select_grid(grid_idx);

    grid_page.active_slots[i] = grid_page.getRow();

    if (first_note == 255) {
      first_note = i;
    }

    auto *ptrack = empty_track.load_from_grid(track_idx, grid_page.getRow());

    if (!ptrack) {
      return;
    } // read failure

    ptrack->chain.store_in_mem(i, &(chains[0]));
    if (ptrack->active != track_type) {
      // empty, or incompatible
    } else {
      DEBUG_DUMP(i);
      ptrack->load_immediate(track_idx, seq_track);
      if (track_type != 255) {
        send_masks[i] = 1;
      }
    }
  });

  if (write_original == 1) {
    DEBUG_PRINTLN(F("write original"));
    //     MD.kit.origPosition = md_track->origPosition;
    for (uint8_t c = 0; c < 17; c++) {
      MD.kit.name[c] =
          toupper(grid_page.row_headers[grid_page.cur_row].name[c]);
    }
  }

  /*Send the encoded kit to the devices via sysex*/
  uint16_t myclock = slowclock;
  uint16_t latency_ms = 0;
  FOREACH_ELEKTRON_DEVICE([&](auto i, auto elektron_dev) {
#ifndef EXT_TRACKS
      if (i > 0) {
        break;
      }
#endif
      latency_ms += elektron_dev->sendKitParams(send_masks + i * GRID_WIDTH,
                                                &empty_track);
  });

  // switch back to old grid before driving the GUI loop
  proj.select_grid(old_grid);
  // note, do not re-enter grid_task -- stackoverflow
  GUI.removeTask(&grid_task);
  while (clock_diff(myclock, slowclock) < latency_ms) {
    GUI.loop();
  }
  GUI.addTask(&grid_task);

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
    if (seq_track) {
      seq_track->mute_state = mute_states[i];
    }
  });

  /*All the tracks have been sent so clear the write queue*/
  write_original = 0;
  if ((mcl_cfg.chain_mode == 0) || (mcl_cfg.chain_mode == 2)) {
    next_transition = (uint16_t)-1;
    return;
  }

  // Cache

  cache_next_tracks(select_array, &empty_track, &empty_track2);

  // in_sysex = 0;
  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
    if ((select_array[i] > 0) && (grid_page.active_slots[i] >= 0)) {
      transition_level[i] = 0;
      next_transitions[i] = MidiClock.div16th_counter -
                            (seq_track->step_count *
                             seq_track->get_speed_multiplier());
      calc_next_slot_transition(i);
    }
  });

  calc_next_transition();
  calc_latency(&empty_track);
}

void MCLActions::cache_next_tracks(uint8_t *slot_select_array,
                                   EmptyTrack *empty_track,
                                   EmptyTrack *empty_track2, bool gui_update) {
  DEBUG_PRINT_FN();

  uint8_t old_grid = proj.get_grid();

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
    if (slot_select_array[i] > 0) {

      if (gui_update) {
        handleIncomingMidi();
        if (i % 8 == 0) {
          if (GUI.currentPage() != &grid_write_page) {
            GUI.loop();
          }
        }
      }

      if (track_idx == 255) {
        return;
      }

      proj.select_grid(grid_idx);

      auto *ptrack = empty_track->load_from_grid(track_idx, chains[i].row);
      if (ptrack == nullptr || !ptrack->is_active() ||
          track_type != ptrack->active) {
        return;
      }

      auto *pmem_track = empty_track2->load_from_mem(track_idx, track_type);
      if (pmem_track != nullptr && pmem_track->active == ptrack->active) {
        // track type matched.
        auto *psound = ptrack->get_sound_data_ptr();
        auto *pmem_sound = pmem_track->get_sound_data_ptr();
        auto szsound = ptrack->get_sound_data_size();
        auto szmem_sound = pmem_track->get_sound_data_size();

        if (!psound || !pmem_sound || szsound != szmem_sound) {
          // something's wrong, don't send
        } else if (memcmp(psound, pmem_sound, szsound) != 0) {
          send_machine[i] = 0;
        } else {
          send_machine[i] = 1;
        }
      }
      ptrack->store_in_mem(track_idx);
    }
  });

  proj.select_grid(old_grid);
}

void MCLActions::calc_next_slot_transition(uint8_t n) {

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(n);
  //  DEBUG_PRINTLN(next_transitions[n]);
  if (chains[n].loops == 0) {
    next_transitions[n] = -1;
    return;
  }

  uint8_t grid_idx, track_idx, track_type, dev_idx;

  SeqTrack *seq_track =
          get_dev_slot_info(n, &grid_idx, &track_idx, &track_type, &dev_idx);

  uint16_t next_transitions_old = next_transitions[n];
  float len;

  float l = chains[n].length;
  len = (float)chains[n].loops * l *
        (float)seq_track->get_speed_multiplier();
  while (len < 4) {
    if (len < 1) {
      len = 4;
      transition_offsets[n] = 0;
    } else {
      len = len * 2;
    }
  }

  // Last offset must be carried over to new offset.
  transition_offsets[n] += (float)(len - (uint16_t)(len)) * 12;
  if (transition_offsets[n] >= 12) {
    transition_offsets[n] = transition_offsets[n] - 12;
    len++;
  }

  DEBUG_DUMP(len - (uint16_t)(len));
  DEBUG_DUMP(transition_offsets[n]);
  next_transitions[n] += (uint16_t)len;

  // check for overflow and make sure next nearest step is greater than
  // midiclock counter
  while ((next_transitions[n] >= next_transitions_old) &&
         (next_transitions[n] < MidiClock.div16th_counter)) {
    next_transitions[n] += (uint16_t)len;
  }
  DEBUG_PRINTLN(next_transitions[n]);
}

void MCLActions::calc_next_transition() {
  next_transition = (uint16_t)-1;
  DEBUG_PRINT_FN();
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (grid_page.active_slots[n] >= 0) {
      if ((chains[n].loops > 0) &&
          (chains[n].row != grid_page.active_slots[n])) {
        if (MidiClock.clock_less_than(next_transitions[n], next_transition)) {
          DEBUG_PRINTLN(n);
          DEBUG_PRINTLN(grid_page.active_slots[n]);
          DEBUG_PRINTLN(chains[n].row);
          DEBUG_PRINTLN(next_transitions[n]);
          DEBUG_PRINTLN(F(" "));
          next_transition = next_transitions[n];
        }
      }
    }
  }
  nearest_bar = next_transition / 16 + 1;
  nearest_beat = next_transition % 4 + 1;
  // next_transition = next_transition % 16;

  DEBUG_PRINTLN(F("current_step"));
  DEBUG_PRINTLN(MidiClock.div16th_counter);
  DEBUG_PRINTLN(F("nearest step"));
  DEBUG_PRINTLN(next_transition);
}

void MCLActions::calc_latency(DeviceTrack *empty_track) {

  for (uint8_t a = 0; a < NUM_GRIDS; a++) {
    dev_latency[a].latency = 0;
  }

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
    if ((grid_page.active_slots[i] < 0) || (send_machine[i] != 0) || next_transitions[i] != next_transition || track_idx == 255) {
      return;
    }
    auto *ptrack = empty_track->load_from_mem(track_idx, track_type);
    if (ptrack == nullptr || !ptrack->is_active() || track_type != ptrack->active) {
      return;
    }
    dev_latency[grid_idx].latency = ptrack->calc_latency(i);
  });

  float tempo = MidiClock.get_tempo();
  //  div32th_per_second: tempo / 60.0f * 4.0f * 2.0f = tempo * 8 / 60
  float div32th_per_second = tempo * 0.133333333333f;
  //  div32th_per_second: tempo / 60.0f * 4.0f * 2.0f * 6.0f = tempo * 8 / 10
  float div192th_per_second = tempo * 0.8f;

  FOREACH_DEVICE([&](auto i, auto dev) {
    dev_latency[i].latency = 0;

    float bytes_per_second_uart1 = dev->uart->speed / 10.0f;
    float latency_in_seconds = dev_latency[i].latency / bytes_per_second_uart1;
    dev_latency[i].div32th_latency =
        round(div32th_per_second * latency_in_seconds) + 1;
    dev_latency[i].div192th_latency =
        round(div192th_per_second * latency_in_seconds) + 3;
  });
}

MCLActions mcl_actions;
