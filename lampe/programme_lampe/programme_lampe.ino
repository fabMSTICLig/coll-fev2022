
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        6 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 16 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


void setup() {
  // put your setup code here, to run once:

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  Serial.begin(9600);
  pixels.clear();
}

void afficherValeur(int valeur){
  Serial.print("sensor = ");
  Serial.println(valeur);
  
}


void loop() {
  
  int x=0;
  int r=50;
  int v=50;
  int b=50;
  int lum = (analogRead(A2) + analogRead(A3)) / 2;
  int button = analogRead(A5);
  
if (lum<200){
  x=1;
}
else if (200<lum && lum<400){
  x=2;
}
else if (400<lum && lum<600){
  x=3;
}

else if (600<lum && lum<800){
  x=4;
}
else {
  x=5;
}

// blanc
if (110<button && button<120){
  r=random(1,50);
  v=random(1,50);
  b=random(1,50);
  

}
// rouge
else if (220<button && button<230){
  r=50;
  v=0;
  b=0;
}
// orange
else if (330<button && button<340){
  r=50;
  v=7;
  b=0;
}

// jaune
else if (430<button && button<450){
  r=50;
  v=25;
  b=0;
}
// vert
else if (540<button && button<550){
  r=0;
  v=50;
  b=0;
}
// bleu
else if (635<button && button<650){
  r=0;
  v=0;
  b=50;
}
// violet
else if (730<button && button<740){
  r=50;
  v=0;
  b=45;
}
// rose
else if (820<button && button<830){
  r=50;
  v=0;
  b=18;
}
r=r*x;
v=v*x;
b=b*x;

  pixels.fill(pixels.Color(r, v, b));
  pixels.show();   // Send the updated pixel colors to the hardware.
   delay(500);
}
