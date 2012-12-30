// todo: handle midi clock
//       http://little-scale.blogspot.com/2008/05/how-to-deal-with-midi-clock-signals-in.html
// todo: add bypass mode

#include <MIDI.h>


#define STAT1    7
#define STAT2    6
#define BUTTON1  2
#define BUTTON2  3
#define BUTTON3  4

const int CHANNEL      = 1;
const int UP           = 0;
const int DOWN         = 1;
const int BOUNCE       = 2;
const int UPDOWN       = 3;
const int ONETHREE     = 4;
const int ONETHREEEVEN = 5;
const int MODES        = 6;

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
int clockTick;
boolean blinkOn;
boolean hold;
boolean buttonOneDown;
boolean buttonTwoDown;
boolean buttonThreeDown;
boolean bypass;
boolean midiThruOn;
boolean arpUp;
boolean clockSync;

void setup() {
  blinkTime = lastTime = millis();
  buttonOneHeldTime = buttonTwoHeldTime = buttonThreeHeldTime = 0;
  notesHeld = 0;
  playBeat=0;
  blinkOn = false;
  hold=true;
  arpUp = true; // used to determine which way arp is going when in updown mode
  buttonOneDown = buttonTwoDown = buttonThreeDown = false;
  mode=0;
  bypass = midiThruOn = false;
  tempo = 400;
  debounceTime = 50;
  clockSync = false;
  clockTick = 1;

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

  MIDI.setHandleNoteOn(HandleNoteOn); 
  MIDI.setHandleControlChange (HandleControlChange);
  MIDI.setHandleClock (HandleClock);
  MIDI.setHandleStart (HandleStart);
  MIDI.setHandleStop (HandleStop);
  MIDI.turnThruOff();

  digitalWrite(STAT1,HIGH);
  digitalWrite(STAT2,HIGH);

}


// This function will be automatically called when a NoteOn is received.
// see documentation here: 
// http://arduinomidilib.sourceforge.net/

void HandleNoteOn(byte channel, byte pitch, byte velocity) { 


  if (velocity == 0) // note released
    notesHeld--;
  else {
    // If it's in hold mode and you are not holding any notes down,
    // it continues to play the previous arpeggio. Once you press
    // a new note, it resets the arpeggio and starts a new one.
    if (notesHeld==0 && hold) 
      resetNotes();

    notesHeld++;
  }


  // Turn on an LED when any notes are held and off when all are released.
  if (notesHeld > 0)
    digitalWrite(STAT2,LOW); // stupid midi shield has high/low backwards for the LEDs
  else
    digitalWrite(STAT2,HIGH); // stupid midi shield has high/low backwards for the LEDs



  // find the right place to insert the note in the notes array
  for (int i = 0; i < sizeof(notes); i++) {

    if (velocity == 0) { // note released
      if (!hold && notes[i] >= pitch) { 

        // shift all notes in the array beyond or equal to the
        // note in question, thereby removing it and keeping
        // the array compact.
        if (i < sizeof(notes))
          notes[i] = notes[i+1];
      }
    }
    else {

      if (notes[i] == pitch)
        return;   // already in arpeggio
      else if (notes[i] != '\0' && notes[i] < pitch)
        continue; // ignore the notes below it
      else {
        // once we reach the first note in the arpeggio that's higher
        // than the new one, scoot the rest of the arpeggio array over 
        // to the right
        for (int j = sizeof(notes); j > i; j--)
          notes[j] = notes[j-1];

        // and insert the note
        notes[i] = pitch;
        return;        
      }
    }
  }
}

// just pass midi CC through
void HandleControlChange (byte channel, byte number, byte value) {
  MIDI.sendControlChange(number,value,channel); 
}

// This is a midi clock sync "start" message. In this case, switch to clock
// sync mode and trigger the first note.
void HandleStart () {
  clockSync = true;  
  clockTick = 1;
  playBeat = 0;
  cli();
  tick = millis();
  sei();

  handleTick(tick);
}

// Turn clock sync off when "stop" is received.
void HandleStop () {
  clockSync = false;
}

// on each tick of the midi clock, determine whether or note to play
// a note
void HandleClock () {

  cli();
  tick = millis();
  sei();

  // keep a counter of every clock tick we receive. if the count
  // corresponds with the subdivision selected with the tempo knob,
  // play the note and reset the clock.
  //
  // clock is 1 based instead of 0 for ease of calculation 

  if (clockTick >= (int)map(analogRead(1),0,1023,1,4)*6 + 1) {
    handleTick(tick);
    clockTick = 1;
  }


  clockTick++;
}

void loop() {

  MIDI.read();


  cli();
  tick = millis();
  sei();

  handleButtonOne();
  handleButtonTwo();
  handleButtonThree();

  // if not in  clock synch mode, we just read the tempo from the
  // tempo knob. 
  if (!clockSync) {  
    // There's no need to be precise about it here. This simple 
    // calculation is done quickly and gives a very wide range.
    tempo = 6000 / ((127-analogRead(1)/8) + 2);
    handleTick(tick);
  }

}

void handleTick(unsigned long tick) {

  // leave the LED long enough to be brightish but not so long
  // that it ends up being solid instead of blinking
  if (blinkOn && tick - blinkTime > 10) {
    blinkOn=false;
    digitalWrite(STAT1,HIGH); // stupid midi shield has high/low backwards for the LEDs
  }
  if (clockSync || tick - lastTime > tempo) {
    blinkTime = lastTime = tick;
    digitalWrite(STAT1,LOW); // stupid midi shield has high/low backwards for the LEDs
    blinkOn = true;


    if ((hold || notesHeld > 0) && notes[0] != '\0') { 


      // stop the previous note
      // MIDI.sendNoteOff(notes[playBeat],0,CHANNEL);

      // fixes a bug where a random note would sometimes get played when switching chords
      if (notes[playBeat] == '\0')
        playBeat = 0;

      // play the current note
      MIDI.sendNoteOn(notes[playBeat],velocity(),CHANNEL);

      // decide what the next note is based on the mode.
      if (mode == UP) 
        up();
      else if (mode == DOWN) 
        down();
      else if (mode == BOUNCE) 
        bounce();
      else if (mode == UPDOWN) 
        upDown();
      else if (mode == ONETHREE)
        oneThree();
      else if (mode == ONETHREEEVEN)
        oneThreeEven();

    }
  }
}


int velocity() {
  int velocity = 127 - analogRead(0)/8;

  // don't let it totally zero out.
  if (velocity == 0)
    velocity++;

  return velocity;

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


void bounce() {
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


void upDown() {
  if (sizeof(notes) == 1)
    playBeat=0;
  else
    if (arpUp) {
      if (notes[playBeat+1] == '\0') {
        arpUp = false;           
      }    
      else
        playBeat++;   
    }
    else {
      if (playBeat == 0) {
        arpUp = true;
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

void oneThreeEven() {

  if (notes[playBeat+1] == '\0') {
    playBeat = 0;
    arpUp = true;
    return;
  }


  if (arpUp)
    playBeat += 2;
  else
    playBeat--;

  arpUp = !arpUp;


}

// empties out the arpeggio. used when switching modes, when in hold mode and
// a new arpeggio is started, or when the reset button is pushed.
void resetNotes() {
  for (int i = 0; i < sizeof(notes); i++)
    notes[i] = '\0';
}


// Basic code to read the buttons.
char button(char button_num)
{
  return (!(digitalRead(button_num)));
}


// Button one is the the Hold button.
void handleButtonOne() {
  boolean buttonOnePressed = button(BUTTON1);
  if (buttonOnePressed) {
    if (buttonOneHeldTime == 0)
      buttonOneHeldTime = tick;

    if (!buttonOneDown && (tick - buttonOneHeldTime > debounceTime)) {
      buttonOneDown = true;
      hold = !hold;
      resetNotes();
    }
  }
  else {
    buttonOneDown = false;
    buttonOneHeldTime = 0;
  }
}

// Button two is the mode button.
void handleButtonTwo() {
  boolean buttonTwoPressed = button(BUTTON2);
  if (buttonTwoPressed) {
    if (buttonTwoHeldTime == 0)
      buttonTwoHeldTime = tick;

    if (!buttonTwoDown && (tick - buttonTwoHeldTime > debounceTime)) {
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
}

// button three is the reset button
void handleButtonThree() {
  boolean buttonThreePressed = button(BUTTON3);
  if (buttonThreePressed) {
    if (buttonThreeHeldTime == 0)
      buttonThreeHeldTime = tick;

    if (!buttonThreeDown && (tick - buttonThreeHeldTime > debounceTime)) {

      buttonThreeDown = true;
      resetNotes();
    }
  }
  else {
    buttonThreeDown = false;
    buttonThreeHeldTime = 0;
  }
}















