#ifndef VOLUME_H
#define VOLUME_H

#include <math.h>   // für NAN

// volume level enum
typedef enum { VOLUME_ACCEPTABLE, VOLUME_ELEVATED, VOLUME_DANGEROUS, VOLUME_UNKNOWN} VolumeLevel;

// maps a value from one range to another
double mapDouble(int x, int in_min, int in_max, double out_min, double out_max);

// Evaluates the volume level with hysteresis
VolumeLevel getVolumeLevelHysteresis(int db, VolumeLevel lastVolumeLevelState, int minValidDb, int acceptableThreshold, int elevatedThreshold, int maxValidDb, int hysteresis);

#endif // VOLUME_H