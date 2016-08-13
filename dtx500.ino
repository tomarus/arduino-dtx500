#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Message.h>
#include <midi_Namespace.h>
#include <midi_Settings.h>

// PIN Setup for 15 pin flatcable.
// Pin 0 is connected to GND for all 14 outputs
//
// The 595's are connected opposite of each other with the 15 pin flatcable in between.
// pin 8 of 595-1 is GND = pin 0 on flatcable.
// pin 8 of 595-2 is NC (hole) on flatcable.
//
// This should match more or less the DTX-500 cable layout.
// MIDI keys are notes as originally received from a TR-8.
//

// set to MIDI_CHANNEL_OMNI to listen on all channels
const int channel = 10;

// debug
const int switchPin = 3;
const int ledPin = 8;

// Number of notes
const int numpins = 14;

// Hihat pin/note
const int hihat = 4;
// Hihat Control pin/note
const int hihatControl = 1;

struct pinstruct {
  byte hc1;  // hc595-1
  byte hc2;  // hc595-2
  byte note; // midi note number (gm drum map)
  unsigned int off;   // timer to note-off
};

pinstruct pins[numpins] = {
  {0x00, 0x02, 0,  0},
  {0x80, 0x00, 0,  0}, // HH Control
  {0x00, 0x04, 36, 0}, // BD1       Bass Drum 1
  {0x40, 0x00, 57, 0}, // BD1-AUX   Crash Cymbal 2
  {0x00, 0x08, 0,  0}, // HHAT      Open HiHat  46!!
  {0x20, 0x00, 49, 0}, // CRASH     Crash Cymbal 1
  {0x00, 0x10, 51, 0}, // RIDE      Ride Cymbal
  {0x10, 0x00, 43, 0}, // TOM3      Low Tom   41??
  {0x00, 0x20, 37, 0}, //           Rim Shot / Cow Bell
  {0x08, 0x00, 47, 0}, // TOM2      Mid Tom   43??
  {0x00, 0x40, 39, 0}, //           HandClap
  {0x04, 0x00, 50, 0}, // TOM1      High Tom  45??
  {0x00, 0x80, 40, 0}, //           Not on tr8 (E-3)
  {0x02, 0x00, 38, 0}  // SNARE     Acoustic Snare
};

// Pins for 74HC595's
// Change to fit your setup!
const int data = 11;
const int clock = 10;
const int latch = 13;

//

// MIDI Initialization Settings
struct mySettings : public midi::DefaultSettings {
  static const bool UseRunningStatus = false;
  static const bool Use1ByteParsing = true;
  static const bool HandleNullVelocityNoteOnAsNoteOff = false;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, midiInput, mySettings);

// Handy stuff
const int ON = HIGH;
const int OFF = LOW;

// State of pins
byte state1 = 0x00;
byte state2 = 0x00;

//

void setup() {
  pinMode(data, OUTPUT);
  pinMode(clock, OUTPUT);
  pinMode(latch, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT);

  midiInput.begin(channel);
  midiInput.turnThruOff();
  midiInput.setHandleNoteOn(handleNoteOn);
  midiInput.setHandleNoteOff(handleNoteOff);

  for (int i = 0; i < 10; i++) {
    analogWrite(ledPin, 255);
    delay(100);
    analogWrite(ledPin, 0);
    delay(100);
  }
}

void loop() {
  midiInput.read();

  for (int i = 0; i < numpins; i++) {
    if (pins[i].off > 0) {
      pins[i].off--;
      if (pins[i].off == 0) {
        noteoff(i);
      }
    }
  }

  // Check if test knob is pressed. Loop through all notes once.
  while (digitalRead(switchPin) == LOW) {
    for (int i = 0; i < numpins; i++) {
      noteon(i, 100);
      delay(100);
      noteoff(i);
      delay(300);
    }
  }
}

//

static void handleNoteOn(byte channel, byte note, byte vel) {
  for (int i = 0; i < numpins; i++) {
    if (pins[i].note == note) {
      if (vel == 0) {
        noteoff(i);
      } else {
        noteon(i, vel);
      }
    }

    // Take care of hihats
    if (note == 0x2a && vel > 0) { //closed HH
      noteon(hihat, vel);
      noteoff(hihatControl);
    }
    if (note == 0x2e && vel > 0) { //open HH
      noteon(hihat, vel);
      noteon(hihatControl, 0xffff);
    }
  }
}

static void handleNoteOff(byte channel, byte note, byte vel) {
  for (int i = 0; i < numpins; i++) {
    if (pins[i].note == note) {
      noteoff(i);
    }
  }
}

void noteon(int n, int len) {
  state1 |= pins[n].hc1;
  state2 |= pins[n].hc2;

  pins[n].off = len;
  writeState();
  analogWrite(ledPin, 255);
}

void noteoff(int n) {
  state1 &= ~pins[n].hc1;
  state2 &= ~pins[n].hc2;
  pins[n].off = 0;
  writeState();

  if (state1 == 0 || state2 == 0) {
    analogWrite(ledPin, 0);
  }
}

//

void writeState() {
  digitalWrite(latch, 0);
  shiftOut(data, clock, state2);
  shiftOut(data, clock, state1);
  digitalWrite(latch, 1);
}

// 74hc595 shift
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  int i = 0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  for (i = 7; i >= 0; i--)  {
    digitalWrite(myClockPin, 0);

    if ( myDataOut & (1 << i) ) {
      pinState = 1;
    } else {
      pinState = 0;
    }

    digitalWrite(myDataPin, pinState);
    digitalWrite(myClockPin, 1);
    digitalWrite(myDataPin, 0);
  }

  digitalWrite(myClockPin, 0);
}

