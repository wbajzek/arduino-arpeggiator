arduino-arpeggiator
===================

Arpeggiator based on Arduino and the SparkFun midi shield. It will run free
or sync to midi clock if it detects a start message. 

Button D2 = Toggle hold-mode on/off (I might replace hold-mode on/off
with an octave button, since I will probably always use it in hold mode.)

Button D3 = Cycle through arpeggio modes

Button D4 = Stop arpeggio


Pot A1 = Tempo (With midi sync, controls the beat divisions)

Pot A2 = Velocity


Arpeggio modes:
* Up
* Down
* Bounce (e.g., 1-2-3-2)
* UpDown (e.g., 1-2-3-3-2-1)
* OneThree (e.g., 1-3-2-4-3-5-4)
* OneThreeEven (e.g., 1-3-2-4-3-5)


Depends on http://sourceforge.net/projects/arduinomidilib/
