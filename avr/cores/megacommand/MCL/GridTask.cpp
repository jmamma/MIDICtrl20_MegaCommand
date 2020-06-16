#include "EmptyTrack.h"
#include "GridTask.h"
#include "MCL.h"

#define DIV16_MARGIN 8
#define HANDLE_GROUPS 1

void GridTask::setup(uint16_t _interval) { interval = _interval; }

void GridTask::destroy() {}

uint16_t compare_memory(void* _pa, void* _pb, uint16_t len)
{
  uint8_t *pa = (uint8_t*)_pa, *pb = (uint8_t*)_pb;
  for (uint16_t pos = 0; pos < len; ++pos) {
    if(*pa++ != *pb++) return pos;
  }
  return len;
}

void GridTask::run() {
  EmptyTrack empty_track;
  MDTrack *md_track = (MDTrack *)&empty_track;

  GUI.removeTask(&grid_task);
  while(true) {


  DEBUG_PRINT_FN();

  uint16_t size =
      sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);

  EmptyTrack empty_track2;
  MDTrack *md_track2 = (MDTrack *)&empty_track2;

  // Compare slots from grid with load from memory BANK2
  for (uint8_t n = 11; n < 12; n++) {
    md_track->load_from_mem(n);
    // Do nothing for 2 seconds.
    uint32_t start_clock = slowclock;
    while (clock_diff(start_clock, slowclock) < 500) {
      GUI.loop();
    }
    md_track2->load_from_mem(n);
    uint16_t cbyte = compare_memory((md_track), (md_track2), size);
    if (cbyte != size) {
      char str[16];
      char pos[5];
      char val[5];
      itoa(n, str, 10);
      itoa(cbyte, pos, 10);
      itoa(((uint8_t*)md_track)[cbyte], val, 16);
      strcat(str, " ");
      strcat(str, pos);
      strcat(str, " ");
      strcat(str, val);
      itoa(((uint8_t*)md_track2)[cbyte], val, 16);
      strcat(str, " ");
      strcat(str, val);
      oled_display.textbox("HIT ", str);

    } else {
    }   
  }
  }
  GUI.addTask(&grid_task);
}

GridTask grid_task(0);
