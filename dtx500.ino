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
// MIDI keys are notes from the GM drum map.
//
//         595-1 595-2 Description (0 = not set, pin 2 (0x01) on the 595's is not used)
// pin1  = 0x00  0x02  HHC1
// pin2  = 0x80  0x00  HHC2
// pin3  = 0x00  0x04  BD1
// pin4  = 0x40  0x00  BD1-AUX (Crash)
// pin5  = 0x00  0x08  HHAT
// pin6  = 0x20  0x00  CRASH
// pin7  = 0x00  0x10  RIDE
// pin8  = 0x10  0x00  TOM3
// pin9  = 0x00  0x20  TOM3-AUX
// pin10 = 0x08  0x00  TOM2
// pin11 = 0x00  0x40  TOM2-AUX
// pin12 = 0x04  0x00  TOM1
// pin13 = 0x00  0x80  TOM1-AUX
// pin14 = 0x02  0x00  SNARE

struct pinstruct {
  byte hc1;  // hc595-1
  byte hc2;  // hc595-2
  byte note; // midi note number (gm drum map)
  int off;   // timer to note-off
};

const int numpins = 14;

pinstruct pins[numpins] = {
  {0x00, 0x02, 0,  0},
  {0x80, 0x00, 0,  0},
  {0x00, 0x04, 36, 0}, // BD1       Bass Drum 1
  {0x40, 0x00, 57, 0}, // BD1-AUX   Crash Cymbal 2
  {0x00, 0x08, 46, 0}, // HHAT      Open HiHat
  {0x20, 0x00, 49, 0}, // CRASH     Crash Cymbal 1
  {0x00, 0x10, 51, 0}, // RIDE      Ride Cymbal
  {0x10, 0x00, 41, 0}, // TOM3      Low Tom
  {0x00, 0x20, 0,  0},
  {0x08, 0x00, 43, 0}, // TOM2      Mid Tom
  {0x00, 0x40, 0,  0},
  {0x04, 0x00, 45, 0}, // TOM1      High Tom
  {0x00, 0x80, 0,  0},
  {0x02, 0x00, 38, 0}  // SNARE     Acoustic Snare
};

// Pins for 74HC595's (Breadboard version)
// Change to fit your setup!
const int data = 2;
const int clock = 3;
const int latch = 4;

// Pins for 74HC595's (Stripboard version)
// I'm not a smart man, these pins could have been used for programming the atmega.
//const int data = 11;
//const int clock = 10;
//const int latch = 13;

const int channel = 10; // set to MIDI_CHANNEL_OMNI to listen on all channels

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

  midiInput.begin(channel);
  midiInput.turnThruOff();
  midiInput.setHandleNoteOn(handleNoteOn);
  midiInput.setHandleNoteOff(handleNoteOff);
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
}

static void handleNoteOn(byte channel, byte note, byte vel) {
  for (int i = 0; i < numpins; i++) {
    if (pins[i].note == note) {
      noteon(i, vel);
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
  pins[n].off = len * 100; // magic number depending on total clock cycles.

  digitalWrite(latch, 0);
  shiftOut(data, clock, state1);
  shiftOut(data, clock, state2);
  digitalWrite(latch, 1);
}

void noteoff(int n) {
  state1 &= ~pins[n].hc1;
  state2 &= ~pins[n].hc2;
  pins[n].off = 0;

  digitalWrite(latch, 0);
  shiftOut(data, clock, state1);
  shiftOut(data, clock, state2);
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

