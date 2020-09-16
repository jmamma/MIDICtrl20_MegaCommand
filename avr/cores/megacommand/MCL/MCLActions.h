/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "MCLActionsEvents.h"
#include "lambda.h"

#define PATTERN_STORE 0
#define PATTERN_UDEF 254
#define STORE_IN_PLACE 0
#define STORE_AT_SPECIFIC 254

#define TRANSITION_NORMAL 0
#define TRANSITION_MUTE 2
#define TRANSITION_UNMUTE 3

class DeviceLatency {
public:
  uint16_t latency;
  uint8_t div32th_latency;
  uint8_t div192th_latency;
};

class ChainModeData {
public:
  DeviceLatency dev_latency[2];

  GridChain chains[NUM_SLOTS];

  uint16_t md_latency;

  uint16_t next_transition = (uint16_t)-1;

  uint16_t nearest_bar;
  uint8_t nearest_beat;

  uint16_t next_transitions[NUM_SLOTS];
  uint8_t transition_offsets[NUM_SLOTS];
  uint8_t send_machine[NUM_SLOTS];
  uint8_t transition_level[NUM_SLOTS];
};


typedef void (*foreach_gridtrack_fn)(uint8_t, uint8_t, uint8_t, uint8_t, MidiDevice*, ElektronDevice*, SeqTrack*, uint8_t);
typedef void (*foreach_device_fn)(uint8_t, MidiDevice*);
typedef void (*foreach_elektron_device_fn)(uint8_t, ElektronDevice*);

class MCLActions : public ChainModeData {
public:
  uint8_t do_kit_reload;
  int writepattern;
  uint8_t write_original = 0;

  uint8_t patternswitch = PATTERN_UDEF;

  uint16_t start_clock16th = 0;
  uint32_t start_clock32th = 0;
  uint32_t start_clock96th = 0;
  uint8_t store_behaviour;

  MCLActions() {}

  void setup();

  uint8_t get_grid_idx(uint8_t slot_number);
  GridDeviceTrack *get_grid_dev_track(uint8_t slot_number, uint8_t *id, uint8_t *dev_idx);

  SeqTrack *get_dev_slot_info(uint8_t slot_number, uint8_t *grid_idx, uint8_t *track_idx, uint8_t *track_type, uint8_t *dev_idx);

  void send_globals();
  void switch_global(uint8_t global_page);
  void kit_reload(uint8_t pattern);

  void store_tracks_in_mem(int column, int row, uint8_t *slot_select_array,
                           uint8_t merge);

  void write_tracks(int column, int row, uint8_t *slot_select_array);
  void send_tracks_to_devices(uint8_t *slot_select_array);
  void prepare_next_chain(int row, uint8_t *slot_select_array);

  void cache_next_tracks(uint8_t *slot_select_array, EmptyTrack *empty_track,
                         EmptyTrack *empty_track2, bool update_gui = false);
  void calc_next_slot_transition(uint8_t n);
  void calc_next_transition();
  void calc_latency(DeviceTrack *empty_track);

  void __attribute__((noinline)) foreach_device(foreach_device_fn fn) {
    MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
    };

    for (uint8_t i = 0; i < NUM_GRIDS; ++i) {
      fn(i, devs[i]);
    }
  }

  void __attribute__((noinline)) foreach_elektron_device(foreach_elektron_device_fn fn) {
    MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
    };

    for (uint8_t i = 0; i < NUM_GRIDS; ++i) {
      auto elektron_dev = devs[i]->asElektronDevice();
      if (elektron_dev) {
        fn(i, elektron_dev);
      }
    }
  }

  void __attribute__((noinline)) foreach_gridtrack(foreach_gridtrack_fn fn) { 
    MidiDevice *devs[2] = {
      midi_active_peering.get_device(UART1_PORT),
      midi_active_peering.get_device(UART2_PORT),
    };
    ElektronDevice *elektron_devs[2] = {
      devs[0]->asElektronDevice(),
      devs[1]->asElektronDevice(),
    };
    uint8_t grid_idx, track_idx, track_type, dev_idx;
    for (uint8_t i = 0; i < NUM_SLOTS; ++i) {
      SeqTrack *seq_track = get_dev_slot_info(i, &grid_idx, &track_idx, &track_type, &dev_idx);
      fn(i, grid_idx, track_idx, dev_idx, devs[grid_idx], elektron_devs[grid_idx], seq_track, track_type);
    }
  }

};

extern MCLActionsCallbacks mcl_actions_callbacks;
extern MCLActionsMidiEvents mcl_actions_midievents;

extern MCLActions mcl_actions;

struct Lambda {
  template<typename T>
  static void lambda_ptr_foreach_gridtrack_exec(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, MidiDevice* p5, ElektronDevice* p6, SeqTrack* p7, uint8_t p8) {
    (*(T*)fn())(p1,p2,p3,p4,p5,p6,p7,p8);
  }

  template<typename T>
  static void lambda_ptr_foreach_device_exec(uint8_t p1, MidiDevice* p2) {
    (*(T*)fn())(p1,p2);
  }

  template<typename T>
  static void lambda_ptr_foreach_elektron_device_exec(uint8_t p1, ElektronDevice* p2) {
    (*(T*)fn())(p1,p2);
  }


  template<typename T>
  static foreach_gridtrack_fn ptr_foreach_gridtrack_fn(T& t) {
    fn(&t);
    return (foreach_gridtrack_fn) lambda_ptr_foreach_gridtrack_exec<T>;
  }
  template<typename T>
  static foreach_elektron_device_fn ptr_foreach_elektron_device_fn(T& t) {
    fn(&t);
    return (foreach_elektron_device_fn) lambda_ptr_foreach_elektron_device_exec<T>;
  }
  template<typename T>
  static foreach_device_fn ptr_foreach_device_fn(T& t) {
    fn(&t);
    return (foreach_device_fn) lambda_ptr_foreach_device_exec<T>;
  }

  static void* fn(void* new_fn = nullptr) {
    static void* fn;
    if (new_fn != nullptr)
      fn = new_fn;
    return fn;
  }
};

#define FOREACH_GRID_TRACK(...) \
{ \
  auto __foreach_grid_tracks_fn = __VA_ARGS__; \
  mcl_actions.foreach_gridtrack(Lambda::ptr_foreach_gridtrack_fn(__foreach_grid_tracks_fn)); \
}

#define FOREACH_DEVICE(...) \
{ \
  auto __foreach_device_fn = __VA_ARGS__; \
  mcl_actions.foreach_device(Lambda::ptr_foreach_device_fn(__foreach_device_fn)); \
}

#define FOREACH_ELEKTRON_DEVICE(...) \
{ \
  auto __foreach_elektron_device_fn = __VA_ARGS__; \
  mcl_actions.foreach_elektron_device(Lambda::ptr_foreach_elektron_device_fn(__foreach_elektron_device_fn)); \
}
