#include <Wire.h> //bibliothèque pour la communication avec les capteurs (I2C)
#include <PCF85063TP.h> //bibliothèque pour le capteur horloge
#include <LiquidCrystal_PCF8574.h>
#include <Servo.h> //bibliothèque pour le servo moteur

#include <SoftwareSerial.h>//bibliothèque pour la cimmonucation avec le lecteur mp3
#include <DFRobotDFPlayerMini.h>//bibliothèque pour le lecteur mp3

#include <EEPROM.h>//bibliothèque pour sauvgarder des données 

//configuration de la bibliothèque encodeur
#define ENCODER_USE_INTERRUPTS
//#define ENCODER_DO_NOT_USE_INTERRUPTS
#include <Encoder.h>//bibliothèque pour le bouton rotatif

#include <Bounce2.h>//bibliothèque pour gérer les evenements du bouton

//objet permettant de controller l'écran LCD
LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 20 chars and 4 line display
//objet permettant de controller l'horloge RTC
//RtcDS3231<TwoWire> Rtc(Wire);
PCD85063TP clock;
//objet permettant d'utiliser la communication avec le lecteur mp3
SoftwareSerial mySoftwareSerial(5, 6);
//objet permettant de controller le lecteur mp3
DFRobotDFPlayerMini myDFPlayer;
//objet permettant de contrôller le bouton rotatif
Encoder myEnc(2, 3);

//constante definissant le numéro de la pin où est branché le bouton
#define REVEIL_BUT 7
//objet permettant de gerer les evenements du bouton
Bounce reveil_bouton = Bounce();

//constantes indiquant les adresses où sont stokées les valeurs de l'alarme
#define EEPROMAHEURE 0
#define EEPROMAMINUTE 1
#define EEPROMAACTIVE 2

#define PIN_SUPER_BOUTON 4


bool backlight = true;

//variable contenant l'heure de l'alarme
unsigned char aheure = 9;
//variable contenant les minutes de l'alarme
unsigned char aminute = 0;
//variable booléen indiquant si l'alarme est active où non
bool alarmeon = false;
//variable contenant l'heure
unsigned char heure = 9;
//variable contenant la minute actuelle
int min = 0;
//variable contenant la seconde actuelle
int sec = 0;

//variable contenant l'état du sequenceur gerant le menu et l'alarme
int mode = 0;


Servo myservo; // variable pour le servo moteur


void init_lcd()
{
    int error = 0;
    Wire.begin();
    Wire.beginTransmission(0x27);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.println("LCD found.");
      lcd.begin(20, 4); // initialize the lcd

  } else {
    Serial.print("Error: ");
    Serial.print(error);
    Serial.println(": LCD not found.");
  }
  return (error);
}

void init_servo()
{
  myservo.attach(8);
  pinMode(PIN_SUPER_BOUTON, INPUT_PULLUP);
}

//Fonction executée au démarage de la carte (mise sous tension)
void setup() {
  //Initialise la liaison série
  Serial.begin(9600);

  init_servo();
  init_lcd();
  
  //initialise le bouton principal
  reveil_bouton.attach(REVEIL_BUT,INPUT_PULLUP);
  //defini le temps de réaction du bouton en milliseconde
  reveil_bouton.interval(25);

  //initialise la communication avec le lecteur mp3
  mySoftwareSerial.begin(9600);
  //Initialise le lecteur mp3, si cela echou on affiche un message. 
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  else //message affiché si l'initialisation c'est bien passée
  {
    Serial.println(F("DFPlayer Mini online."));
  }

  //on set le volume au maximum
  myDFPlayer.volume(30);

  //initialisation du capteur de l'horloge
  clock.begin();
  //Réglage de l'horloge
  uint8_t ret = clock.calibratBySeconds(0, -0.000041);
  
  //on efface l'écran
  lcd.clear();
  lcd.setBacklight(255);


  //On récupère les données en mémoire et on initialise les variables
  setTempsAlarme(EEPROM.read(EEPROMAHEURE), EEPROM.read(EEPROMAMINUTE));
//   setColor(EEPROM.read(EEPROMCR),EEPROM.read(EEPROMCV),EEPROM.read(EEPROMCB));
  alarmeon = EEPROM.read(EEPROMAACTIVE);
}

//fonction modifiant l'heure a laquelle l'alarme doit démarrer
void setTempsAlarme(unsigned char heure, unsigned char min)
{
  if (heure > 23)
    heure = 0;
  if (min > 59)
    min = 0;
  aheure = heure;
  aminute = min;
  EEPROM.write(EEPROMAHEURE, aheure);
  EEPROM.write(EEPROMAMINUTE, aminute);
}

void setTime(unsigned char pheure, unsigned char pminute, unsigned char pseconde)
{
  if (pheure > 23)
    pheure = 0;
  if (pminute > 59)
    pminute = 0;
  if (pseconde > 59)
    pseconde = 0;
  heure = pheure;
  min = pminute;
  sec = pseconde;
  clock.stopClock();
  clock.fillByHMS(heure, min, sec );
  clock.setTime();
  clock.startClock();
  
}

//on déclenche ou on arrete l'alarme
void setAlarme(bool on)
{
  if(on)//si on déclenche
  {
    mode = 10;//on passe en mode 10
    myDFPlayer.play(1); // on déclenche la musique
    // ecrit un message sur l'ecran en le faisant clignoté
    lcd.blink();
    lcd.print("C'est l'heure!!!");
  }
  else
  {
      mode = 0;//on repasse en mode 0 (affichage de l'heure)
      myDFPlayer.pause(); //on arrete la musique
      lcd.noBlink(); //on arrete le clignotement de l'écran
  }
}

///////////////////////////////////////
//Variables et fonction permettant de claculer le nombre de pas tourné sur le bouton rotatif
//la fonction renvoie le nombre de pas effectué durant les 200 dernières miliseconde
// long lastupdate = 0;
long endupdate = 0;
long startupdate = 0;
long oldPosition = -999;
long storePosition = -999;

int calculEncDiff()
{
  
  long newPosition = myEnc.read();
  if (newPosition != oldPosition)
  {
    if (startupdate == 0)
        startupdate = millis();
    endupdate=millis();
    oldPosition = newPosition;
  }
  if ((endupdate!=0 && millis()-endupdate>100) || ((millis()-startupdate>200) &&storePosition!=oldPosition))
  {
    int diff = (storePosition - oldPosition) / 4;
    storePosition = oldPosition;
    endupdate = 0;
    startupdate = 0;
    return (-diff); // /!\ signe !?
  }
  return 0;
}
//////////////////////////

//Fonction affichant l'heure sur l'écran LCD
void printTime()
{
//   if (sec != clock.second) //Si on a changé de sec on met à jour l'affichage
//   {
    //on met à jour la sec actuelle
    sec = clock.second;
    // on se déplace sur la colone 4 et la ligne 0 sur l'ecran LCD
    // répétition pour éviter les erreurs d'affichage
    lcd.setCursor(5, 1);
    // lcd.setCursor(5, 1);
    // lcd.setCursor(5, 1);
    if (clock.hour < 10)
        lcd.print(0); //Si entre 0 et 9 on affiche un 0 avant
    lcd.print(clock.hour, DEC);//affichage de l'heure
    lcd.print(":");
    if (clock.minute < 10)
        lcd.print(0);
    lcd.print(clock.minute, DEC);
    lcd.print(":");
    if (clock.second < 10)
        lcd.print(0);
    lcd.print(clock.second, DEC);
//   }
}

//Fonction affichant le menu de l'alarme sur l'écran LCD
void printAlarme()
{
  lcd.setCursor(6, 0);
  lcd.print("Alarme");
  lcd.setCursor(1, 2);
  if (alarmeon)
    lcd.print("On");
  else
    lcd.print("Off");
  lcd.setCursor(5, 2);
  if (aheure < 10)
    lcd.print("0");
  lcd.print(aheure, DEC);
  lcd.print(":");
  if (aminute < 10)
    lcd.print(0);
  lcd.print(aminute, DEC);
}

//Fonction affichant le menu de l'horloge sur l'écran LCD
void printConfTemps()
{
  lcd.setCursor(6, 0);
  lcd.print("HEURE");
  lcd.setCursor(1, 2);
  lcd.setCursor(5, 2);
  if (heure < 10)
    lcd.print("0");
  lcd.print(heure, DEC);
  lcd.print(":");
  if (min < 10)
    lcd.print(0);
  lcd.print(min, DEC);
  lcd.print(":");
  if (sec < 10)
    lcd.print(0);
  lcd.print(sec, DEC);
}

//Fonction contenant le sequenceur gérant le menu et l'alarme
//enc est le nombre de pas tourné sur le bouton rotatif (0 si immobile)
void menu(int enc){
  
  Serial.print("Mode: ");
  Serial.println(mode);
  
  //booleen indiquant s'il faut mettre à jour l'affichage
  //bouton appuyé ou bouton rotatif tourné
  bool change = false;
  if (reveil_bouton.fell()) //Si on appuie sur le bouton
  {
    change = true; //on mettera a jour l'affichage
    if (mode < 10)//Si on est en mode menu (mode inférieur à 10)
    {
      if (mode == 0) // mode normal : heure
      {
        heure = clock.hour;
        min = clock.minute;
        sec = clock.second;

      }
      mode++;
      if (mode > 7)
      {
        mode = 0;
        lcd.noCursor();
      }

    } 
    else if(mode == 10) //Si mode 10 (alarme allumé) on l'éteint
    {
      setAlarme(false);
    }
  }

  //Gestion du menu selon les valeurs du bouton rotatif
  if(enc != 0)//Si le bouton rotatif a été tourné, on mettera à jour l'affichage 
  {
    change = true;
  }
  if(mode == 1)//Mode 1 on change l'activation de l'alarme
  {
    if(enc!=0)
        alarmeon = ! alarmeon;
    EEPROM.write(EEPROMAACTIVE, alarmeon);
  }
  else if(mode == 2)//Mode 2 on change l'heure de l'alarme
  {
    if(enc != 0)
    {
      setTempsAlarme(aheure+enc,aminute);
    }
  }
  else if(mode == 3)//Mode 3 on change les minutes de l'alarme
  {
    if(enc != 0)
    {
      setTempsAlarme(aheure,aminute+enc);
    }
  }
  else if (mode == 4) // Mode 4 on change les heures
  {
        setTime(heure + enc, min, sec);
  }
  else if (mode == 5) // Mode 5 on change les minutes
  {
        setTime(heure,min + enc, sec);
  }
  else if (mode == 6) // Mode 6 on change les secondes
  {
        setTime(heure,min, sec+enc);
  }
  


  //Si il faut mettre à jour l'affichage, on efface l'écran
  if(change)
  {
    Serial.println("Clear");
    lcd.setCursor(0,0);
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.clear();   
  }

  //Gestion de l'affichage
  if (mode == 0)//Mode 0 affichage de l'heure
  {
    printTime();
    Serial.print("mode0, time: heure:");
    Serial.print(heure);
    Serial.print(" : ");
    Serial.print(min);
    Serial.print(" : ");
    Serial.print(sec);
    
  }
  else if (1 <= mode && mode <= 3 && change)//Mode 1 à 3 affichage du menu de l'alarme
  {
    Serial.println("Menu Alarme");
    printAlarme();
    //on affiche le chevron sur le paramètre à modifier
    if (mode == 1)
    {
      lcd.setCursor(0,2);
      lcd.print(">");
    } else if(mode == 2)
    {
      lcd.setCursor(5,3);
      lcd.print("^^");
    } else if (mode == 3)
    {
      lcd.setCursor(8,3);
      lcd.print("^^");
    }
  }
  else if(4 <= mode && mode <= 6 && change)//Mode 4 à 6 affichage du menu de l'heure
  {
    Serial.println("Menu Changement d'heure");

    printConfTemps();
    //on affiche le chevron sur le paramètre à modifier
    if (mode == 4)
    {
      lcd.setCursor(5,3);
      lcd.print("^^");
    }
    else if(mode == 5)
    {
      lcd.setCursor(8,3);
      lcd.print("^^");
    }
    else if (mode == 6)
    {
      lcd.setCursor(11,3);
      lcd.print("^^");
    }
  }
  else if (mode == 7 && change) // Mode 7 active/desactive lumière écran
  {
    lcd.setCursor(1,1);
    lcd.clear();
    lcd.print("luminositee ecran?");
    if(enc != 0)
    {
      if (backlight)
        lcd.setBacklight(255);
      else
        lcd.setBacklight(0);
      backlight = !backlight;
    }
  }
}

//Fonction étant exécutée en boucle
void loop() {
  /// USLESS BOX 
  int tableau_angle_servo[2] = {110, 40};
  int super_bouton = digitalRead(PIN_SUPER_BOUTON);
  int pos = tableau_angle_servo[super_bouton];
//      if (super_bouton == false) {
//        myDFPlayer.volume(0);
//delay(1);
//      }
//      else {
//        myDFPlayer.volume(30);
//delay(1);
//      }
  myservo.write(pos);


  
  /// REVEIL
  //on met a jour les objets en fonction du bouton principal
  reveil_bouton.update();
  //on récupère la valeur du bouton rotatif
  int encDiff= calculEncDiff();

  //on gère le menu et l'alarme
  menu(encDiff);

  clock.getTime();
  //Si la min actuelle a changé on vérifie l'alarme 
  if(min != clock.minute)
  {
    min = clock.minute;
    //Si l'heure et les minutes correspondent on déclenche l'alarme
    if(clock.minute == aminute && clock.hour == aheure && mode==0 && alarmeon)
    {

      
      lcd.setCursor(0,0);
      lcd.clear();
      lcd.setCursor(0,1);
      lcd.clear();
      setAlarme(true);
    }
  }
  //on met en pause 10 milliseconde, ce qui économise de l'énergie`
  delay(10);
}
