#define LED_PIN LED_BUILTIN
#define DEBUG
#include <stdint.h> /* uint*_t types */
#include <avr/pgmspace.h> /* pgm_read_dword */
#include "DebugUtils.h"

#include "IrCodes.h"
#define num_brightness_steps sizeof(brightness_steps)/sizeof(brightness_steps[0])
#define num_keys_up sizeof(keys_up)/sizeof(keys_up[0])
#define num_keys_down sizeof(keys_down)/sizeof(keys_down[0])
#define num_keys_toggle sizeof(keys_toggle)/sizeof(keys_toggle[0])

#include <IRremote.h>

#define IS_ON HIGH
#define IS_OFF LOW
#define DIR_UP 1
#define DIR_DOWN -1
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_TOGGLE 3
#define KEY_REPEAT 4
#define KEY_NOOP 5
#define DEBOUNCE_TO 200
uint32_t _debounce[KEY_NOOP]; // KEY_NOOP should have max id


int LEFT_PIN = 9;
int RIGHT_PIN = 10;
int TOGGLE_PIN = 6;
int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);
decode_results results;

Brightness br = {0,IS_ON,num_brightness_steps - 1,num_brightness_steps - 1};

// https://playground.arduino.cc/Code/PwmFrequency
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  irrecv.enableIRIn(); // Start the receiver
  pinMode(LEFT_PIN, OUTPUT);
  pinMode(RIGHT_PIN, OUTPUT);
  pinMode(TOGGLE_PIN, OUTPUT);
  setPwmFrequency(LEFT_PIN, 4);
  setPwmFrequency(RIGHT_PIN, 4);  
  Serial.begin(115200);
  for (uint8_t i = 0; i < KEY_NOOP; i++) {
    _debounce[i] = millis();
  }
  SetLampState(br);
  DEBUG_PRINT("Setup finished. Key arrays lengths:");
  DEBUG_PRINT(num_keys_up);
  DEBUG_PRINT(num_keys_down);
  DEBUG_PRINT(num_keys_toggle);
}

// Binary search along the lines of 
// http://forum.arduino.cc/index.php?topic=205281.msg1538385#msg1538385
int16_t findElement(const uint32_t keys[], uint16_t ar_size, uint32_t search) {
    uint16_t first = 0;
    uint16_t last = ar_size - 1;
    uint16_t middle;
    uint32_t curval;
    
    if ((search < pgm_read_dword(&keys[0])) || (search > pgm_read_dword(&keys[last]))) {
      return -1;
    }
    
    do {
      middle = (first + last)/2;
      curval = pgm_read_dword(&keys[middle]);
      if (curval == search) {
        return middle;
      }
      if (curval < search)
      {
        first = middle+1;    
        continue;
      }
      else {
        last = middle-1;
      }
    } 
    while(first <= last);
    return -1;
}


uint8_t debounce(uint8_t key) {
  uint32_t now = millis();
  uint8_t orig_key = key;
  if (now - _debounce[key] < DEBOUNCE_TO) {
    key = KEY_NOOP;
  }
  _debounce[orig_key] = now;
  return key;
}


uint8_t IrCode2Key(uint32_t r, uint32_t mask) {
  if (r == REPEAT) {
    return KEY_REPEAT;
  }

  if (findElement(keys_up, num_keys_up, r) >= 0) {
    return KEY_UP;
  }

  if (findElement(keys_down, num_keys_down, r) >= 0) {
    return KEY_DOWN;
  }

  if (findElement(keys_toggle, num_keys_toggle, r) >= 0) {
    return debounce(KEY_TOGGLE);
  }

  return KEY_NOOP;
}


Brightness GoDir(Brightness b) {
  uint8_t max_steps = num_brightness_steps - 1;
  
  if (b.dir > 0) {
    if (b.left < max_steps ) {
      b.left += 1;
    } else if (b.right < max_steps) {
      b.right += 1;
    }
  } else if (b.dir < 0) {
    if (b.left > 0) {
      b.left -= 1;
    } else if (b.right > 0) {
      b.right -= 1;
    }
  }

  return b;
}

Brightness UpdateBrightnessValues(Brightness b, uint8_t key) {
    if (key == KEY_REPEAT) {
      if (b.dir != 0) { // repeat only up and down, not toggle
        DEBUG_PRINT("REPEAT");
        return GoDir(b);
      }
    } else {
      switch (key) {
        case KEY_UP:
          b.dir = DIR_UP;
          DEBUG_PRINT("KEY_UP");
          break;
        
        case KEY_DOWN:
          b.dir = DIR_DOWN;
          DEBUG_PRINT("KEY_DOWN");
          break;
        
        case KEY_TOGGLE:
          b.dir = 0;
          b.is_on = (b.is_on == IS_ON)? IS_OFF : IS_ON;
          DEBUG_PRINT("KEY_TOGGLE");
          break;

         default:
          b.dir = 0;
          DEBUG_PRINT("KEY_NOOP");
          break;
      }
      return GoDir(b);
    }
}


Brightness SetLampState(Brightness b) {
  if (digitalRead(TOGGLE_PIN) != b.is_on) {
    digitalWrite(TOGGLE_PIN, b.is_on);
  }

  if (b.is_on == IS_ON) {
    analogWrite(LEFT_PIN, brightness_steps[b.left]);
    analogWrite(RIGHT_PIN, brightness_steps[b.right]);
  }
  
  return b;
}



void loop() {
  if (irrecv.decode(&results) && !results.overflow) {
    digitalWrite(LED_PIN, HIGH); 
 
    DEBUG_PRINTX(results.value);
    // DEBUG_PRINTX((uint32_t)((1 << results.bits) - 1));
    
    br = SetLampState(
      UpdateBrightnessValues(br, IrCode2Key(results.value, (1 << results.bits) - 1))
    );
    DEBUG_PRINTX((uint32_t) br.is_on << 24 | br.dir << 16 | br.left << 8 | br.right);
    // DEBUG_PRINTX((uint32_t) brightness_steps[br.left] << 8 | brightness_steps[br.right]);
    
    irrecv.resume(); // Receive the next value
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  delay(50);
}
