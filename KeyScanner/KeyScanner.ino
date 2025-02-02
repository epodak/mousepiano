// Hardware: Mega 2560 R2 + Ethernet Shield

#define MOUSEDEBUG 0
#define LOGICDEBUG 0
#define VISUALIZER 0

#if defined(ARDUINO) && ARDUINO > 18
#include <SPI.h>a
#endif

#include <Ethernet.h>
#include "AppleMidi.h"

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0F, 0x4F, 0xF7
};

#define FASTADC 1

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

byte keyUp[88]; // 704 bits
byte keyDown[88]; // 704 bits

byte keyVelocity[88]; // 704 bits
byte keyStatus[22]; // 176 bits

// Total memory

// Each keyVelocity has these bits
// delayTime = 0-127 for velocity (7 bits)
// down = boolean if key is down

// Each keyStatus has info for 4 keys
// goingDown = boolean for downward motion
// sent = if midi message sent

byte s0 = 22;
byte s1 = 23;
byte s2 = 24;
byte s3 = 25;

byte controlPins[4] = {
  s0, s1, s2, s3
};

byte muxChannel[8] = {
  B00001000, // Channel 0 & 1
  B01001100, // Channel 2 & 3
  B00101010, // Channel 4 & 5
  B01101110, // Channel 6 & 7
  B00011001, // Channel 8 & 9
  B01011101, // Channel 10 & 11
  B00111011, // Channel 12 & 13
  B01111111  // Channel 14 & 15
};

APPLEMIDI_CREATE_DEFAULT_INSTANCE();

void setup() {

#if FASTADC
  // set prescale to 16
  sbi(ADCSRA, ADPS2) ;
  cbi(ADCSRA, ADPS1) ;
  cbi(ADCSRA, ADPS0) ;
#endif

  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);

  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);

  Serial.begin(115200);
  Serial.print("Getting IP...");

  if (Ethernet.begin(mac) == 0) {
    Serial.println();
    Serial.println( "Failed DHCP!" );
    for (;;)
      ;
  }

  // print your local IP address:
  Serial.println();
  Serial.print("IP: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  // Create a session and wait for a remote host to connect to us
  AppleMIDI.begin("test");

  // Actively connect to a remote host
  // IPAddress host(192, 168, 2, 1);
  // AppleMIDI.Invite(host, 5004);

  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  AppleMIDI.OnReceiveControlChange(OnAppleMidiControlChange);

  for (byte i = 0; i < 88; i++) {
    setDownStateForKey(false, i);
    setGoingDownStateForKey(false, i);
    setSentStateForKey(true, i);
  }
}

byte keyHigh = 180;
byte keyMid = 100;
byte keyLow = 80;

int currentPosition = 0;
int numberOfBoards = 6;

boolean showMessages = false;

void loop() {
  // Listen to incoming notes
  AppleMIDI.run();

  if (Serial.available() > 0) {
    // read the incoming byte
    int inputByte = Serial.read();

    if (inputByte == 108) {
      showMessages = !showMessages;
      Serial.read();
    } else if (inputByte == 10) {
      if (currentPosition > 87) {
        currentPosition = 0;
      }
      else {
        currentPosition++;
      }
    } else {
      Serial.println("Rec: " + String(inputByte));
    }
  }

  for (byte m = 0; m < 16; m++) {
    switchToChannel(m);
    delay(2);

    for (byte i = 0; i < numberOfBoards; i++) {
      byte key = (i * 16) + m;

      // Don't read the end of the last board
      if (key < 88) {
        byte keyPosition = map(analogRead(i), 0, 1023, 0, 255);

        if (keyPosition > keyUp[key]) {
          keyUp[key] = keyPosition;
        }
        if (keyDown[key] == 0) {
          keyDown[key] = abs(keyUp[key] * 0.8);
        }
        if (keyPosition < keyDown[key]) {
          keyDown[key] = keyPosition;
        }

        keyPosition = map(keyPosition, keyDown[key], keyUp[key], 0, 255);

        if (key == currentPosition && showMessages) {
          Serial.println("Channe: " + String(i + 1) + " - Key: " + String(key + 1) + " = " + String(keyPosition + 1) + " - " + String(analogRead(i)) + " / " + String(keyUp[key]) + " - " + String(keyDown[key]));
        }

        if (VISUALIZER) {
          Serial.print(String(key) + ":" + String(keyPosition));

          if (key != 87) {
            Serial.print(",");
          } else {
            Serial.println();
          }
        }

        // If below the height threshold
        if (keyPosition < keyHigh) {
          // If not down
          if (!downStateForKey(key)) {
            // We are going down
            setGoingDownStateForKey(true, key);

            // If we're below the key low
            if (keyPosition < keyLow) {
              setDownStateForKey(true, key);

              // We need to announace that we're down!
              setSentStateForKey(false, key);
            }
            else {
              // Reduce the velocity over time
              byte currentPosition = getKeyPosition(key);
              
              setKeyPosition(key, (currentPosition - 2));
            }
          }
          // We are already down
          else {
            // Above mid point and had been going down
            if (keyPosition > keyMid && goingDownStateForKey(key)) {
              // We're heading back up
              setGoingDownStateForKey(false, key);
              setDownStateForKey(false, key);

              // We need to announce that we're up!
              setSentStateForKey(true, key);
            }
          }
        }
        else {
          // We're above the threshold
          // If we were down or were going down
          if (downStateForKey(key) == 1) {

            setGoingDownStateForKey(false, key);
            setDownStateForKey(false, key);

            // We need to announce that we're up!
            setSentStateForKey(false, key);
          } else {
            setKeyPosition(key, 127);
          }
        }

        if (downStateForKey(key)) {
          if (!sentStateForKey(key)) {
            // Send MIDI Note ON with velocity
            if (MOUSEDEBUG) {
              Serial.println("Midi ON: " + String(key) + " Velocity:" + getKeyPosition(key));
            }
            AppleMIDI.noteOn(key + 24, getKeyPosition(key), 1);
            setSentStateForKey(true, key);
            setDownStateForKey(true, key);
          }
        }
        else {
          if (!sentStateForKey(key)) {
            // Send MIDI Note Off
            if (MOUSEDEBUG) {
              Serial.println("Midi OFF: " + String(key) + " Velocity:" + getKeyPosition(key));
            }
            AppleMIDI.noteOff(key + 24, 0, 1);
            setKeyPosition(key, getKeyPosition(key));
            setSentStateForKey(true, key);
          }
        }
      }
    }
  }
}

// Zero based channel selector (0-15)
void switchToChannel(byte channel) {
  byte block = muxChannel[(byte)floor(channel / 2)];

  if (LOGICDEBUG) {
    Serial.println("Channel: " + String(channel));
    Serial.println("Block: " + String((byte)floor(channel / 2)));
  }

  if (channel % 2 == 0) {
    for (byte i = 0; i < 4; i++) {
      digitalWrite(controlPins[i], bitRead(block, 7 - i));
      if (LOGICDEBUG) {
        Serial.print(bitRead(block, 7 - i));
      }
    }
  }
  else {
    for (byte i = 0; i < 4; i++) {
      digitalWrite(controlPins[i], bitRead(block, 3 - i));

      if (LOGICDEBUG) {
        Serial.print(bitRead(block, 3 - i));
      }
    }
  }

  if (LOGICDEBUG) {
    Serial.println("");
  }
}

boolean goingDownStateForKey(byte key) {
  byte section = 0;
  byte block = byte(abs(key / 4)); // key=87, block = 21

  if (block > 0) {
    section = byte(key - (block * 4)); // key = 87, section = 2
  } else {
    section = key;
  }

  return bitRead(keyStatus[block], (section * 2));
}

void setGoingDownStateForKey(boolean state, byte key) {
  byte block = 0;
  byte section = 0;

  block = byte(abs(key / 4)); // key=87, block = 21

  if (block > 0) {
    section = byte(key - (block * 4)); // key = 87, section = 2
  } else {
    section = key;
  }

  if (LOGICDEBUG) {
    Serial.println("Section: " + String(section));
  }

  if (state) {
    bitSet(keyStatus[block], (section * 2));
  }
  else {
    bitClear(keyStatus[block], (section * 2));
  }
}

boolean sentStateForKey(byte key) {
  byte block = byte(abs(key / 4)); // key=87, block = 21
  byte section;

  if (key > 0) {
    section = byte(key - (block * 4)); // key = 87, section = 2
  } else {
    section = key;
  }

  if (LOGICDEBUG) {
    Serial.println("Section: " + String(section));
  }

  return bitRead(keyStatus[block], (section * 2) + 1);
}

void setSentStateForKey(boolean state, byte key) {
  byte block = byte(abs(key / 4)); // key=87, block = 21
  byte section;

  if (key > 0) {
    section = byte(key - (block * 4)); // key = 87, section = 2
  } else {
    section = key;
  }

  if (LOGICDEBUG) {
    Serial.println("Section: " + String(section));
  }

  if (state) {
    bitSet(keyStatus[block], (section * 2) + 1);
  }
  else {
    bitClear(keyStatus[block], (section * 2) + 1);
  }
}

boolean downStateForKey(byte key) {
  // Return whether down state bit is true
  return ((keyVelocity[key] & 1) == 1);
}

void setDownStateForKey(boolean state, byte key) {
  if (state) {
    bitSet(keyVelocity[key], 0);
  }
  else {
    bitClear(keyVelocity[key], 0);
  }
}

byte getKeyPosition(byte key) {
  // Shift off the down state and return number
  return byte(keyVelocity[key] >> 1);
}

void setKeyPosition(byte key, byte keyPosition) {
  byte down = byte(keyVelocity[key] & 1); // Get down state from byte
  keyPosition = byte(keyPosition << 1); // Shift keyPosition to make room for down state
  keyVelocity[key] = byte(keyPosition | down); // Merge keyPosition and down state

  if (LOGICDEBUG) {
    Serial.println(String(keyVelocity[key]));
  }
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

void OnAppleMidiConnected(char* name) {
  Serial.print("C: ");
  Serial.println(name);
}

void OnAppleMidiDisconnected() {
  Serial.println("D");
}

void OnAppleMidiControlChange(byte channel, byte number, byte value) {
  if (number == 123) {
    //clearRegisters();
    delay(20);
  }
}
