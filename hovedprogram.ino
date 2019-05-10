// DALIAS program, IN1060, 2019
#include <Adafruit_NeoPixel.h>
#define PIN        7 // for ringen
#define NUMPIXELS 12 // hvor mange pixels ringen har
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define INITIAL_DELAYVAL 50 // Tid (i millisekunder) for å vente mellom pikseler, i inisialisering
#define DELAYVAL 800 // Tid (i millisekunder) for å vente mellom pikseler, i notifiseringsdans
#define INTERVALL 12000 // Tiden (i milisekunder) etter det systemet notifiserer brukeren at hun ikkehar beveget seg

#include "HX711.h"
#define calibration_factor 781.86 //-7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  3 // for vektsensor
#define CLK  2 // for vektsensor

HX711 scale;


unsigned long forrigeTiden = 0;
float vekt = 0;
float forrigeVekt = 0; // forrige målt vekten
float endaForrigeVekt = 0; // vekten før forrige målt vekten
int antallHentinger = 0; // ganger man har hentet vann (= at vekten er høyere enn den målt to ganger før)
int antallDrikkinger = 0; // ganger man har drukket vann (= at vekten er lavere enn den målt to ganger før)
int antallPikslerHenting = 0; // antall piksler som skal lyses (og som signaliserer henting)
int antallPikslerDrikking = 0; // antall piksler som skal lyses (og som signaliserer drikking)
boolean foersteForandring = false; // vi regner bare med andre forandring, den første er å ta bort glassen

void setup() {

  //viserGulTre();
  pixels.begin(); // inisialiserer ringen
  pixels.clear(); // ingen pixel lyser
  pixels.setPixelColor(0,pixels.Color(250,128,114));
  pixels.show(); 
  delay(2000);
  pixels.setPixelColor(4,pixels.Color(250,128,114));
  pixels.show(); 
  delay(2000);
  pixels.clear(); // ingen pixel lyser
  pixels.show(); 

  scale.begin(DOUT, CLK); // inisialiserer vektmaaler
  scale.set_scale(calibration_factor); 
  vekt = scale.get_units();
  forrigeVekt = vekt;
  endaForrigeVekt = forrigeVekt;
  forrigeTiden=millis();
  
  Serial.begin(9600);
}

void loop() {
  vekt = scale.get_units(10);
  delay(200);
  Serial.print("hva det måler: ");
  Serial.print(vekt);
  Serial.print(" forrige: ");
  Serial.print(forrigeVekt);
  Serial.print(" enda forrige: ");
  Serial.println(endaForrigeVekt);
  
  unsigned long naaTiden=millis();
  if (naaTiden - forrigeTiden > INTERVALL) {
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
      pixels.setPixelColor(NUMPIXELS/2+i,pixels.Color(250,128,114));
    }
    pixels.show(); // viser igjen pikslene som skal lyse, etter å ha sent påminnelsen
  }
  
  if (vekt - forrigeVekt > 100.0 || forrigeVekt - vekt > 100.0) {
    if (!foersteForandring) {foersteForandring = true;}
    else {foersteForandring = false;}

    // man glemmer venting, det betyr: man begynner å telle på nytt
    forrigeTiden = naaTiden;

 //   if (vekt - endaForrigeVekt > 100.0 and !foersteForandring) {
    if (vekt - forrigeVekt > 100.0 and !foersteForandring) {
     // har hentet vann
      antallHentinger++;
      Serial.println("Hentet vann...");
    // man lyser en til lys
      pixels.setPixelColor(antallPikslerHenting,pixels.Color(250,128,114));
      pixels.show(); 
      antallPikslerHenting++;
    
 //   } else if (endaForrigeVekt - vekt > 100.0 && !foersteForandring) { 
    } else if (forrigeVekt - vekt > 100.0 && !foersteForandring) { 
      // har drukket vann
      Serial.println("Drakk vann...");
      antallDrikkinger++;
      pixels.setPixelColor(NUMPIXELS/2+antallPikslerDrikking,pixels.Color(250,128,114));
      pixels.show(); 
      antallPikslerDrikking++;
    }
    endaForrigeVekt=forrigeVekt;
    forrigeVekt=vekt;
  }
  
}
