#include <Key.h>
#include <Keypad.h>

#include <math.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3f, 16, 2);


const byte ROWS = 5;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'A', 'B', 'C', 'D'},
  {'1', '2', '3', 'E'},
  {'4', '5', '6', 'F'},
  {'7', '8', '9', 'G'},
  {'H', '0', 'I', 'J'},
};

byte rowPins[ROWS] = {31, 33, 35, 37, 39};
byte colPins[COLS] = {47, 45, 43, 41};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

const int pin_temperatura = 0; //Sensore temperatura ANALOG
const int pin_hum = 1; //Sensore umidità ANALOG
const int buttonPin = 18; //Bottone per interrupt
const int rele_temp = 24; //Relè per accensione e spengimento elemento riscaldante
const int pin_tone = 26; //Altoparlante
const int pin_temp_switch = 49;  //Interruttore circuito temperatura DIGITAL
const int pin_manauto_switch = 8; //Interruttore man/auto
const int pin_water = 9; //Sensore acqua DIGITAL
const int rele_water = 10;

//DICHIARAZIONE DEI PIN DEI LED
const int pin_hum_high = 30;
const int pin_hum_ok = 34;
const int pin_hum_low = 46;
const int pin_temp_high = 32;
const int pin_temp_ok = 36;
const int pin_man = 40;
const int pin_auto = 42;
const int pin_temp_low = 48;
const int pin_temp_general = 44;
const int pin_water_ok = 38;
const int pin_water_low = 50;
const int pin_general = 52;

double VOLT = 5;
double v;
double volt;
double Rt;
double R1 = 10000;
double temp; //temperatura in Kelvin
double Temperatura; //da trasformare in Celsius
double Humidity; //Prova umidità

bool flag, flag1 = false, flag_temp, flag_hum;

bool flag_temp_min = false, flag_temp_max = false, flag_hum_min = false, flag_hum_max = false;
bool errore_temp = false, errore_hum = false, errore_temp_max = false, errore_temp_min = false, errore_hum_max = false, errore_water = false;

bool disattiva_allarme_temp = false;
bool disattiva_allarme_hum = false;
bool disattiva_allarme_water = false;

bool flag_rele;
bool manuale, automatico;

bool flag_timer=true;

bool flag_acqua=false;
bool dummy_acqua=false;

volatile int stato = LOW;
int prova = 13;
int i = 0;
float somma;

float temp_min = 30, temp_max = 32;
float hum_min = 20, hum_max = 40;


const int AirValue = 791;
const int WaterValue = 390;
int intervals = (AirValue - WaterValue) / 3;
float soilMoistureValue = 0;

int counter = 0;
int timer = 21;
int acqua = 0;


void setup() {
  Serial.begin(9600);
  pinMode(prova, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(rele_temp, OUTPUT);
  pinMode(pin_tone, OUTPUT);
  pinMode(pin_manauto_switch, INPUT_PULLUP);
  pinMode(pin_temp_switch, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(18), cambiaStato, FALLING);

  
  //LED
  pinMode(pin_hum_high, OUTPUT);
  pinMode(pin_temp_high, OUTPUT);
  pinMode(pin_hum_ok, OUTPUT);
  pinMode(pin_temp_ok, OUTPUT);
  pinMode(pin_water_ok, OUTPUT);
  pinMode(pin_man, OUTPUT); //man switch (led)
  pinMode(pin_auto, OUTPUT); //auto switch (led)
  pinMode(pin_temp_general, OUTPUT); //generic temperature switch
  pinMode(pin_hum_low, OUTPUT);
  pinMode(pin_temp_low, OUTPUT);
  pinMode(pin_water_low, OUTPUT);
  pinMode(pin_general, OUTPUT);
  pinMode(pin_water, INPUT);
  pinMode(rele_water, OUTPUT);

  int s;
  int t;
  for (t = 0; t < 3; t++)
  {
    for (s = 30; s <= 52; s = s + 2)
    {
      delay(20);
      digitalWrite(s, HIGH);
      tone(pin_tone, s * 10 + 200, 500);
      delay(30);
      digitalWrite(s, LOW);
      //tone(pin_tone, 500, 500);

    }
  }


  //LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
}


/*---------------------------------------------------------------INIZIO LOOP------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void loop() {

  if(counter<15)digitalWrite(pin_water,HIGH);
  lcd.setCursor(0, 0);
  digitalWrite(pin_general, HIGH);


  if (digitalRead(pin_manauto_switch) == LOW) {
    automatico = false;
    manuale = true;
    flag_rele = false; //NUOVA AGGIUNTA
  }
  else {
    automatico = true;
    manuale = false;
    flag_rele = true;
  }



  //FUNZIONE CALCOLO TEMPERATURA
  somma = 0;
  for (i = 0; i < 10; i++)
  {
    v = analogRead(pin_temperatura);
    delay(10);
    somma = somma + v;

  }
  v = somma / 10;
  volt = VOLT * v / 1023;
  Rt = R1 * (VOLT / volt - 1);
  temp = 1 / (0.001319 + (0.000234125 * log(Rt)) + (0.0000000876741 * log(Rt) * log(Rt) * log(Rt)));
  Temperatura = (812 - 3 * abs(temp - 273.15)) / 25;

  somma = 0;
  for (i = 0; i < 10; i++)
  {
    soilMoistureValue = analogRead(pin_hum); //putSensorinsertintosoil
    delay(10);
    somma = somma + soilMoistureValue;
  }
  soilMoistureValue = somma / 10;
  Humidity = (79500 - (100 * soilMoistureValue)) / 401;

  /*SICCOME IL SENSORE È CAPACITIVO SE INTERRUTORE THERMO GENERAL
    ACCESO CALA TENSIONE E SBALLA VALORE UMIDITÀ QUINDI LO AGGIUSTIAMO MANUALMENTE
  */

  if (digitalRead(pin_temp_switch) == HIGH)Humidity = Humidity - 3;
  if (digitalRead(rele_water) == LOW)Humidity= Humidity -3.3;
  if (Humidity < 0)Humidity = 0;


  delay(50);



  char key = keypad.getKey();
  
    //VISUALIZZAZIONE TASTO PREMUTO
    if (key) {
    Serial.print("Key: ");
    Serial.println(key);
    }
  

  /*----------------------MENU--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


  if (key == 'A')
  {
    lcd.clear();
    lcd.print("MENU");
    lcd.setCursor(0, 1);
    lcd.print("F2-T #-H *-H20");
    flag = true;
    //flag1 = true; //AGGIUNTA
  }
  if (key == 'G' && flag == true) {  //CANCELLO IL MENU
    lcd.clear();
    flag = false;
    flag_hum = false;
    flag_temp = false;
    flag1 = false;
    flag_temp_min = false;
    flag_temp_max = false;
    flag_hum_min = false;
    flag_hum_max = false;
  }

  if (key == 'B' && flag == true)
  {
    flag_hum = false;
    lcd.clear();
    lcd.print("Tmin=");
    lcd.print(temp_min);
    lcd.print(" Tmax=");
    lcd.print(temp_max);
    lcd.setCursor(0, 1);
    lcd.print("set_m 1 set_M 2");
    flag_temp = true;
  }
  //REGOLAZIONE VALORI MIN/MAX DELLA TEMPERATURA
  if (key == '1' && flag_temp == true && flag == true)
  {
    lcd.clear();
    lcd.print("Set T_min");
    lcd.setCursor(0, 1);
    lcd.print("T_min=");
    lcd.print(temp_min);
    flag_temp_min = true;
    flag_temp_max = false;
  }

  if (key == 'E' && flag_temp == true && flag == true && flag_temp_min == true) {
    lcd.clear();
    temp_min = temp_min + 1;
    if (temp_min > 49)temp_min = 49;
    lcd.print("Set T_min");
    lcd.setCursor(0, 1);
    lcd.print("T_min=");
    lcd.print(temp_min);
  };


  if (key == 'F' && flag_temp == true && flag == true && flag_temp_min == true) {
    lcd.clear();
    temp_min = temp_min - 1;
    if (temp_min < -10)temp_min = -10;
    lcd.print("Set T_min");
    lcd.setCursor(0, 1);
    lcd.print("T_min=");
    lcd.print(temp_min);
  };

  if (key == '2' && flag_temp == true && flag == true )
  {
    lcd.clear();
    lcd.print("Set T_max");
    lcd.setCursor(0, 1);
    lcd.print("T_max=");
    lcd.print(temp_max);
    lcd.print(" C");
    flag_temp_min = false;
    flag_temp_max = true;
  }

  if (key == 'E' && flag_temp == true && flag == true && flag_temp_max == true) {
    lcd.clear();
    temp_max = temp_max + 1;
    if (temp_max > 50)temp_max = 50;
    lcd.print("Set T_max");
    lcd.setCursor(0, 1);
    lcd.print("T_max=");
    lcd.print(temp_max);
    lcd.print(" C");
  };


  if (key == 'F' && flag_temp == true && flag == true && flag_temp_max == true) {
    lcd.clear();
    temp_max = temp_max - 1;
    if (temp_max < -9)temp_max = -9;
    lcd.print("Set T_max");
    lcd.setCursor(0, 1);
    lcd.print("T_max=");
    lcd.print(temp_max);
    lcd.print(" C");
  };
  //FINE REGOLAZIONE VALORI MIN/MAX DELLA TEMPERATURA

  //SELEZIONE UMIDITÀ DA MENU
  if (key == 'C' && flag == true)
  {
    flag_temp = false;
    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Hmin=");
    lcd.print(hum_min);
    lcd.print(" H_max=");
    lcd.print(hum_max);
    lcd.setCursor(0, 1);
    lcd.print("set_m 1 set_M 2");
    flag_hum = true;
  }

  //REGOLAZIONE DEI VALORI MIN/MAX DELL'UMIDITÀ
  if (key == '1' && flag_hum == true && flag == true)
  {
    lcd.clear();
    lcd.print("Set H_min");
    lcd.setCursor(0, 1);
    lcd.print("H_min=");
    lcd.print(hum_min);
    lcd.print(" %");
    flag_hum_min = true;
    flag_hum_max = false;
  }

  if (key == 'E' && flag_hum == true && flag == true && flag_hum_min == true) {
    lcd.clear();
    hum_min = hum_min + 1;
    if (hum_min > 99)hum_min = 99;
    lcd.print("Set H_min");
    lcd.setCursor(0, 1);
    lcd.print("H_min=");
    lcd.print(hum_min);
    lcd.print(" %");
  };

  if (key == 'F' && flag_hum == true && flag == true && flag_hum_min == true) {
    lcd.clear();
    hum_min = hum_min - 1;
    if (hum_min < 0)hum_min = 0;
    lcd.print("Set H_min");
    lcd.setCursor(0, 1);
    lcd.print("H_min=");
    lcd.print(hum_min);
    lcd.print(" %");
  };

  if (key == '2' && flag_hum == true && flag == true )
  {
    lcd.clear();
    lcd.print("Set H_max");
    lcd.setCursor(0, 1);
    lcd.print("H_max=");
    lcd.print(hum_max);
    lcd.print(" %");
    flag_hum_min = false;
    flag_hum_max = true;
  }

  if (key == 'F' && flag_hum == true && flag == true && flag_hum_max == true) {
    lcd.clear();
    hum_max = hum_max - 1;
    if (hum_max < 1)hum_max = 1;
    lcd.print("Set H_max");
    lcd.setCursor(0, 1);
    lcd.print("H_max=");
    lcd.print(hum_max);
    lcd.print(" %");
  };

  if (key == 'E' && flag_hum == true && flag == true && flag_hum_max == true) {
    lcd.clear();
    hum_max = hum_max + 1;
    if (hum_max >= 100)hum_max = 100;
    lcd.print("Set H_max");
    lcd.setCursor(0, 1);
    lcd.print("H_max=");
    lcd.print(hum_max);
    lcd.print(" %");
  };
  //FINE REGOLAZIONE DEI VALORI MIN/MAX DELL'UMIDITÀ


   Serial.println(flag);
   Serial.println(flag_acqua);
   Serial.print("var acqua:");
   Serial.println(acqua);
   Serial.print("Counter:");
   Serial.println(counter);
   Serial.print("timer:");
   Serial.println(timer);
   Serial.println("---------------------");
       
       //EROGAZIONE ACQUA MANUALE
        if (key == 'D' && flag == true || dummy_acqua == true)
        {
          dummy_acqua = true;
          digitalWrite(rele_water, LOW);
          acqua = acqua + 1;
          if (acqua > 10)
          {
            dummy_acqua = false;
            digitalWrite(rele_water, HIGH);
          }
          if (dummy_acqua == false)acqua = 0;
        }

  
  
  if ((flag == false && flag1 == false && errore_temp_max == false && errore_temp_min == false && errore_hum_max == false && errore_temp == false) || (flag_temp_min == false && flag_temp_max == false && flag_hum_min == false && flag_hum_min == false && errore_temp_max == false && errore_hum == false && errore_temp == false && key == '1')) {

    lcd.print("T= ");
    lcd.print(int(Temperatura * 10) / 10.);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("H= ");
    lcd.print(int(Humidity * 10) / 10.);
    if (Humidity * 10 / 10. < 10)lcd.print(" %    1/4");
    else lcd.print(" %   1/4");
    flag_hum = false;
    flag_temp = false;
  }
  if (flag == false && key == '2')
  {
    lcd.clear();
    if (digitalRead(rele_temp) == LOW)lcd.print("heat ON");
    else lcd.print("heat OFF");
    lcd.setCursor(0, 1);
   if (digitalRead(rele_water) == HIGH) {
            lcd.print("water OFF");
            lcd.print("    2/4");
          }
          else {
            lcd.print("water ON");
            lcd.print("     2/4");
          }
    flag1 = true;
  }

  if (flag == false && key == '3')
  {
    lcd.clear();
    lcd.print("minT=");
    lcd.print(temp_min);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("maxT=");
    lcd.print(temp_max);
    if (temp_max < 10)lcd.print(" C     3/4");
    else lcd.print(" C    3/4");
    flag1 = true;
  }
  if (flag == false && key == '4')
  {
    lcd.clear();
    lcd.print("minH=");
    lcd.print(hum_min);
    lcd.print(" %");
    lcd.setCursor(0, 1);
    lcd.print("maxH=");
    lcd.print(hum_max);
    if (hum_max < 10)lcd.print(" %     4/4");
    else lcd.print(" %    4/4");
    flag1 = true;
  }
  //FINE MENU



  /*----------------------AUTOMAZIONE------------------------------------------------------------------------------------------------------------------------------------*/

  /*qua inizia la parte del programma che regola
    l'automazione della serra tramite la gestione
    degli errori e il controllo della temperatura,
    dell'umidità, dell'acqua e della luce*/



  swap_minmax(temp_min,temp_max);
  swap_minmax(hum_min,hum_max);
  

  //CONTROLLO TEMPERATURA
  if (Temperatura < temp_min && flag_rele == true && automatico == true)digitalWrite(rele_temp, LOW);
  else if (Temperatura >= temp_min && automatico == true)digitalWrite(rele_temp, HIGH);


  if (manuale == true && Temperatura < temp_min && flag == false)
  {
    lcd.clear();
    if (errore_temp_min == false)lcd.clear();
    lcd.print("ERRORE");
    lcd.setCursor(0, 1);
    lcd.print("TEMP TROPPO BASSA");
    errore_temp_min = true;
    if (disattiva_allarme_temp == false)tone(pin_tone, 1000, 500);
  }
  else {
    errore_temp_min = false;
    disattiva_allarme_temp = false;
  }



  if (manuale == true && Temperatura > temp_max && flag == false)
  {
    lcd.clear();
    if (errore_temp_max == false)lcd.clear();
    lcd.print("ERRORE");
    lcd.setCursor(0, 1);
    lcd.print("TEMP TROPPO ALTA");
    errore_temp_max = true;
    if (disattiva_allarme_temp == false)tone(pin_tone, 1000, 500);
  }
  else {
    errore_temp_max = false;
    disattiva_allarme_temp = false;
  }


  //CONTROLLO UMIDITÀ
  if (manuale == true && Humidity > hum_max && flag == false)
  {
    lcd.clear();
    if (errore_hum_max == false)lcd.clear();
    lcd.print("ERRORE");
    lcd.setCursor(0, 1);
    lcd.print("HUM TROPPO ALTA");
    errore_hum_max = true;
    if (disattiva_allarme_hum == false)tone(pin_tone, 1000, 500);
  }
  else {
    errore_hum_max = false;
    disattiva_allarme_hum = false;
  }

  //CONTROLLO ACQUA
  if (manuale == true && digitalRead(pin_water) == LOW && flag == false)
  {
    lcd.clear();
    if (errore_water == false)lcd.clear();
    lcd.print("ERRORE");
    lcd.setCursor(0, 1);
    lcd.print("MANCA ACQUA");
    errore_water = true;
    if (disattiva_allarme_water == false)tone(pin_tone, 1000, 500);
  }
  else {
    errore_water = false;
    disattiva_allarme_water = false;
  }


//ACCENSIONE/SPENGIMENTO POMPA DELL'ACQUA
        if (Humidity < hum_min && flag == false && dummy_acqua == false && digitalRead(pin_water) == HIGH)
        {

          if (counter % 100 == 0 || timer < 50)
          {

            if (flag_timer == true)timer = 0;
            flag_timer = false;
            timer = timer + 1;
            digitalWrite(rele_water, LOW);
          }
          else{
            digitalWrite(rele_water, HIGH);
            flag_timer = true;
          }
        }
      else if(dummy_acqua==false)(digitalWrite(rele_water,HIGH));
 
  //ACCENSIONE/SPENGIMENTO ELEMENTO RISCALDANTE
  if (digitalRead(pin_temp_switch) == HIGH && manuale == true) {
    digitalWrite(pin_temp_general, HIGH); //LED
    digitalWrite(rele_temp, LOW);
    //flag_rele = true;
  }
  else if (digitalRead(pin_temp_switch) == LOW && manuale == true) {
    digitalWrite(pin_temp_general, LOW); //LED
    digitalWrite(rele_temp, HIGH);
    //flag_rele = false;
  }
  else digitalWrite(pin_temp_general, LOW);



  //LEDS
  /*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


  if (Temperatura >= temp_min && Temperatura <= temp_max)digitalWrite(pin_temp_ok, HIGH);
  else digitalWrite(pin_temp_ok, LOW);

  if (Temperatura < temp_min)digitalWrite(pin_temp_low, HIGH);
  else digitalWrite(pin_temp_low, LOW);

  if (Temperatura > temp_max)digitalWrite(pin_temp_high, HIGH);
  else digitalWrite(pin_temp_high, LOW);

  if (Humidity >= hum_min && Humidity <= hum_max)digitalWrite(pin_hum_ok, HIGH);
  else digitalWrite(pin_hum_ok, LOW);

  if (Humidity < hum_min)digitalWrite(pin_hum_low, HIGH);
  else digitalWrite(pin_hum_low, LOW);

  if (Humidity > hum_max)digitalWrite(pin_hum_high, HIGH);
  else digitalWrite(pin_hum_high, LOW);

  //PIN ACQUA
  if (digitalRead(pin_water) == HIGH)
  {
    digitalWrite(pin_water_ok, HIGH);
    digitalWrite(pin_water_low, LOW);
  }

  else {
    digitalWrite(pin_water_ok, LOW);
    digitalWrite(pin_water_low, HIGH);
  }


  //LED MAN/AUTO
  if (digitalRead(pin_manauto_switch) == HIGH) {
    digitalWrite(pin_man, HIGH);
    digitalWrite(pin_auto, LOW);
  }
  else {
    digitalWrite(pin_man, LOW);
    digitalWrite(pin_auto, HIGH);
  }


  //PARSER JSON PER NODE RED
  Serial.print("{\"T\":");
    Serial.print(Temperatura);
    Serial.print(",\"H\":");
    Serial.print(Humidity);
    Serial.print(",\"tm\":");
    Serial.print(temp_min);
    Serial.print(",\"tM\":");
    Serial.print(temp_max);
    Serial.print(",\"hm\":");
    Serial.print(hum_min);
    Serial.print(",\"hM\":");
    Serial.print(hum_max);
    Serial.print("}");
  

  counter = counter + 1;

  digitalWrite(4, HIGH); //NON CANCELLARE
}

//FUNZIONE INTERRUPT TASTINO
void cambiaStato()
{
  stato = !stato;
  digitalWrite(prova, stato);
  Serial.print("{\"ERR\"}\n");
  disattiva_allarme_hum = true;
  disattiva_allarme_temp = true;
  disattiva_allarme_water = true;
}


//FUNZIONE SWITCH VALORI MIN E MAX
void swap_minmax(float &a,float &b)
{
float c;
if (a>b)
{
 c=a;
 a=b;
 b=c;
}
}
