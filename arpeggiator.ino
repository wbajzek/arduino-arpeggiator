// todo: handle midi clock
//       http://little-scale.blogspot.com/2008/05/how-to-deal-with-midi-clock-signals-in.html
// todo: add bypass mode

#include <MIDI.h>


#define STAT1    7
#define STAT2    6
#define BUTTON1  2
#define BUTTON2  3
#define BUTTON3  4

const int CHANNEL  = 1;
const int UP       = 0;
const int DOWN     = 1;
const int UPDOWN   = 2;
const int ONETHREE = 3;
const int MODES    = 4;

byte notes[10];
unsigned long tempo;
unsigned long lastTime;
unsigned long blinkTime;
unsigned long tick;
unsigned long buttonOneHeldTime;
unsigned long buttonTwoHeldTime;
unsigned long buttonThreeHeldTime;
unsigned long debounceTime;
int playBeat;
int notesHeld;
int mode;
boolean blinkOn;
boolean hold;
boolean buttonOneDown;
boolean buttonTwoDown;
boolean buttonThreeDown;
boolean bypass;
boolean midiThruOn;
boolean arpUp;

void setup() {
  blinkTime = lastTime = millis();
  buttonOneHeldTime = buttonTwoHeldTime = buttonThreeHeldTime = 0;
  notesHeld = 0;
  playBeat=0;
  blinkOn = false;
  hold=true;
  arpUp = true; // used to determine which way arp is going when in updown mode
  buttonOneDown = buttonTwoDown = buttonThreeDown = false;
  mode=3;
  bypass = midiThruOn = false;
  tempo = 400;
  debounceTime = 10;

  pinMode(STAT1,OUTPUT);   
  pinMode(STAT2,OUTPUT);
  pinMode(BUTTON1,INPUT);
  pinMode(BUTTON2,INPUT);
  pinMode(BUTTON3,INPUT);

  digitalWrite(BUTTON1,HIGH);
  digitalWrite(BUTTON2,HIGH);
  digitalWrite(BUTTON3,HIGH);

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
// http://arduinomidilib.sourceforge.net/

void HandleNoteOn(byte channel, byte pitch, byte velocity) { 

  if (velocity == 0)
    notesHeld--;
  else {
    // If it's in hold mode and you are not holding any notes down,
    // it continues to play the previous arpeggio. Once you press
    // a new note, it resets the arpeggio and starts a new one.
    if (notesHeld==0 && hold) 
      resetNotes();      
    notesHeld++;
  }
  // Turn on an LED when notes are held.
  if (notesHeld > 0)
    digitalWrite(STAT2,LOW);
  else
    digitalWrite(STAT2,HIGH);


  for (int i = 0; i < sizeof(notes); i++) {

    if (velocity == 0) {
      if (!hold && notes[i] >= pitch) {
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
}


void HandleControlChange (byte channel, byte number, byte value) {
  MIDI.sendControlChange(number,value,channel); 
}

void loop() {

  // Call MIDI.read the fastest you can for real-time performance.
  MIDI.read();


  cli();
  tick = millis();
  sei();

  boolean buttonOnePressed = button(BUTTON1);
  boolean buttonTwoPressed = button(BUTTON2);
  boolean buttonThreePressed = button(BUTTON3);

  if (buttonOnePressed) {
    if (!buttonOneDown) {
      buttonOneDown = true;
      buttonOneHeldTime = tick;
      hold = !hold;
      resetNotes();
    }
  }
  else {
    buttonOneDown = false;
    buttonOneHeldTime = 0;
  }

  if (buttonTwoPressed) {
    if (!buttonTwoDown) {
      buttonTwoHeldTime = tick;
      buttonTwoDown = true;
      playBeat=0;
      mode++;
      if (mode == MODES) {
        mode=0;
      }
      arpUp = true;
    }
  }
  else {
    buttonTwoDown = false;
    buttonTwoHeldTime = 0;
  }

  if (buttonThreePressed) {
    resetNotes();
    if (!buttonThreeDown) {
      buttonThreeDown = true;
      buttonThreeHeldTime = tick;
    }
  }
  else {
    buttonThreeDown = false;
    buttonThreeHeldTime = 0;
  }



  tempo = 6000 / ((127-analogRead(1)/8) + 10);

  if (blinkOn && tick - blinkTime > 50) {
    blinkOn=false;
    digitalWrite(STAT1,HIGH);
  }
  if (tick - lastTime > tempo) {
    blinkTime = lastTime = tick;
    digitalWrite(STAT1,LOW);
    blinkOn = true;


    if ((hold || notesHeld > 0) && notes[0] != '\0') { 

      if (mode == UP) 
        up();
      else if (mode == DOWN) 
        down();
      else if (mode == UPDOWN) 
        upDown();
      else if (mode == ONETHREE)
        oneThree();


      // stop the previous note
      MIDI.sendNoteOff(notes[playBeat],0,CHANNEL);
      byte velocity = 127 - analogRead(0)/8;

      // don't let it totally zero out.
      if (velocity == 0)
        velocity++;
      // play the current note
      MIDI.sendNoteOn(notes[playBeat],velocity,CHANNEL);
    }
  }
}

void up() {
  playBeat++;
  if (notes[playBeat] == '\0')
    playBeat=0;     
}

void down() {
  if (playBeat == 0) {
    playBeat = sizeof(notes)-1;
    while (notes[playBeat] == '\0') {
      playBeat--;
    }
  }        
  else       
    playBeat--;
}

void upDown() {
  if (sizeof(notes) == 1)
    playBeat=0;
  else
    if (arpUp) {
      if (notes[playBeat+1] == '\0') {
        arpUp = false;           
        playBeat--;
      }    
      else
        playBeat++;   
    }
    else {
      if (playBeat == 0) {
        arpUp = true;
        playBeat++;
      }
      else
        playBeat--;
    }
}

void oneThree() {
  if (arpUp)
    playBeat += 2;
  else
    playBeat--;
  
  arpUp = !arpUp;
  
  if (notes[playBeat] == '\0') {
    playBeat = 0;
    arpUp = true;
  }
}

void resetNotes() {
  for (int i = 0; i < sizeof(notes); i++)
    notes[i] = '\0';
}

char button(char button_num)
{
  return (!(digitalRead(button_num)));
}







