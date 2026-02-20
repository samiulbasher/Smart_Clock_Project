#include "Arduino.h"
#include "config.h"
#include "buzzer.h"


#define BEEP_FREQ_SHORT 2000   // Hz
#define BEEP_FREQ_LONG  1200   // Hz


/*
*Effect	           Change
*Softer            beep	lower freq (1000–1500 Hz)
*Sharper           beep	higher freq (2500 Hz)
*Faster click	     reduce onTime
*Bell-like	       increase offTime slightly
*/

struct BeepState {
  bool active = false;
  uint8_t steps = 0;
  uint16_t freq = 0;
  uint16_t onTime = 0;
  uint16_t offTime = 0;
  uint32_t nextChange = 0;
  bool toneOn = false;
};

static BeepState beep;



void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off
}

void manageBuzzer() {
  if (!beep.active) return;

  uint32_t now = millis();
  if (now < beep.nextChange) return;

  if (beep.toneOn) {
    noTone(BUZZER_PIN);
    beep.nextChange = now + beep.offTime;
  } else {
    tone(BUZZER_PIN, beep.freq);
    beep.nextChange = now + beep.onTime;
  }

  beep.toneOn = !beep.toneOn;
  beep.steps--;

  if (beep.steps == 0) {
    noTone(BUZZER_PIN);
    beep.active = false;
  }
}

/*
// beeper function without blocking (but some limitations! because of updateWeather() is blocking)
void beeper(uint8_t count, uint8_t pattern) {
  if (beep.active) return; // ignore if already beeping

  if (pattern == SHORT_BEEP) {
    beep.freq = BEEP_FREQ_SHORT;
    beep.onTime = 60;
    beep.offTime = 80;
  } else {
    beep.freq = BEEP_FREQ_LONG;
    beep.onTime = 150;
    beep.offTime = 120;
  }

  beep.steps = count * 2;   // ON + OFF per beep
  beep.toneOn = false;
  beep.active = true;
  beep.nextChange = millis();
}
*/


// Blocking beeper function with delay 
void beeper(uint8_t count, uint8_t pattern) {
  beep.active = false; // ensure not using non-blocking mode

  if (pattern == SHORT_BEEP) {
    beep.freq = BEEP_FREQ_SHORT;
    beep.onTime = 60;
    beep.offTime = 80;
  } else {
    beep.freq = BEEP_FREQ_LONG;
    beep.onTime = 150;
    beep.offTime = 120;
  }

  for (uint8_t i = 0; i < count; i++) {
    tone(BUZZER_PIN, beep.freq);
    delay(beep.onTime);
    noTone(BUZZER_PIN);
    if (i < count - 1) {
      delay(beep.offTime);
    }
  }
}