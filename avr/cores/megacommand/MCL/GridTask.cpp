#include "MCL_impl.h"

#define DIV16_MARGIN 8

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::destroy() {}

void GridTask::run() {
  //  DEBUG_PRINTLN(MidiClock.div32th_counter / 2);
  //  A4Track *a4_track = (A4Track *)&temp_track;
  //  ExtTrack *ext_track = (ExtTrack *)&temp_track;
  if (MidiClock.state != 2) {
    return;
  }

  EmptyTrack empty_track;
  EmptyTrack empty_track2;

  MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
  };
  bool send_device[2] = {0};

  int slots_changed[NUM_SLOTS];
  uint8_t track_select_array[NUM_SLOTS] = {0};

  bool send_ext_slots = false;
  bool send_md_slots = false;

  uint8_t div32th_margin = 8;

  uint32_t div32th_counter;
  if ((mcl_cfg.chain_mode == 0) ||
      (mcl_actions.next_transition == (uint16_t)-1)) {
    return;
  }

  // Get within four 16th notes of the next transition.
  if (!MidiClock.clock_less_than(MidiClock.div32th_counter + div32th_margin,
                                 (uint32_t)mcl_actions.next_transition * 2)) {

    DEBUG_PRINTLN(F("Preparing for next transition:"));
    DEBUG_DUMP(MidiClock.div16th_counter);
    DEBUG_DUMP(mcl_actions.next_transition);
    // Transition window
    div32th_counter = MidiClock.div32th_counter + div32th_margin;
  } else {
    return;
  }

  GUI.removeTask(&grid_task);

  FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){

    slots_changed[i] = -1;

    if ((mcl_actions.chains[i].loops == 0) || (grid_page.active_slots[i] < 0))
      return;

    uint32_t next_transition = (uint32_t)mcl_actions.next_transitions[i] * 2;

    // If next_transition[i] is less than or equal to transition window, then
    // flag track for transition.
    if (MidiClock.clock_less_than(div32th_counter, next_transition))
      return;

    slots_changed[i] = mcl_actions.chains[i].row;
    if ((mcl_actions.chains[i].row != grid_page.active_slots[i]) ||
        (mcl_cfg.chain_mode == 2)) {

      SeqTrack *seq_track = mcl_actions.get_dev_slot_info(
          i, &grid_idx, &track_idx, &track_type, &dev_idx);

      auto *pmem_track = empty_track.load_from_mem(track_idx, track_type);
      if (pmem_track != nullptr) {
        slots_changed[i] = mcl_actions.chains[i].row;
        track_select_array[i] = 1;
        memcpy(&mcl_actions.chains[i], &pmem_track->chain, sizeof(GridChain));
        if (pmem_track->active) {
          send_device[dev_idx] = true;
        }
      }
    }
    // Override chain data if in manual or random mode
    if (mcl_cfg.chain_mode == 2) {
      mcl_actions.chains[i].loops = 0;
    } else if (mcl_cfg.chain_mode == 3) {
      mcl_actions.chains[i].loops = random(1, 8);
      mcl_actions.chains[i].row =
          random(mcl_cfg.chain_rand_min, mcl_cfg.chain_rand_max);
    }
  });

  uint32_t div192th_total_latency = 0;
  for (uint8_t a = 0; a < NUM_GRIDS; a++) {
    div192th_total_latency = mcl_actions.dev_latency[a].div192th_latency;
  }
  DEBUG_PRINTLN(F("sending tracks"));
  bool wait;
  for (int8_t c = 2 - 1; c >= 0; c--) {
    wait = true;

    FOREACH_GRID_TRACK([&](auto i, auto grid_idx, auto track_idx, auto dev_idx, auto dev, auto elektron_dev, auto seq_track, auto track_type){
      if ((track_idx == 255) || (dev_idx != c))
        return;

      // Wait on first track of each device;
      if (wait && send_device[c] && mcl_actions.dev_latency[c].latency > 0) {

        uint32_t go_step =
            mcl_actions.next_transition * 12 - div192th_total_latency;
        div192th_total_latency -= mcl_actions.dev_latency[dev_idx].latency;
        uint32_t diff;

        while (((diff = MidiClock.clock_diff_div192(MidiClock.div192th_counter,
                                                    go_step)) != 0) &&
               (MidiClock.div192th_counter < go_step) &&
               (MidiClock.state == 2)) {
          if (diff > 8) {

            handleIncomingMidi();
            GUI.loop();
          }
        }
      }
      wait = false;

      if (slots_changed[i] < 0)
        return;

      auto *pmem_track = empty_track.load_from_mem(track_idx, track_type);

      if (pmem_track->is_active()) {
        pmem_track->transition_load(track_idx, seq_track, i);

      } else {
        pmem_track->transition_clear(track_idx, seq_track);
      }

      grid_page.active_slots[i] = slots_changed[i];
    });
  }
  if (mcl_cfg.chain_mode != 2) {

    bool update_gui = true;
    mcl_actions.cache_next_tracks(track_select_array, &empty_track,
                                  &empty_track2, update_gui);

    // Once tracks are cached, we can calculate their next transition
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      if (track_select_array[n] > 0) {
        mcl_actions.calc_next_slot_transition(n);
      }
    }

    mcl_actions.calc_next_transition();
    mcl_actions.calc_latency(&empty_track);
  } else {
    mcl_actions.next_transition = (uint16_t)-1;
  }

  GUI.addTask(&grid_task);
}

GridTask grid_task(0);
