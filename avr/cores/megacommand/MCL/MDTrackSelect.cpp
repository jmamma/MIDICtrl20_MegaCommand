#include "MCL_impl.h"

void MDTrackSelect::start() {

}

bool MDTrackSelect::on() {
  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  MD.activate_track_select();
  sysex->addSysexListener(this);

  state = true;
  return true;
}

bool MDTrackSelect::off() {
  sysex->removeSysexListener(this);
  if (!state) {
    return false;
   }
  state = false;
  if (!MD.connected) {
    return false;
  }
  MD.deactivate_track_select();
  return true;
}

void MDTrackSelect::end() { }


void MDTrackSelect::end_immediate() {
  if (!state) {
    return;
  }

 if (sysex->getByte(0) != ids[0]) { return; }
 if (sysex->getByte(1) != ids[1]) { return; }

 MD.currentTrack = sysex->getByte(2);
 return;
}

MDTrackSelect md_track_select;
