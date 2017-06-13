/**
JC Andrioli - 13/may/2015
Dual player controller board for arduino leonardo, being used here to simulate a native USB keyboard.
This code maps pins to keystrokes on a single keyboard. This pretending game is useful to make cheap
arcade machines that don't need an analog joystick. Even useful if you DO have a joystick but it's
not analog - like those crappy chinese "red ball joysticks". 
If you want to reuse this for a system WITH analog joysticks, you can do that too, but only one
joystick works reliably (although on Windows it seems to be possible with 2 joysticks too). But who
would build an arcade machine on Windows...

In order to use this with your emulators, all you need to do is configure them with your keyboard!
Remember, there will be no real joystick listed in your computers device list, drivers, etc. Forget
all that complexity. For the software running on the machine, this all really looks just like a kbd.

-- Key maps --
Each player needs at least 12 keys: start,select,up,down,left,right,a,b,x,y,l,r.
Check the 'keys' array variable below, you can see which keys I'm using.
And in some cases you might want those 2 extra keys U and J, for example NeoGeo or "strange"
arcade (MAME) games use them.
You also may want a switch to dis/enable the trigger function (turbo mode).
If you're running a linux-based arcade machine you probably want some other keys too, like
reset, ESC, TAB, SPACE, etc.

Physical connections:
1 - the leo has at most 24 usable pins, and each player uses at least 12 keys, plus the "system"
    keys... we need more pins. A clever way to achieve that is multiplexing, i.e. a matrix of 6:
    pin 0_____|_____|_____|_____|______|_____|
    pin 1_____|_____|_____|_____|______|_____|
    pin 2_____|_____|_____|_____|______|_____|
    pin 3_____|_____|_____|_____|______|_____|
    pin 4_____|_____|_____|_____|______|_____|
    pin 5_____|_____|_____|_____|______|_____|
         pin 6 pin 7 pin 8 pin 9 pin 10 pin 11
    And that's how we get 36 inputs with 12 digital pins. And this is already coded by the
    keypad library!
**/
#include "Keypad.h"
#include "Keyboard.h" //this is solved by choosing Leonardo as your board

char keys[6][6] = {                           /*check arduinos HID.cpp for more keys*/
  { 'q', 'w', 'e', KEY_DOWN_ARROW, 't', 'y' }, //down
  { 'a', 's', 'd', KEY_RIGHT_ARROW, 'g', 'h' }, //right
  { 'z', 'x', 'c', 'v', 'b', 'n' }, //98 is keypad keycode for DOWN
  { 'u', 'i', 'o', 'p', 'j', 'k' },
  { 'l', 'm', KEY_LEFT_ARROW, '1', '2', '3' }, //left
  { '4', '5', KEY_UP_ARROW, '7', '8', '9' } // up
  /*{ 'q', 'w', 'e', 'r', 't', 'y'},
  { 'u', 'a', 's', 'd', 'f', 'g'},
  { 'h', 'j', KEY_INSERT, KEY_DELETE, KEY_UP_ARROW, KEY_DOWN_ARROW },
  { KEY_LEFT_ARROW, KEY_RIGHT_ARROW, 'o', 'p', '[', ']' },
  { 'k', 'l', ';', '\'', KEY_ESC, KEY_TAB },
  { KEY_RETURN, KEY_BACKSPACE, KEY_HOME, KEY_END, NO_KEY, NO_KEY }*/
};

byte rowPins[6] = {0, 1, 2, 3, 4, 5};
byte colPins[6] = {8, 9,10,11,12, 7}; //dont ude pin 13, is my led
Keypad Kpad( makeKeymap(keys), rowPins, colPins, 6, 6 );

static byte kpadState;

unsigned long loopCount;
unsigned long startTime;
String msg;

// By default enable turbo (trigger / quick shot)
// Turbo mode is an option for some games where
// you need to keep pressing buttons quickly, like
// some old shooting games. This should be off 
// for games that require drag-n-drop. Currently
// this feature is mapped to a keypad key that is
// in reality a phisical switch and not a button. 
boolean blnTurbo = true;            
// EntertainmentMode is where I make LEDs blink
// and dance. Default is on because when we power
// up the cabinet, the PC isn't ready yet so the
// arduino has a lil bitta time to goof around. 
boolean blnEntertainmentMode = true;

void setup() {
  
  // make pin 6 an input and turn on the
  // pullup resistor so it goes high unless
  // connected to ground:
  pinMode(6, INPUT_PULLUP);
  
  // initialize control over the keyboard:
  Keyboard.begin();

  Serial.begin(115200);
  loopCount = 0;
  startTime = millis();
  msg = "";
  Kpad.begin( makeKeymap(keys) );
  Kpad.addEventListener(keypadEvent);  // Add an event listener.
  Kpad.setHoldTime(250);                   // Default is 1000mS
}

void loop() {
  loopCount++;
  // print stats every 5s
  if ( (millis() - startTime) > 5000 ) {
    Serial.print("Average loops per second = ");
    Serial.println(loopCount / 5);
    startTime = millis();
    loopCount = 0;
    digitalWrite(13,!digitalRead(13));
  }

  // Fills Kpad.key[ ] array with up-to 10 active keys.
  // Returns true if there are ANY active keys.
  if (Kpad.getKeys())
  {
    for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
    {
      if ( Kpad.key[i].stateChanged )   // Only find keys that have changed state.
      {
        switch (Kpad.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:
            msg = " PRESSED.";
            Keyboard.press(Kpad.key[i].kchar);
            break;
          case HOLD:
            msg = " HOLD.";
            // if turbo mode is active, we will repeatedly press and release as long HOLD is true
            if (blnTurbo) {
              Keyboard.release(Kpad.key[i].kchar);
              Keyboard.press(Kpad.key[i].kchar); 
            }           
            break;
          case RELEASED:
            msg = " RELEASED.";
            Keyboard.release(Kpad.key[i].kchar);
            break;
          case IDLE:
            msg = " IDLE.";
        }
        
        Serial.print("Key ");
        Serial.print(Kpad.key[i].kchar);
        Serial.println(msg);
      }
    }
  }
}

void keypadEvent(KeypadEvent key) {
  // in here when in alpha mode.
  kpadState = Kpad.getState( );
  //swOnState( key );
} // end ltrs keypad events

/**********************************************/
/*//Pin connected to ST_CP of 74HC595
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 11;

//holders for infromation you're going to pass to shifting function
byte data;
byte dataArray[10];

void setup() {
  //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  Serial.begin(9600);

  //Binary notation as comment
  dataArray[0] = 0xFF; //0b11111111
  dataArray[1] = 0xFE; //0b11111110
  dataArray[2] = 0xFC; //0b11111100
  dataArray[3] = 0xF8; //0b11111000
  dataArray[4] = 0xF0; //0b11110000
  dataArray[5] = 0xE0; //0b11100000
  dataArray[6] = 0xC0; //0b11000000
  dataArray[7] = 0x80; //0b10000000
  dataArray[8] = 0x00; //0b00000000
  dataArray[9] = 0xE0; //0b11100000

  //function that blinks all the LEDs
  //gets passed the number of blinks and the pause time
  blinkAll_2Bytes(2,500); 
}

void loop() {

  for (int j = 0; j < 10; j++) {
    //load the light sequence you want from array
    data = dataArray[j];
    //ground latchPin and hold low for as long as you are transmitting
    digitalWrite(latchPin, 0);
    //move 'em out
    shiftOut(dataPin, clockPin, data);
    //return the latch pin high to signal chip that it 
    //no longer needs to listen for information
    digitalWrite(latchPin, 1);
    delay(300);
  }
}



// the heart of the program
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOut�
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {  
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}


//blinks the whole register based on the number of times you want to 
//blink "n" and the pause between them "d"
//starts with a moment of darkness to make sure the first blink
//has its full visual effect.
void blinkAll_2Bytes(int n, int d) {
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, 0);
  shiftOut(dataPin, clockPin, 0);
  digitalWrite(latchPin, 1);
  delay(200);
  for (int x = 0; x < n; x++) {
    digitalWrite(latchPin, 0);
    shiftOut(dataPin, clockPin, 255);
    shiftOut(dataPin, clockPin, 255);
    digitalWrite(latchPin, 1);
    delay(d);
    digitalWrite(latchPin, 0);
    shiftOut(dataPin, clockPin, 0);
    shiftOut(dataPin, clockPin, 0);
    digitalWrite(latchPin, 1);
    delay(d);
  }
}*/

