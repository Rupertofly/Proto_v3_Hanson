#include <Arduino.h>
#include <Adafruit_ATParser.h>
#include <Adafruit_BLE.h>
#include <Adafruit_BLEBattery.h>
#include <Adafruit_BLEEddystone.h>
#include <Adafruit_BLEGatt.h>
#include <Adafruit_BLEMIDI.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <Adafruit_BluefruitLE_UART.h>
#include "keyboardSetup.h"

void setup();

void loop();

bool b1_press = false;
long b1_start = 0;
bool b2_press = false;
long b2_start = 0;
bool bb_press = false;
long bb_start = 0;

String c_send = "-";


const char ca_map[27] = {'0','a','b','c','d','e','f','g',
                         'h','i','j','k','l','m','n','o',
                         'p','q','r','s','t','u','v','w',
                         'x','y','z'};


int i_ind = 0;




const int pin_flex_1 = A0;
const int pin_flex_2 = A1;
const int pin_flex_3 = A2;

const int pin_button_1 = A3;
const int pin_button_2 = A4;

const int pin_vib_1 = 5;
const int pin_vib_2 = 9;
const int pin_vib_3 = 10;


String s_keyboard_name = "RupertM_Keyboard";

class vibe {
  int pin;
  int strength;
  int reps_left;
  long c_time;
  long s_time;
  long o_time;
  long e_time;
  long f_time;
  bool active;
  bool between;
  int reps;

  public:

  vibe(int _pin){
    pin = _pin;
    pinMode(pin, OUTPUT);
    analogWrite(pin,0);
    s_time = 0L;
    c_time = millis();
    active = false;
    reps = 0;
    between = false;

  }
  bool buzz(long _length, int _strength, int _repatitions, long rep_legth){
    if (active == false){
      s_time = millis();
      c_time = millis();
      o_time = _length;
      strength = _strength;
      e_time = s_time+o_time;
      reps = _repatitions;
      if (_repatitions > 1){
        f_time = rep_legth;
        reps_left = reps;
      }
      analogWrite(pin,strength);
      active = true;
      return true;
    } else {
      return false;
    }
  }
  void update(){
    c_time = millis();
    if (active){
      if (c_time > e_time){
        if (reps_left == 1) reset_buzz();
        if (reps_left > 1){
          s_time = e_time+f_time;
          e_time = s_time+o_time;
          analogWrite(pin,0);
          between = true;
        }
      }
      if (between && c_time > s_time){
        between = false;
        reps_left--;
        analogWrite(pin, strength);
      }
    }
  }
  void reset_buzz(){
    analogWrite(pin, 0);
    active = false;
    s_time = 0L;
    o_time = 0L;
    e_time = 0L;
    f_time = 0L;
    reps = 0;
    reps_left = 0;
    between = false;
  }
  bool get_state(){
    return active;
  }
};

class sensor {
  public:
  int pin;
  int state;
  int thresholds[3];
  int readings[10];
  int readIndex;
  float average;
  int total;


  sensor(int _pin, int _1, int _2, int _3){
    pin = _pin;
    thresholds[0] = _1;
    thresholds[1] = _2;
    thresholds[2] = _3;
    readIndex = 0;
    average = 0;
    total = 0;
    for (int i=0;i<10;i++) readings[i] = 0;

  }
  void update(){
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(pin);
    total = total + readings[readIndex];
    readIndex++;
    if (readIndex >= 10) readIndex = 0;
    average = total/10;
  }
  float get_av(){
    return average;
  }
  int get_exact(){
    return analogRead(pin);
  }
  int get_val(){
    return get_val(false);
  }
  int get_val(bool _exact){
    float v = average;
    int val = 0;
    if (_exact) v = float(analogRead(pin));
    if (v>=thresholds[0]) val = 0;
    if (v<=thresholds[0]&&v>=thresholds[1])val = 1;
    if (v<=thresholds[1]) val = 2;
    return val;
  }
  void set_threshold(int _thresh, int _pos){
    thresholds[_pos] = _thresh;
  }
};

vibe vm[3] = {vibe(pin_vib_1),vibe(pin_vib_2),vibe(pin_vib_3)};

sensor sf[3] = {sensor(pin_flex_3,650,400,40),sensor(pin_flex_2,850,600,50),sensor(pin_flex_1,750,500,50)};

vibe v1(pin_vib_1);
vibe v2(pin_vib_2);
vibe v3(pin_vib_3);

sensor s3(pin_flex_1,750,500,50);
sensor s2(pin_flex_2,850,600,50);
sensor s1(pin_flex_3,650,400,40);

class controller{
  public:
  int f_v[3];
  bool f_vib_1[3]  = {false,false,false};
  bool f_vib_2[3]  = {false,false,false};
  controller(){
    zero();
  }
  void zero(){
    for (int i=0;i<3;i++){
      f_v[i] = 0;
      f_vib_1[i] = false;
      f_vib_2[i] = false;
    }
  }
  void update(){
    vm[0].update();
    vm[1].update();
    vm[2].update();
    sf[0].update();
    sf[1].update();
    sf[2].update();
    f_v[0] = sf[0].get_val();
    f_v[1] = sf[1].get_val();
    f_v[2] = sf[2].get_val();
    switch (f_v[0]){
      case 1:
        if (!f_vib_1[0])
        if (vm[0].buzz(100, 128, 2, 50)) f_vib_1[0] = true;
        break;
      case 2:
        if (!f_vib_2[0])
        if (vm[0].buzz(150, 255, 2, 50)) f_vib_2[0] = true;
        break;
      default:
      f_vib_1[0] = false;
      f_vib_2[0] = false;
      break;
    }
    switch (f_v[1]){
      case 1:
        if (!f_vib_1[1])
        if (vm[1].buzz(100, 128, 2, 50)) f_vib_1[1] = true;
        break;
      case 2:
        if (!f_vib_2[2])
        if (vm[1].buzz(150, 255, 2, 50)) f_vib_2[1] = true;
        break;
      default:
      f_vib_1[1] = false;
      f_vib_2[1] = false;
      break;
    }
    switch (f_v[2]){
      case 1:
        if (!f_vib_1[2])
        if (vm[2].buzz(100, 128, 2, 50)) f_vib_1[2] = true;
        break;
      case 2:
        if (!f_vib_2[2])
        if (vm[2].buzz(150, 255, 2, 50)) f_vib_2[2] = true;
        break;
      default:
      f_vib_1[2] = false;
      f_vib_2[2] = false;
      break;
    }
  }
  int get_code(){
    return (f_v[0]+(f_v[1]*3)+(f_v[2]*9));
  }
  String debug(){
    String dbg;
    String data[3];
    for (int i=0;i<3;i++){
      data[i] = "sensor" + String(i) + "[";
      data[i] += "av_val: " + String(sf[i].get_av()) + " ";
      data[i] += "ex_val: " + String(sf[i].get_exact()) + " ";
      data[i] += "code_val: " + String(sf[i].get_val()) + " ";
      data[i] += "1st_thr: " + String(sf[i].thresholds[0]) + " ";
      data[i] += "2nd_thr: " + String(sf[i].thresholds[1]) + " ";
      data[i] += "] ";
    }
    dbg = data[0] + data[1] + data[2];
    return dbg;
  }
};

controller control;


void set_values(){
  if (digitalRead(pin_button_1) == 0 && digitalRead(pin_button_2) == 1){
    if (!b1_press){
      b1_start = millis();
      b1_press = true;
    } else {
      if (millis() - b1_start > 3000){
        b1_press = false;
        vm[0].buzz(300, 255, 3, 50);
        vm[1].buzz(300, 255, 3, 50);
        vm[2].buzz(300, 255, 3, 50);
        sf[0].set_threshold((int)sf[0].get_av(),0);
        sf[1].set_threshold((int)sf[1].get_av(),0);
        sf[2].set_threshold((int)sf[2].get_av(),0);
      }
    }
  } else b1_press = false;
  if (digitalRead(pin_button_2) == 0 && digitalRead(pin_button_1) == 1){
    if (!b2_press){
      b2_start = millis();
      b2_press = true;
    } else {
      if (millis() - b2_start > 3000){
        b2_press = false;
        vm[0].buzz(300, 255, 3, 50);
        vm[1].buzz(300, 255, 3, 50);
        vm[2].buzz(300, 255, 3, 50);
        sf[0].set_threshold((int)sf[0].get_av(),1);
        sf[1].set_threshold((int)sf[1].get_av(),1);
        sf[2].set_threshold((int)sf[2].get_av(),1);
      }
    }
  } else b2_press = false;
  if (digitalRead(pin_button_1) == 0 && digitalRead(pin_button_2) == 0){
    if (!bb_press){
      bb_start = millis();
      bb_press = true;
    } else {
      if (millis() - bb_start > 3000){
        b1_press = false;
        vm[0].buzz(300, 255, 3, 50);
        vm[1].buzz(300, 255, 3, 50);
        vm[2].buzz(300, 255, 3, 50);
        sf[0].set_threshold((int)sf[0].get_av(),2);
        sf[1].set_threshold((int)sf[1].get_av(),2);
        sf[2].set_threshold((int)sf[2].get_av(),2);
      }
    }
  } else bb_press = false;
}

byte y_loop = 0;
byte y_high = 0;
boolean b_receive = false;

int i_send;


boolean b_send = false;
boolean b_zero = true;


boolean key_check(byte _y_key){ //check whether current value is higher then previous values, and increment loop count
  if (_y_key!=0){
    if (_y_key > y_high){
      y_high = _y_key;
    }
  }
  if(y_loop >= 18){
    return true;
    //y_loop=0;
  } else {
    y_loop++;
    return false;
  }
}






void setup(){
  Serial.begin(9600);
  setupKeyboard(s_keyboard_name);
  pinMode(pin_button_1, INPUT_PULLUP);
  pinMode(pin_button_2, INPUT_PULLUP);
}
void loop(){
  control.update();
  Serial.println(control.debug());
  delay(20);
  set_values();

  byte y_output = control.get_code();
  if (y_output == 0 && b_zero == false){ // if current state is zero, and zero is yet to be received, mark zero received
    b_zero = true;
    y_loop = 0;
  }
  if (y_output != 0 && b_zero) b_receive = true; //if receiving a character other then zero,
  if (y_loop <19&&b_receive){ //if looped less then 5 times and still receiving a characeter other then 0
    if (key_check(y_output)) { //if looped 5 times
      b_send = true; //tell code to send
      i_send = y_high; // tell code what to send
      y_high = 0; //reset values
      b_receive = 0;
    }
  }
  if (b_send){
    Serial.println(i_send);
    b_send = false;
    b_zero = false;
    vm[0].buzz(50, 255, 3, 50);
    if (i_send == 30){
      c_send = "\\b";
    } else {
      c_send = String(ca_map[i_send]);
    }
    ble.print("AT+BleKeyboard="); //write character to bluetooth
    ble.println(c_send);
    //ble.info();
    //ble.sendCommandCheckOK("AT+BleHIDEn=On");
    //ble.sendCommandCheckOK("AT+BleKeyboardEn=On");
    if ( ble.waitForOK() )
    {
      Serial.println("OK!");
    } else
    {
      Serial.println("FAILED!");
    }
  }


}




















/*
const int i_pin_vib = 5;
const int i_pin_swit = 6;
const int i_pin_f[3] = {A0,A1,A2};



//Send Character
String c_send = "-";


const char ca_map[27] = {'0','a','b','c','d','e','f','g',
                         'h','i','j','k','l','m','n','o',
                         'p','q','r','s','t','u','v','w',
                         'x','y','z'};


int i_ind = 0;

int totals[3];

void buzz(int _t, int _s, bool _d, int _dl);
byte get_finger(bool _f);
boolean key_check(byte _y_key);

byte l_s_b[3] = {10,10,10};


String s_keyboard_name = "RupertM_Keyboard";

int threshold_vals[3][3] = {{200,500,800},{450,700,900},{450,700,980}};

float finger_vals[3] = {0.0,0.0,0.0};

byte fin_vals[3] = {0,0,0};

byte y_loop = 0;
byte y_high = 0;

boolean b_send = false;
boolean b_zero = true;
boolean b_receive = false;
boolean haptic_1[3] = {false,false,false};
boolean haptic_2[3] = {false,false,false};

// Send Val
int i_send;


void setup(){
  //Serial.begin(9600);
  delay(1000);
  Serial.begin(115200);

  setupKeyboard(s_keyboard_name);
  pinMode(i_pin_vib,OUTPUT);
  pinMode(i_pin_swit, INPUT_PULLUP);


}
void loop(){
Serial.print(finger_vals[0]);
  Serial.print(" ");
  Serial.print(finger_vals[1]);
  Serial.print(" ");
  Serial.print(finger_vals[2]);
  Serial.print(" ");
  Serial.println();
  */
  /*
  delay(100);
  byte y_output = get_finger(false);
//  Serial.print(y_loop);
//  Serial.print(" ");
//  Serial.println(y_output);
  if (y_output == 0 && b_zero == false){ // if current state is zero, and zero is yet to be received, mark zero received
    b_zero = true;
    y_loop = 0;
    for (int i = 0; i<3;i++){
      haptic_1[i] = false;
      haptic_2[i] = false;
    }
  }
  if (y_output != 0 && b_zero) b_receive = true; //if receiving a character other then zero,
  if (y_loop <8&&b_receive){ //if looped less then 5 times and still receiving a characeter other then 0
    if (key_check(y_output)) { //if looped 5 times
      b_send = true; //tell code to send
      i_send = y_high; // tell code what to send
      y_high = 0; //reset values
      b_receive = 0;
    }
  }
  if (get_finger(true) == 30){
    b_send = true;
    i_send = 30;
  }
  if (b_send){
    Serial.println(i_send);
    b_send = false;
    b_zero = false;
    buzz(30, 255, true, 30);
    if (i_send == 30){
      c_send = "\\b";
    } else {
      c_send = String(ca_map[i_send]);
    }
    ble.print("AT+BleKeyboard="); //write character to bluetooth
    ble.println(c_send);
    //ble.info();
    //ble.sendCommandCheckOK("AT+BleHIDEn=On");
    //ble.sendCommandCheckOK("AT+BleKeyboardEn=On");
    if ( ble.waitForOK() )
    {
      Serial.println("OK!");
    } else
    {
      Serial.println("FAILED!");
    }

    if (i_send == 30) delay(500);

  }

}

void buzz(int _t, int _s, bool _d, int _dl){
  switch (_d){
    case false:
    analogWrite(i_pin_vib, _s);
    delay(_t);
    analogWrite(i_pin_vib,0);
    break;
    case true:
    analogWrite(i_pin_vib, _s);
    delay(_t);
    analogWrite(i_pin_vib,0);
    delay(_dl);
    analogWrite(i_pin_vib, _s);
    delay(_t);
    analogWrite(i_pin_vib,0);
    break;

  }

}
byte get_finger(bool _f){
  bool buzz_loop_1 = false;
  bool buzz_loop_2 = false;
  for (int i = 0; i<3;i++){
    if (l_s_b[i] < 10) l_s_b[i]++;

    finger_vals[i] = 0.5*finger_vals[i]+ (0.5*(analogRead(i_pin_f[i])));

    if (finger_vals[i] < threshold_vals[i][2] && finger_vals[i] > threshold_vals[i][1]){
      if (haptic_1[i] == false){
        buzz_loop_1 = true;
        haptic_1[i] = true;
      }
      fin_vals[i] = 1;
      l_s_b[i] = 0;
    } else if (finger_vals[i] < threshold_vals[i][1]){
      fin_vals[i] = 2;
      l_s_b[i] = 0;
      if (haptic_2[i] == false){
         buzz_loop_2 = true;
         haptic_2[i] = true;
       }
    } else {
      fin_vals[i] = 0;
    }
    if (buzz_loop_1) buzz(30, 180, false, 10);
    if (buzz_loop_2) buzz(15, 180, true, 10);
  }
  int val = fin_vals[0]+(fin_vals[1]*3)+(fin_vals[2]*9);
  if (_f){
    if (finger_vals[0] < threshold_vals[0][0]&&finger_vals[1] < threshold_vals[1][0]&&finger_vals[2] < threshold_vals[2][0]){
      val = 30;
    }
  }
  return val;

  delay(50);
}

boolean key_check(byte _y_key){ //check whether current value is higher then previous values, and increment loop count
  if (_y_key!=0){
    if (_y_key > y_high){
      y_high = _y_key;
    }
  }
  if(y_loop >= 7){
    return true;
    //y_loop=0;
  } else {
    y_loop++;
    return false;
  }
}
*/
