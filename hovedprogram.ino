// DALIAS program, IN1060, 2019
#include <Adafruit_NeoPixel.h>
#define PIN        7 // for ringen
#define NUMPIXELS 12 // hvor mange pixels ringen har
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define INITIAL_DELAYVAL 50 // Tid (i millisekunder) for å vente mellom pikseler, i inisialisering
#define DELAYVAL 800 // Tid (i millisekunder) for å vente mellom pikseler, i notifiseringsdans
#define INTERVALL_UTEN_AT_NOE_SKJER 12000 // Tiden (i milisekunder) etter det systemet notifiserer brukeren at hun ikkehar beveget seg
#define UVESENTLIG_VEKT_FORSKJELL 20 // for å stabilisere vektmålinger
#define MANGE_TIMER 120000 // 2880000 ms er 8 timer: etter det uten røring skal systemet gå i dvale

#include "HX711.h"
#define calibration_factor 781.86 //-7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  3 // for vektsensor
#define CLK  2 // for vektsensor

HX711 scale;

unsigned long forrigeTidenNoeSkjedde; // for å måle tiden for å dvale
unsigned long forrigeTiden = 0; // for å måle tiden der det er ingen ting som skjer for å minne brukeren
unsigned long nytid; // for å måle tiden mellom forskjellige vektmålinger
float vekt = 0;
float nyvekt = 0; // for å drive med repetert målinger
float forrigeVekt = 0; // forrige målt vekten
float endaForrigeVekt = 0; // vekten før forrige målt vekten
int antallHentinger = 0; // ganger man har hentet vann (= at vekten er høyere enn den målt to ganger før)
int antallDrikkinger = 0; // ganger man har drukket vann (= at vekten er lavere enn den målt to ganger før)
int antallPikslerHenting = 0; // antall piksler som skal lyses (og som signaliserer henting)
int antallPikslerDrikking = 0; // antall piksler som skal lyses (og som signaliserer drikking)
boolean utAvCoaster = false; // vi regner bare med den annen forandring i vekt, den første er å ta bort glassen, som er ut av Coasteren
boolean dvaleModus = true; // systemet sover, og viser igenting

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
  visMaalinger('u');
 
  unsigned long naaTiden=millis();
  if (naaTiden - forrigeTidenNoeSkjedde > MANGE_TIMER) {
    dvaleModus=true;
    ikkeFlereLys();
  }
  if (naaTiden - forrigeTiden > INTERVALL_UTEN_AT_NOE_SKJER and !dvaleModus) {
    forrigeTiden = naaTiden;
    senderPaaminnelse();
  }
  
  if (abs(vekt - forrigeVekt) > 100) { // dette betyr at noe har skjedd
    forrigeTidenNoeSkjedde=naaTiden;
    if (dvaleModus) {
      dvaleModus=false;
      if (forrigeVekt - vekt > 100) { // har man tatt glass fra coasteren for å veke systemet
        utAvCoaster=false; // det er egentlig det motsatte fordi den toggles senere
      } else { // eller har man kommet med (ny?) glassen til coasteren for å veke systemet
        utAvCoaster=true;
      }
    }
    // stabiliserer verdien til vekt før den lagres
    nyvekt=scale.get_units(10);
    while (abs(nyvekt - vekt) > UVESENTLIG_VEKT_FORSKJELL) {
      vekt=nyvekt;
      nyvekt=scale.get_units(10);
    }
    vekt=nyvekt;
    visMaalinger('i');
    if (!utAvCoaster) {
      utAvCoaster = true;
      Serial.println("Bytter status... UtAvCoaster true");
      }
    else {
      utAvCoaster = false;
      Serial.println("Bytter status... UtAvCoaster false");
      }
    
    // man glemmer venting, det vil si, man begynner å telle på nytt
    forrigeTiden = naaTiden;

    if (vekt - endaForrigeVekt > 50 and !utAvCoaster) {
     // har hentet vann
      antallHentinger++;
      Serial.println("Hentet vann...");
      signaliserHenting();
    
    } else if (endaForrigeVekt - vekt > 50 && !utAvCoaster) { 
       // har drukket vann
      Serial.println("Drakk vann...");
      antallDrikkinger++;
      signaliserDrikking();
    }
    endaForrigeVekt=forrigeVekt;
    forrigeVekt=vekt;
  }
  
}
void visMaalinger(char hva){
    Serial.print(hva); 
    Serial.print(" hva det måler: ");
    Serial.print(vekt);
    Serial.print(" forrige: ");
    Serial.print(forrigeVekt);
    Serial.print(" enda forrige: ");
    Serial.println(endaForrigeVekt);
  }

 void senderPaaminnelse(){
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

void signaliserHenting(){
      // man lyser en til lys, foreløpig
      pixels.setPixelColor(antallPikslerHenting,pixels.Color(250,128,114));
      pixels.show(); 
      antallPikslerHenting++;
}
void signaliserDrikking(){
      // man lyser en til lys, foreløpig
      pixels.setPixelColor(NUMPIXELS/2+antallPikslerDrikking,pixels.Color(250,128,114));
      pixels.show(); 
      antallPikslerDrikking++;
}
void ikkeFlereLys(){
      pixels.clear(); // ingen pixel lyser
      pixels.show(); 
      Serial.println("går i dvale... ");
}
