#include "MCL_impl.h"

void MNMTrack::load_immediate(uint8_t tracknumber) {
  DEBUG_PRINT_FN();
  MNM.insertMachineInKit(tracknumber, &(machine));
  load_seq_data(tracknumber);
  store_in_mem(tracknumber);
}

void MNMTrack::get_machine_from_kit(uint8_t tracknumber) {
  DEBUG_PRINT_FN();
  memcpy(machine.params, MNM.kit.parameters[tracknumber], 72);

  machine.track = tracknumber;
  machine.level = MNM.kit.levels[tracknumber];
  machine.model = MNM.kit.models[tracknumber];

  memcpy(machine.modifier.destPage, MNM.kit.destPages + tracknumber, 6*2);
  memcpy(machine.modifier.destParam, MNM.kit.destParams + tracknumber, 6*2);
  memcpy(machine.modifier.range, MNM.kit.destRanges + tracknumber, 6*2);
  machine.modifier.HPKeyTrack = MNM.kit.hpKeyTrack;
  machine.modifier.LPKeyTrack = MNM.kit.lpKeyTrack;
  machine.modifier.mirrorLR = MNM.kit.mirrorLR;
  machine.modifier.mirrorUD = MNM.kit.mirrorUD;

  machine.trig.legatoAmp = MNM.kit.trigLegatoAmp;
  machine.trig.legatoFilter = MNM.kit.trigLegatoFilter;
  machine.trig.legatoLFO = MNM.kit.trigLegatoLFO;
  machine.trig.portamento = MNM.kit.trigPortamento;
  machine.trig.track = MNM.kit.trigTracks[tracknumber];

  machine.type = MNM.kit.types[tracknumber];
}

bool MNMTrack::store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge,
                                  bool online) {

  DEBUG_PRINT_FN();
  active = MNM_TRACK_TYPE;

  if (tracknumber != 255 && online == true) {
    get_machine_from_kit(tracknumber);

    chain.length = mcl_seq.ext_tracks[tracknumber].length;
    chain.speed = mcl_seq.ext_tracks[tracknumber].speed;

    if (merge > 0) {
      // TODO decode MNM track data from a pattern...
    } else {
      memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));
    }
  }
  // Write data to sd
  return proj.write_grid((uint8_t *)(this), sizeof(MNMTrack), tracknumber, row);
}

