#include "MCL_impl.h"
//#include "MCLSd.h"

#define A4_SOUND_LENGTH 0x19F

uint16_t A4Track::calc_latency(uint8_t tracknumber) {
  uint16_t a4_latency = A4_SOUND_LENGTH;
  return a4_latency;
}

void A4Track::transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  uint8_t n = slotnumber;
  if ((mcl_actions.send_machine[slotnumber] == 0)) {
    DEBUG_PRINTLN("here");
    DEBUG_PRINTLN("send a4 sound");
    sound.toSysex();
  }
  ExtTrack::transition_load(tracknumber, seq_track, slotnumber);
}

bool A4Track::get_track_from_sysex(uint8_t tracknumber) {
  auto ret = Analog4.getBlockingSoundX(tracknumber);
  if (ret) {
    sound.fromSysex(Analog4.midi);
  }
  return ret;
}

bool A4Track::store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge,
                            bool online) {

  active = A4_TRACK_TYPE;

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("storing a4 track");
  uint32_t len;

  // [>analog 4 tracks<]
#ifdef EXT_TRACKS
  if (online) {
    memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));

    chain.length = mcl_seq.ext_tracks[tracknumber].length;
    chain.speed = mcl_seq.ext_tracks[tracknumber].speed;
  }
#endif
  ret = proj.write_grid((uint8_t *)this, sizeof(A4Track), tracknumber, row);
  if (!ret) {
    return false;
  }
  return true;
}

// !! Note do not rely on editor code lint errors -- these are for 32bit/64bit
// x86 sizes Do compile with avr-gcc and observe the error messages

//__SIZE_PROBE<sizeof(MDSeqTrackData)> mdseqtrackdata;
//__SIZE_PROBE<sizeof(MDSeqTrackData)> mdseqtrackdata;
//__SIZE_PROBE<sizeof(a4sound_t)> sza4t;

//__SIZE_PROBE<sizeof(A4Track)> sza4;
//__SIZE_PROBE<sizeof(MDTrack)> szmd;
//__SIZE_PROBE<sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine)>
//szmd_2;
//__SIZE_PROBE<sizeof(EmptyTrack)> szempty;
//__SIZE_PROBE<FX_TRACK_LEN> szfx;
//__SIZE_PROBE<sizeof(GridTrack) + sizeof(MDFXData)> szfx_2;

//__SIZE_PROBE<BANK1_MD_TRACKS_START> addr_md;
//__SIZE_PROBE<BANK1_A4_TRACKS_START> addr_a4;
//__SIZE_PROBE<BANK1_FILE_ENTRIES_END> addr_end;

//__SIZE_PROBE<sizeof(MNMClass)> sz_mnm_class;
//__SIZE_PROBE<sizeof(MDClass)> sz_md_class;
//__SIZE_PROBE<sizeof(A4Class)> sz_a4_class;
