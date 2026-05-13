#include <Arduino.h>
#include <math.h> // for NAN
#include "volume.h"

// pin definitions
typedef enum {
  POTI_PIN = A0, 
  BUSHBUTTON_PIN = 2,
  GREEN_LED_PIN = 13, 
  YELLOW_LED_PIN = 12, 
  RED_LED_PIN = 7 
} PinId;

// structures
typedef struct {
  int potiValue;
  int bushbuttonValue;
} InputState;

typedef struct {
  bool potiChanged;
  bool bushbuttonChanged;
} InputDiff;

typedef struct {
  int db;                     
  VolumeLevel volumeLevel;    
} DerivedState;

typedef struct {
  bool dbChanged;
  bool volumeLevelChanged;
} DerivedDiff;

// function prototypes
void readInput(InputState* state)
{
  state->potiValue = analogRead(POTI_PIN);
  state->bushbuttonValue = digitalRead(BUSHBUTTON_PIN);
}

InputDiff diffInput(const InputState* current, const InputState* last)
{
  InputDiff diff = {0};

  diff.bushbuttonChanged = current->bushbuttonValue != last->bushbuttonValue;

  return diff;
}

DerivedDiff diffDerived(const DerivedState* current, const DerivedState* last)
{
  DerivedDiff diff = {0};

  //diff.dbChanged = current->db != last->db;
  diff.volumeLevelChanged = current->volumeLevel != last->volumeLevel;

  return diff;
}

// function declarations

void handleVolumeTransition(VolumeLevel lastVolumeLevelState, VolumeLevel currentVolumeLevelState);

void setup() {
  //inputs
  pinMode(POTI_PIN, INPUT);
  pinMode(BUSHBUTTON_PIN, INPUT_PULLUP); // Pull-up-Widerstand aktiviert, da Taster gegen GND geschaltet ist
  //outputs
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  //output start values
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.begin(9600);
}

void loop() 
{
  const int POTI_MIN_VALUE = 0;
  const int POTI_MAX_VALUE = 1023;
  const int VOLUME_MIN_DB = 30;
  const int VOLUME_MAX_DB = 110;
  const int VOLUME_ACCEPTABLE_THRESHOLD_DB = 60;
  const int VOLUME_ELEVATED_THRESHOLD_DB = 90;
  const int HYSTERESIS_DB = 2;

  static InputState currentInputState, lastInputState;
  static DerivedState currentDerivedState, lastDerivedState;
  static bool derivedInitialized = false; // flag to check if derived state has been initialized at least once

  // read all pysical inputs
  readInput(&currentInputState);
  // recognize physical input changes
  InputDiff inputDiff = diffInput(&currentInputState, &lastInputState);
  if(inputDiff.bushbuttonChanged)
  {
    // rising Edge because of pull-up resistor, so LOW means button pressed
    if (currentInputState.bushbuttonValue == LOW)
    {
      static int resetState = HIGH; // initial state
      resetState  = (resetState  == HIGH) ? LOW : HIGH; // toggle state 
      Serial.println("Reset Program: ");
      derivedInitialized = false; // reset derived state initialization
      delay(100);
    }
  }

  // map poti value to db value
  double mappedDb = mapDouble(currentInputState.potiValue, POTI_MIN_VALUE, POTI_MAX_VALUE, VOLUME_MIN_DB, VOLUME_MAX_DB);
  currentDerivedState.db = VOLUME_MIN_DB; // default value in case of mapping error
  if (isnan(mappedDb)) {
    Serial.println("Mapping error!");
  }
  else {
    currentDerivedState.db = (int)(mappedDb + 0.5); // korrektes Runden
    //Serial.println(currentDerivedState.db);
  }

  if (!derivedInitialized) 
  {
    lastDerivedState.volumeLevel = VOLUME_UNKNOWN; // initial state
    derivedInitialized = true;
  }
  currentDerivedState.volumeLevel =  getVolumeLevelHysteresis(currentDerivedState.db, lastDerivedState.volumeLevel, VOLUME_MIN_DB, VOLUME_ACCEPTABLE_THRESHOLD_DB, VOLUME_ELEVATED_THRESHOLD_DB, VOLUME_MAX_DB, HYSTERESIS_DB);

  // recognize derived changes
  DerivedDiff derivedDiff = diffDerived(&currentDerivedState, &lastDerivedState);

  if(derivedDiff.volumeLevelChanged)  // recognize changes
  {
    // set outputs LEDs
    handleVolumeTransition(lastDerivedState.volumeLevel, currentDerivedState.volumeLevel);
      
    Serial.print("Transition: ");
    Serial.print(lastDerivedState.volumeLevel);
    Serial.print(" -> ");
    Serial.println(currentDerivedState.volumeLevel);
  
  }
  lastDerivedState = currentDerivedState; // update last derived state
  lastInputState = currentInputState; // update last input state
}

// functions definitions

/*************************************************************************************************************
 * Handles the transition between different volume levels.
 * This function updates the LED actions based on the transition from the last volume level to the current volume level.
 * 
 * @param lastVolumeLevelState   The previous volume level.
 * @param currentVolumeLevelState The current volume level.
 *  
 *************************************************************************************************************/
void handleVolumeTransition(VolumeLevel lastVolumeLevelState, VolumeLevel currentVolumeLevelState)
{
  if (lastVolumeLevelState == currentVolumeLevelState) return; // nichts zu tun

  // all possible transitions are handled
  switch (lastVolumeLevelState) {
    
    case VOLUME_ACCEPTABLE:
      if (currentVolumeLevelState == VOLUME_ELEVATED) {
        digitalWrite(GREEN_LED_PIN, LOW);
        digitalWrite(YELLOW_LED_PIN, HIGH);
      }
      break;
    case VOLUME_ELEVATED:
      if (currentVolumeLevelState == VOLUME_ACCEPTABLE) {
        digitalWrite(GREEN_LED_PIN, HIGH);
        digitalWrite(YELLOW_LED_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW); //XX
      }
      else if (currentVolumeLevelState == VOLUME_DANGEROUS) {
        digitalWrite(YELLOW_LED_PIN, LOW);
        digitalWrite(RED_LED_PIN, HIGH);
      }
      break;
    case VOLUME_DANGEROUS:
      if (currentVolumeLevelState == VOLUME_ELEVATED) {
        digitalWrite(YELLOW_LED_PIN, HIGH);
        //digitalWrite(RED_LED_PIN, LOW); XX
      }
      break;
    case VOLUME_UNKNOWN:  //initial state, set LEDs according to current state
      if (currentVolumeLevelState == VOLUME_ACCEPTABLE) {
        digitalWrite(GREEN_LED_PIN, HIGH);
      }
      else if (currentVolumeLevelState == VOLUME_ELEVATED) {
        digitalWrite(YELLOW_LED_PIN, HIGH);
      }
      else if (currentVolumeLevelState == VOLUME_DANGEROUS) {
        digitalWrite(RED_LED_PIN, HIGH);
      }
      break;
    default:
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
    break;
  }
}
