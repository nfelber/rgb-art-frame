#include <Adafruit_NeoPixel.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>

#define LED_PIN 0
#define LED_COUNT 5

#define SW 2
#define DT 3
#define CLK 4

struct hsv_t {
  float h;
  float s;
  float v;
};

enum Mode {
  FX,
  Param,
  Brightness,
  MODE_COUNT
} current_mode;

enum FX {
  Spectrum,
  Rainbow,
  Static,
  Fire,
  Flash,
  Test,
  FX_COUNT
} current_fx = EEPROM.read(FX);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int lastStateCLK = 0;
int lastStateSW = 0;

const int MAX_PARAM_VAL = 31;
int current_param_val = EEPROM.read(Param);
int current_brightness = EEPROM.read(Brightness);

ISR(PCINT0_vect)
{
  if( digitalRead(SW) == LOW ) {
    button_event(true);
  }else{
    button_event(false);
  }
  
  int currentStateCLK = digitalRead(CLK);

  if (currentStateCLK != lastStateCLK) {
    if (digitalRead(DT) != currentStateCLK) {
      encoder_event(true);
    } else {
      encoder_event(false);
    }
  }

  lastStateCLK = currentStateCLK;
}

void setup() {
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT);

  PCMSK  |= bit (SW);
  PCMSK  |= bit (DT);
  PCMSK  |= bit (CLK);
  GIMSK  |= bit (PCIE);    // enable pin change interrupts 3
  
  strip.begin();
  strip.show(); 
  strip.setBrightness(current_brightness);
}

void loop() {
  unsigned long t = millis();
  uint32_t colors[5] = {0};

  float param = (float) current_param_val / (float) MAX_PARAM_VAL;

  switch (current_fx) {
    case Spectrum: spectrumFX(t, param, colors); break;
    case Rainbow: rainbowFX(t, param, colors); break;
    case Static: staticFX(param, colors); break;
    case Fire: fireFX(t, param, colors); break;
    case Flash: flashFX(t, param, colors); break;
    case Test: testFX(param, colors); break;
  }

  for(int i=0; i<strip.numPixels(); ++i) {
    strip.setPixelColor(i, colors[i]);
  }
  strip.show();
  delay(50);
}

void encoder_event(bool down) {
  if (!down) {
    switch (current_mode) {
      case FX: current_fx = (current_fx + 1) % FX_COUNT;
               EEPROM.write(FX, current_fx);
               break;
      case Param: current_param_val = (current_param_val + 1) % (MAX_PARAM_VAL + 1);
                  EEPROM.write(Param, current_param_val);
                  break;
      case Brightness: current_brightness = (current_brightness + 8) % 256;
                       EEPROM.write(Brightness, current_brightness);
                       strip.setBrightness(current_brightness);
                       break;
    };
  } else {
    switch (current_mode) {
      case FX: current_fx = (current_fx + FX_COUNT - 1) % FX_COUNT;
               EEPROM.write(FX, current_fx);
               break;
      case Param: current_param_val = (current_param_val + MAX_PARAM_VAL) % (MAX_PARAM_VAL + 1);
                  EEPROM.write(Param, current_param_val);
                  break;
      case Brightness: current_brightness = (current_brightness + 248) % 256;
                       EEPROM.write(Brightness, current_brightness);
                       strip.setBrightness(current_brightness);
                       break;
    };
  }
}

void button_event(bool down) {
  if (down) {
    current_mode = (current_mode + 1) % MODE_COUNT;
  } else {
  }
}

uint32_t hsv_to_rgb(hsv_t hsv) {
  float h = hsv.h - ((int) (hsv.h/360) - (hsv.h < 0?1:0)) * 360.0;
  
  float s = min(1.0, max(0.0, hsv.s));
  float v = min(1.0, max(0.0, hsv.v));
  
  float C = s * v;
  float H = h / 60.0;
  float m = v - C;
  
  float r = 0;
  float g = 0;
  float b = 0;

  if (H < 2.0) {
    float X = C * (1.0 - abs(H - 1.0));
    if (H < 1.0) {  // 0 <= H < 1
      r = C;
      g = X;
    } else {  // 1 <= H < 2
      r = X;
      g = C;
    }
  } else if (H < 4.0) {
    float X = C * (1.0 - abs(H - 3.0));
    if (H < 3.0) {  // 2 <= H < 3
      g = C;
      b = X;
    } else {  // 3 <= H < 4
      g = X;
      b = C;
    }
  } else {
    float X = C * (1.0 - abs(H - 5.0));
    if (H < 5.0) {  // 4 <= H < 5
      r = X;
      b = C;
    } else {  // 5 <= H < 6
      r = C;
      b = X;
    }
  }

  return strip.Color((r + m) * 255, (g + m) * 255, (b + m) * 255);
}

void spectrumFX(unsigned long t, float param, uint32_t *colors) {
  unsigned long m = (500 + 9500 * (1-param));
  double x = (t % m) / (double) m;
  for (int i=0; i<strip.numPixels(); ++i) {
    hsv_t c = { x * 360.0, 1.0, 1.0 };
    colors[i] = hsv_to_rgb(c);
  }
}

void rainbowFX(unsigned long t, float param, uint32_t *colors) {
  unsigned long m = 5000;
  double x = (t % m) / (double) m;
  for (int i=0; i<strip.numPixels(); ++i) {
    hsv_t c = { (x + i / (double) strip.numPixels() * (1-param)) * 360.0, 1.0, 1.0 };
    colors[i] = hsv_to_rgb(c);
  }
}

void staticFX(float param, uint32_t *colors) {
  for (int i=0; i<strip.numPixels(); ++i) {
    hsv_t c = { param * 360.0, 1.0, 1.0 };
    colors[i] = hsv_to_rgb(c);
  }
}

void fireFX(unsigned long t, float param, uint32_t *colors) {
  double x = (t % 10000) / 10000.0 * 8.0 * PI;
  for (int i=0; i<strip.numPixels(); ++i) {
    float xi = x + i*3;
    float noise = sin(xi) + sin(1.25 * xi) + sin(3.25 * xi) + cos(5 * xi);
    hsv_t c = { param * 360.0 + noise * 2.0, 1.0, 1.0 - (noise + 4) / 7.0 };
    colors[i] = hsv_to_rgb(c);
  }
}

void flashFX(unsigned long t, float param, uint32_t *colors) {
  for (int i=0; i<strip.numPixels(); ++i) {
    hsv_t c = { 0, 0, t % (2 + (int)(param*20) + i) == 0?1:0 };
    colors[i] = hsv_to_rgb(c);
  }
}

void testFX(float param, uint32_t *colors) {
  for (int i=0; i<strip.numPixels(); ++i) {
    bool on = ((1 << i) & current_param_val) != 0;
    colors[i] = strip.Color(on * 255, on * 255, on * 255);
  }
}
