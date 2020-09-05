#include "MCL_impl.h"

DeviceTrack* DeviceTrack::init_track_type(uint8_t track_type) {
  DEBUG_PRINT_FN();
  switch (track_type) {
  case A4_TRACK_TYPE_270:
  case MD_TRACK_TYPE_270:
  case EXT_TRACK_TYPE_270:
/*    if (!data) {
      // no space for track upgrade
      return false;
    } else {
      // TODO upgrade right here
      return true;
    } */
    return nullptr;
    break;
  case EMPTY_TRACK_TYPE:
    ::new(this) EmptyTrack;
    break;
  case MD_TRACK_TYPE:
    ::new(this) MDTrack;
    break;
  case A4_TRACK_TYPE:
    ::new(this) A4Track;
    break;
  case EXT_TRACK_TYPE:
    ::new(this) ExtTrack;
    break;
  case MDFX_TRACK_TYPE:
    ::new(this) MDFXTrack;
    break;
  //case MNM_TRACK_TYPE:
    //::new(this) MNMTrack;
    //break;
  }
  return this;
}

DeviceTrack* DeviceTrack::load_from_grid(uint8_t column, uint16_t row) {
  if (!GridTrack::load_from_grid(column, row)) {
    return nullptr;
  }

  // header read successfully. now reconstruct the object.
  auto ptrack = init_track_type(active);

  // virtual functions are ready
  uint32_t len = ptrack->get_track_size();

  DEBUG_PRINTLN(len);

  if(!proj.read_grid(ptrack, len, column, row)) {
    DEBUG_PRINTLN("read failed");
    return nullptr;
  }

  auto ptrack2 = ptrack->init_track_type(active);

  return ptrack2;
}

