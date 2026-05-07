#include "volume.h"

/*************************************************************************************************************
 * Maps a value from one range to another.
 * The input value must be within the given input range.
 * If the parameters are invalid, NAN is returned.
 *
 * @param x       The value to map.
 * @param in_min  Minimum input value.
 * @param in_max  Maximum input value.
 * @param out_min Minimum output value.
 * @param out_max Maximum output value.
 * @return The mapped value as double, or NAN if an error occurs.
 *************************************************************************************************************/
double mapDouble(int x, int in_min, int in_max, double out_min, double out_max)
{
  if (in_min >= in_max) return NAN;
  if (out_min >= out_max) return NAN;
  if (x < in_min || x > in_max) return NAN;

  return (out_max - out_min) / (double)(in_max - in_min) * (x - in_min) + out_min;
}

/*************************************************************************************************************
 * Evaluates the volume level based on the given decibel value and threshold ranges.
 * This function implements hysteresis to prevent rapid toggling between volume levels when the decibel value is close to the thresholds.
 * 
 * The thresholds must be strictly increasing:
 * minValidDb < acceptableThreshold < elevatedThreshold < maxValidDb
 *
 * @param db                    The decibel value to evaluate.
 * @param lastVolumeLevelState  The previous volume level state, used for hysteresis.
 * @param minValidDb            Minimum valid decibel value.
 * @param acceptableThreshold   Upper bound for acceptable volume.
 * @param elevatedThreshold     Upper bound for elevated volume.
 * @param maxValidDb            Maximum valid decibel value.
 * @return The corresponding VolumeLevel, or VOLUME_UNKNOWN if parameters are invalid
 *************************************************************************************************************/
VolumeLevel getVolumeLevelHysteresis(int db, VolumeLevel lastVolumeLevelState, int minValidDb, int acceptableThreshold, int elevatedThreshold, int maxValidDb, int hysteresis)
{
  if (minValidDb >= acceptableThreshold || acceptableThreshold >= elevatedThreshold || elevatedThreshold >= maxValidDb) return VOLUME_UNKNOWN;
  if (db < minValidDb || db > maxValidDb) return VOLUME_UNKNOWN;
  
  VolumeLevel currentVolumeLevelState = lastVolumeLevelState; // hold the last state as default, only change if thresholds are crossed with hysteresis
  switch (lastVolumeLevelState)
  {
    case VOLUME_ACCEPTABLE:
      if(db > (acceptableThreshold + hysteresis)) currentVolumeLevelState = VOLUME_ELEVATED;
      break;
    case VOLUME_ELEVATED:
      if(db < (acceptableThreshold - hysteresis)) currentVolumeLevelState = VOLUME_ACCEPTABLE;
      else if(db > (elevatedThreshold + hysteresis)) currentVolumeLevelState = VOLUME_DANGEROUS;
      break;
    case VOLUME_DANGEROUS:
      if(db < (elevatedThreshold - hysteresis)) currentVolumeLevelState = VOLUME_ELEVATED;
      break;
    default:
      if(db < acceptableThreshold) currentVolumeLevelState = VOLUME_ACCEPTABLE;
      else if(db < elevatedThreshold) currentVolumeLevelState = VOLUME_ELEVATED;
      else currentVolumeLevelState = VOLUME_DANGEROUS;
      break;
  }
  return currentVolumeLevelState;
}