#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 8
#define strobePin 6
#define resetPin 5

Adafruit_NeoPixel pixels[7];
float values[8];
int first_set_count = 0;

int rgbValue = 0;
unsigned int SAMPLE = 2;
unsigned int brightness = 5;
int edgeLight = 1;
unsigned int sensitivity = 9;
unsigned int noise_level = 200;

void setup()
{
  // enable leds output
  DDRD |= 0b11100000;
  DDRB |= 0x00111111;
  for (int i = 7; i <= 13; ++i) {
    values[i - 7] = 0;
    pixels[i - 7] = Adafruit_NeoPixel(NUMPIXELS, 13 - (i - 7),
              NEO_GRB + NEO_KHZ800);
    pixels[i - 7].begin();
  }

  // enable input for master button
  PCICR |= 0b00000010;
  // A1 interrupt
  PCMSK1 |= 0b00000010;
  // enable input butttons
  DDRC = 0x00;
  // enable pull-up for A2, A3, A4, A5
  PORTC = 0x3E;
  Serial.begin(9600);
}

ISR(PCINT1_vect)
{
  //0b00111100
  int button = PINC;
  if (button == 0b00111100)
    return;
  // general colour
  if (button == 0b00111000) {
    rgbValue += 10;
    if (rgbValue > 767)
      rgbValue -= 767;
    return;
  }
  // sensitivity of signal ++
  if (button == 0b00101100) {
    sensitivity++;
    return;
  }
  // sensitivity of signal --
  if (button == 0b00110100) {
    sensitivity--;
    return;
  }
  // increase threshhold for noise
  if (button == 0b00001100) {
    noise_level += 50;
    if (noise_level > 1023)
      noise_level -= 1023;
    return;
  }
  // decrease threshhold for noise
  if (button == 0b00110000) {
    noise_level -= 50;
    if (noise_level < 0)
      noise_level += 1023;
    return;
  }
  // togle edge light
  if (button == 0b00011100) {
    edgeLight = !edgeLight;
    return;
  }
  // btightness down
  if (button == 0b00000100) {
    brightness++;
    if (brightness > 255)
      brightness = 255;
    return;
  }
  // btightness up
  if (button == 0b00100000) {
    brightness--;
    if (brightness <= 0)
      brightness = 1;
    return;
  }
  // sample down
  if (button == 0b00001000) {
    SAMPLE--;
    first_set_count = 0;
    for (int i = 0; i < 7; ++i) {
      values[i] = 0;
    }
    if (SAMPLE == 0)
      SAMPLE = 1;
    return;
  }
  // sample up
  if (button == 0b00010000) {
    SAMPLE++;
    if (SAMPLE > 20)
      SAMPLE = 20;
    return;
  }
  // full reset settings
  if (button == 0b00000000) {
    sensitivity = 9;
    rgbValue = 0;
    SAMPLE = 1;
    brightness = 1;
    edgeLight = 1;
    noise_level = 200;
    first_set_count = 0;
    for (int i = 0; i < 7; ++i) {
      values[i] = 0;
    }
    return;
  }
}

void showRGB(int stip, int nr_led)
{
  float red;
  float green;
  float blue;
  int saveValue = rgbValue;

  for (int i = 0; i < 8; i++) {
    if (i < nr_led) {
      rgbValue += 15;
      if (rgbValue > 767)
        rgbValue -= 767;
      if (rgbValue < 0)
        rgbValue += 767;
      if (rgbValue <= 255) {
        red = 255 - rgbValue;
        green = rgbValue;
        blue = 0;
      } else if (rgbValue <= 511) {
        red = 0;
        green = 511 - rgbValue;
        blue = rgbValue - 256;
      } else {
        red = rgbValue - 512;
        green = 0;
        blue = 767 - rgbValue;
      }
      pixels[stip].setPixelColor(
        i, pixels[stip].Color(red / brightness,
                  green / brightness,
                  blue / brightness));
    } else {
      if (nr_led == i && edgeLight) {
        pixels[stip].setPixelColor(
          i,
          pixels[stip].Color(255 / brightness,
                 255 / brightness,
                 255 / brightness));
      } else {
        pixels[stip].setPixelColor(
          i, pixels[stip].Color(0, 0, 0));
      }
    }
  }
  pixels[stip].show();
  rgbValue = saveValue;
}
void loop()
{
  if (first_set_count < SAMPLE) {
    first_set_count++;
  }
  Serial.print(" rgbValue = ");
  Serial.print(rgbValue);
  Serial.print(" sensitivity = ");
  Serial.print(sensitivity);
  Serial.print(" noise_level = ");
  Serial.print(noise_level);
  Serial.print(" brightness = ");
  Serial.print(brightness);
  Serial.print(" SAMPLE = ");
  Serial.print(SAMPLE);
  Serial.print("\n");

  digitalWrite(resetPin, HIGH);
  delay(5);
  digitalWrite(resetPin, LOW);
  for (int i = 0; i < 7; i++) {
    digitalWrite(strobePin, LOW);
    delayMicroseconds(35);
    float spectrumValue = analogRead(A0);
    if (spectrumValue <= noise_level) {
      spectrumValue = 0;
    }
    float PWMvalue = map(spectrumValue, 0, 1024, 0, sensitivity);
    if (first_set_count == SAMPLE) {
      values[i] = values[i] - values[i] / SAMPLE +
            PWMvalue / SAMPLE;
    } else {
      values[i] += PWMvalue / SAMPLE;
    }
    showRGB(i, values[i]);
    digitalWrite(strobePin, HIGH);
  }
}
