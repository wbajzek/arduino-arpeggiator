#include <MIDI.h>


#define STAT1  7
#define STAT2  6


byte notes[10];
int tempo;
unsigned long time;
unsigned long blinkTime;
int playBeat;
int notesHeld=0;
boolean blinkOn;

void setup() {
  blinkTime = time = millis();
  notesHeld = 0;
  playBeat=0;
  blinkOn = false;

  pinMode(STAT1,OUTPUT);   
  pinMode(STAT2,OUTPUT);

  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);    

  // Connect the HandleNoteOn function to the library, so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(HandleNoteOn);  // Put only the name of the function
  MIDI.setHandleControlChange (HandleControlChange);
  MIDI.turnThruOff();

  digitalWrite(STAT1,HIGH);
  digitalWrite(STAT2,HIGH);

}


// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here: 
// http://arduinomidilib.sourceforge.net/class_m_i_d_i___class.html

void HandleNoteOn(byte channel, byte pitch, byte velocity) { 

  // Do whatever you want when you receive a Note On.

  if (velocity == 0)
    notesHeld--;
  else
    notesHeld++;

  if (notesHeld > 0)
    digitalWrite(STAT2,LOW);
  else
    digitalWrite(STAT2,HIGH);

  for (int i = 0; i < sizeof(notes); i++) {

    if (velocity == 0) {
      if (notes[i] >= pitch) {
        // remove this note
        if (i < sizeof(notes))
          notes[i] = notes[i+1];
      }
    }
    else {
      if (notes[i] == '\0') {
        notes[i] = pitch;
        return;
      }

      if (notes[i] <= pitch)
        continue;

      for (int j = sizeof(notes); j > i; j--)
        notes[j] = notes[j-1];

      notes[i] = pitch;
      return;
    }

  }

  // Try to keep your callbacks short (no delays ect) as the contrary would slow down the loop()
  // and have a bad impact on real-time performance.
}


void HandleControlChange (byte channel, byte number, byte value) {
  MIDI.sendControlChange(number,value,channel); 
}

void loop() {
  // Call MIDI.read the fastest you can for real-time performance.
  MIDI.read();

  tempo = (analogRead(1)/8)*10 + 80;
  unsigned int tick = millis();

  if (tick - time > tempo) {
    
    // stop the previous note
    MIDI.sendNoteOff(notes[playBeat],0,1);
    
    if (notesHeld > 0) { 
      time = tick;
      playBeat++;
      if (notes[playBeat] == '\0')
        playBeat=0;

      // trigger the current note
      MIDI.sendNoteOn(notes[playBeat],127,1);
    }
  }
  
  // turn the LED on when playing a note
  if (tick - blinkTime > tempo) {
    if (!blinkOn) {
      digitalWrite(STAT1,LOW);
      blinkOn = true;
      blinkTime = millis();
    }

  }
  // turn the LED off at the end of a blink
  if (blinkOn && tick - blinkTime > 30) {
    digitalWrite(STAT1,HIGH);
    blinkOn = false;    
  }
}



