// DALIAS program, IN1060, 2019
#include <Adafruit_NeoPixel.h>
#define PIN        7 // for ringen
#define NUMPIXELS 12 // hvor mange pixels ringen har
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define INITIAL_DELAYVAL 50 // Time (in milliseconds) to pause between pixels
#define DELAYVAL 800 // Time (in milliseconds) to pause between pixels

#include "HX711.h"
#define calibration_factor 107.0 //-7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  3 // for vektsensor
#define CLK  2 // for vektsensor

HX711 scale;


unsigned long forrigeTiden=0;
long intervall=12000;
float forrigeVekt;
int antallHentinger = 0;
int antallDrikkinger = 0;
int antallPikslerHenting = 0;
int antallPikslerDrikking = 0;

void setup() {

  pixels.begin(); // initialisere ringen
  pixels.clear(); // ingen pixel lyser
  pixels.setPixelColor(0,pixels.Color(250,128,114));
  pixels.show(); 
  delay(2000);
  pixels.setPixelColor(4,pixels.Color(250,128,114));
  pixels.show(); 
  delay(2000);
  pixels.clear(); // ingen pixel lyser

  scale.begin(DOUT, CLK); // inisialiserer vektmaaler
  scale.set_scale(calibration_factor); 
  forrigeVekt = scale.get_units();
}

void loop() {
  float vekt = scale.get_units();
  unsigned long naaTiden=millis();
  if (naaTiden - forrigeTiden > intervall) {
    forrigeTiden = naaTiden;
  //sender paaminnelse
    for(int i=0; i<NUMPIXELS; i++) { // For hver piksel
      pixels.setPixelColor(i, pixels.Color(220, 20, 60));
      pixels.show();   // viser fargene
      delay(INITIAL_DELAYVAL); // venter litt
      pixels.clear(); // ingen pixel lyser
    }
    //men den skal beholde de forrige piksler som ble fortjent
    for(int i=0; i<antallPikslerHenting; i++) {
      pixels.setPixelColor(i,pixels.Color(250,128,114));
    }
    for(int i=0; i<antallPikslerDrikking; i++) {
      pixels.setPixelColor(NUMPIXELS/2-i,pixels.Color(250,128,114));
    }
    pixels.show(); // ingen pixel lyser
  }
  if (vekt - forrigeVekt > 100) {
    // har hentet vann
    antallHentinger++;

   // man lyser en til lys
    pixels.setPixelColor(antallPikslerHenting,pixels.Color(250,128,114));
    pixels.show(); 
    antallPikslerHenting++;
    
   // man glemmer venting
    forrigeTiden = naaTiden;
  } else if (forrigeVekt - vekt > 200) { // 
    // har drukket vann
    antallDrikkinger++;
    forrigeTiden = naaTiden;

    pixels.setPixelColor(NUMPIXELS/2-antallPikslerDrikking,pixels.Color(250,128,114));
    pixels.show(); 
    antallPikslerDrikking++;
  }
  forrigeVekt=vekt;
}
