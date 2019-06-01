// DALIAS program, IN1060, 2019
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS_INNI_TRE 18 // hele treet
#define INITIAL_DELAYVAL 50 // Tid (i millisekunder) for å vente mellom pikseler, i inisialisering
#define DELAYVAL 800 // Tid (i millisekunder) for å vente mellom pikseler, i påminnelsesdans
#define TID_MELLOM_PAAMINNELSER 60000 // Tiden mellom de tre påminnelser (600000 ms er 10 minutter)//test med 60000 (= 1minutt)
#define INTERVALL_UTEN_AT_NOE_SKJER 300000 // Tiden (5400000 milisekunder=1,5 timer) etter det systemet notifiserer brukeren at hun ikke har beveget seg //test med 300000 (5 minutter)
#define UVESENTLIG_VEKT_FORSKJELL 20 // for å stabilisere vektmålinger
#define VEKT_FORSKJELL 40 // opprinnelig 100
#define MANGE_TIMER 1800000 // 28800000 ms er 8 timer: etter det uten røring skal systemet gå i dvale // test med 1800000 (halvtime)

// inpirert av https://www.tweaking4all.com/hardware/arduino/arduino-ws2812-led/
// fargerkodene tatt fra https://www.rapidtables.com/web/color/RGB_Color.html
#define NUMPIXELS 47 //for alt som lyser 
#define PIN_TRE 5
#define VENTETID_VISE_TRE 400 // hvor lenge skal den vise treet
Adafruit_NeoPixel strip(NUMPIXELS, PIN_TRE, NEO_GRB + NEO_KHZ800);
// Siden alt blir i serie, skal den rekkefølge av trenoder som lyses, kodes her
int rekkefolge[]={16,15,2,1,17,0};

#define VENTETID_FOR_DRIKKING 400 // ? hvor lenge skal den vise at man har drukket (tilbakemelding)

#include "HX711.h" // for vektsensor
// vi kalibrerte ved å gjenbruke kalibreringsprogram på https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide#load-cell-setup
// vi også inspirerte oss av den andre program på samme side som heter "SparkFun_HX711_Example.ino"
#define calibration_factor 781.86 //-7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  3 // for vektsensor
#define CLK  2 // for vektsensor

HX711 scale;

unsigned long forrigeTidenNoeSkjedde; // for å måle tiden for å dvale
unsigned long forrigeTiden = 0; // for å måle tiden der det er ingen ting som skjer for å minne brukeren
unsigned long nytid; // for å måle tiden mellom forskjellige vektmålinger for å stabilisere verdien
float vekt = 0;
float nyvekt = 0; // for å drive med repetert målinger
float forrigeVekt = 0; // forrige målt vekten
float endaForrigeVekt = 0; // vekten før forrige målt vekten
int antallHentinger = 0; // ganger man har hentet vann (= at vekten er høyere enn den målt to ganger før)
int antallDrikkinger = 0; // ganger man har drukket vann (= at vekten er lavere enn den målt to ganger før)
int antallPikslerHenting = 0; // antall piksler som skal lyses (og som signaliserer henting)
int antallPikslerDrikking = 0; // antall piksler som skal lyses (og som signaliserer drikking) TRENGER IKKE LENGER
boolean utAvCoaster = false; // vi regner bare med den annen forandring i vekt, den første er å ta bort glassen, som er ut av Coasteren
boolean dvaleModus = false; // systemet sover, og viser igenting
boolean straffet = false; // viser om brukeren ble straffet, for ikke å straffe en gang til
int antallPaaminnelser = 0; // antall påminnelser som systemet ga
unsigned long intervallPaaminnelser; // hvor mye skal systemet vente før den sende påminnelser, den varierer fra første til andre
int farge1[5]; // for å printe alle farger fra forrige dager på potte
int farge2[5]; // de har 3 deler
int farge3[5];
int gang=0; // for å vite hvor mange ganger etter 7 har man beveget seg

void setup() {

  strip.begin(); // inisialiserer ringen på coaster
  strip.clear(); // ingen pixel lyser
  
  // koden her er bare for å vise at alle piksler kan addresseres SKAL KOMMENTERES 
  for(int i=0; i<NUMPIXELS; i++) {
    strip.setPixelColor(i,strip.Color(250,128,114)); // blå 
    strip.show(); 
    delay(INITIAL_DELAYVAL); // venter litt
   }
  strip.clear();
  strip.show();
  // end av initialisering
  scale.begin(DOUT, CLK); // inisialiserer vektmaaler
  scale.set_scale(calibration_factor); 
  vekt = scale.get_units();
  forrigeVekt = vekt;
  endaForrigeVekt = forrigeVekt;
  forrigeTiden=millis();
  
// initialiserer serial port
  Serial.begin(9600);
  
  intervallPaaminnelser=INTERVALL_UTEN_AT_NOE_SKJER;
/*
  for(int i=0; i<5; i++) { // For hver dag
   farge1[i]=0; 
   farge2[i]=0; 
   farge3[i]=0;
  }
*/
  // effekter for å filme: da skal de vise en "vanlig dag"

  farge1[0]=255; 
  farge2[0]=165; 
  farge3[0]=0; // gull   
  farge1[1]=255; 
  farge2[1]=0; 
  farge3[1]=0; // rød
  farge1[2]=255; 
  farge2[2]=165; 
  farge3[2]=0; // gull   
  farge1[3]=0; 
  farge2[3]=150; 
  farge3[3]=0; // grønn
  farge1[4]=255; 
  farge2[4]=165; 
  farge3[4]=0; // gull   
 
  skriver_farger_pott();
}

void loop() {
  vekt = scale.get_units(10);
  visMaalinger('u');
 
  unsigned long naaTiden=millis();
  if (naaTiden - forrigeTidenNoeSkjedde > MANGE_TIMER) { //kommer inn i dvale modus
    if (!dvaleModus) { //første gang
      ikkeFlereLys();
      straffet=false;
      for(int i=0; i<4; i++) { // For hver dag
        farge1[i]=farge1[i+1];
        farge2[i]=farge2[i+1];
        farge3[i]=farge3[i+1];
        // sette farge til dagens, farge1[4] osv blir gjort når man tegner treet
      }
    }
    dvaleModus=true;
  }
  if (naaTiden - forrigeTiden > intervallPaaminnelser and !dvaleModus and !straffet) {
    forrigeTiden = naaTiden;
    senderPaaminnelse();
    intervallPaaminnelser=TID_MELLOM_PAAMINNELSER;
    if (antallPaaminnelser == 3) {
      straffTre();
      antallPaaminnelser = 0;
      intervallPaaminnelser=INTERVALL_UTEN_AT_NOE_SKJER;
      straffet=true;
    }
  }
  
  if (abs(vekt - forrigeVekt) > VEKT_FORSKJELL) { // dette betyr at noe har skjedd
    forrigeTidenNoeSkjedde=naaTiden;
    // man glemmer venting, det vil si, man begynner å telle på nytt
    forrigeTiden = naaTiden;
    antallPaaminnelser = 0;
    intervallPaaminnelser=INTERVALL_UTEN_AT_NOE_SKJER;
    
    if (dvaleModus) {
      dvaleModus=false;
      viserTre();
      antallHentinger=0;
      antallDrikkinger=0;
   
      if (forrigeVekt - vekt > VEKT_FORSKJELL) { // har man tatt glass fra coasteren for å veke systemet
        utAvCoaster=false; // det er egentlig det motsatte fordi den toggles senere
      } else { // eller har man kommet med (ny?) glassen til coasteren for å veke systemet
        utAvCoaster=true;
      }
      visMaalinger('o');
      
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
    
   
    if (vekt - endaForrigeVekt > VEKT_FORSKJELL and !utAvCoaster) { // det var 50 fçr
     // har hentet vann
      antallHentinger++;
      Serial.println("Hentet vann...");
      signaliserHenting();
      straffet=false; // da kan brukeren ble straffet igjen
    
    } else if (endaForrigeVekt - vekt > VEKT_FORSKJELL && !utAvCoaster) { 
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
    for(int i=23; i<47; i++) { // For hver piksel av de siste 24
      strip.setPixelColor(i, strip.Color(0, 255, 0)); //grønn
      strip.show();   // viser fargene
      delay(INITIAL_DELAYVAL); // venter litt
  
    }
    for(int i=23; i<47; i++) { // For hver piksel av de siste 24
        strip.setPixelColor(i, strip.Color(0, 0, 0)); //ingen ting
    }
    strip.show();
    antallPaaminnelser++;
}

void signaliserHenting(){
      if (antallPikslerHenting < 6) {  // lyser på tre, små noder
        strip.setPixelColor(rekkefolge[antallPikslerHenting],strip.Color(0,150,0)); // skal være grønn
        strip.show(); 
        antallPikslerHenting++;
      } else if (antallPikslerHenting == 6) { //lyser hele rundning
        for (int i=3; i < 15; i++) {
          strip.setPixelColor(i,strip.Color(0,150,0));
        }
        strip.show();
        antallPikslerHenting++;
        gang=0;
      } else if (antallPikslerHenting <11) { // lyser på ringen på tre, tre av gangen
        gang++;
        antallPikslerHenting++;
        for (int i=3; i < 15; i++) { //renser fra fçr
          strip.setPixelColor(i,strip.Color(0,0,0));
        }
        for (int i=3; i<3+3*gang; i++) { // lyser dem
          strip.setPixelColor(i,strip.Color(0,150,0));
        }
        strip.show();
      } else {//mer enn 11 gjør det ingen ting
      }
}
void signaliserDrikking(){
      //viser en effekt som vann som gaar  i treet
      for(int i=0; i< 4; i++) { // Viser de 4 fire opp
        strip.setPixelColor(rekkefolge[i], strip.Color(250,128,114)); //blå
      }
      strip.show();   // viser fargene
      delay(INITIAL_DELAYVAL*4); // venter litt
      strip.clear(); // ingen pixel lyser
      for(int i=4; i< 6; i++) { // Viser de 4 fire opp
        strip.setPixelColor(rekkefolge[i], strip.Color(250,128,114)); //blå
      }
      strip.show(); 
      delay(INITIAL_DELAYVAL*4); // venter litt
      strip.clear(); // ingen pixel lyser
      for(int i=3; i< 15; i++) { // Viser de på ring på midten av treet
        strip.setPixelColor(i, strip.Color(250,128,114)); //blå
      }
      strip.show(); 
      delay(INITIAL_DELAYVAL*4); // venter litt
      antallPikslerDrikking++;
      strip.clear(); // ingen pixel lyser
      //sette tilbake det som skulle vises
      for(int i=0; i<antallPikslerHenting; i++) {
        strip.setPixelColor(rekkefolge[i], strip.Color(0, 255, 0)); //grønn
      }
      if (antallPikslerHenting == 7) {
       for(int i=3; i< 15; i++) { // Viser de på ring på midten av treet
        strip.setPixelColor(i, strip.Color(0,255,0)); // grønn  
        }
      } else {
          for (int i=3; i<3+3*gang; i++) { // lyser dem
          strip.setPixelColor(i,strip.Color(0,255,0));
        }
      }
      strip.show();  
      skriver_farger_pott();
}
void ikkeFlereLys(){
      antallPikslerDrikking=0;
      antallPikslerHenting=0; 
      gang=0;
      strip.clear();
      strip.show();
      Serial.println("går i dvale... ");
      for (int i=0;i<5;i++) {
        Serial.print("farge: ");
        Serial.print(farge1[i]);
        Serial.print("  ");
        Serial.println(farge2[i]);
      }
}

void viserTre(){
  // indikerer farge på treet og setter tallet på farge[4]
     for(int i=0; i<NUMPIXELS_INNI_TRE; i++) { // for hver piksel
          if (antallHentinger < 3) {
            strip.setPixelColor(i, strip.Color(255,0,0)); // her er rød
            Serial.println("Treet er rød");
            farge1[4]=255; farge2[4]=0; farge3[4]=0;
          } else if (antallHentinger < 6) {
            strip.setPixelColor(i, strip.Color(255,128,214)); // her er gul men jeg setter den blaa
            Serial.println("Treet er gul");
            farge1[4]=255; farge2[4]=165; farge3[4]=0;
          } else {
            strip.setPixelColor(i, strip.Color(0, 150, 0)); // her er grønn
            Serial.println("Treet er grønn");
            farge1[4]=0; farge2[4]=150; farge3[4]=0;
          }
          strip.show();   // Vise
          delay(20);
       }
          delay(VENTETID_VISE_TRE); // Vente

       strip.clear(); 
       strip.show();
    skriver_farger_pott();
}
void straffTre() {
  // her skal en lys gaa ned
  if (antallPikslerHenting < 7) {
    strip.setPixelColor(rekkefolge[antallPikslerHenting-1],strip.Color(255,0,0));
    strip.show();
    delay(20);
    strip.setPixelColor(rekkefolge[antallPikslerHenting-1],strip.Color(0,0,0));

  } else if (antallPikslerHenting == 7) {
    for (int i=3; i < 15; i++) {
       strip.setPixelColor(i,strip.Color(0,0,0));
    }
  } else if (gang) { // det betyr det er høyere enn 7
      for (int i=3+3*(gang-1); i<3+3*gang; i++) { // sletter dem
          strip.setPixelColor(i,strip.Color(0,0,0));
        }
        if (gang==1) {//maa skrive hele runding igjen
          for (int i=3; i < 15; i++) {
            strip.setPixelColor(i,strip.Color(0,150,0));
          }
        }
        gang--;
  } 
  strip.show();
  antallPikslerHenting--;
}
void skriver_farger_pott() {
  // lyser 5 lys (fra 18 til 22) som viser det man ha beveget seg de forrige fem dager
  // den gaar ned siden de ble loddet i motsatterekkefçlge
  for(int i=0; i<5; i++) { // For hver dag
     strip.setPixelColor(22-i,strip.Color(farge1[i],farge2[i],farge3[i]));
    }
    strip.show();
}
