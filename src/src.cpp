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
bool b_send = false;
int i_send;


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
  bool b_but_1;
  bool b_but_2;
  bool b_up;
  controller(){
    zero();
  }
  void zero(){
    for (int i=0;i<3;i++){
      f_v[i] = 0;
      f_vib_1[i] = false;
      f_vib_2[i] = false;
      b_but_1 = false;
      b_but_2 = false;
      b_up = true;
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
    if (digitalRead(pin_button_1) == 0){
      delay(15);
      if (digitalRead(pin_button_1) == 0) b_but_1 = true;
      else b_but_1 = false;
    } else b_but_1 = false;
    if (digitalRead(pin_button_2) == 0){
      delay(15);
      if (digitalRead(pin_button_2) == 0) b_but_2 = true;
      else b_but_2 = false;
    } else b_but_2 = false;
  }
  bool b_pressed(int& _b){
    if (b_up){
      if ((b_but_1||b_but_2)){
        if (b_but_1) _b = 1;
        if (b_but_2) _b = 2;
        b_up = false;
        return true;
      } else {
        return false;
      }
    } else {
      if (b_but_1 == false&&b_but_2==false) b_up = true;
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

void chk_button(){
  int but = 0;
  if (control.b_pressed(but)){
    switch (but) {
      case 1:
      b_send = true;
      i_send = 30;
      break;
      case 2:
      b_send = true;
      i_send = 40;
    }
  }
}

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
        sf[0].set_threshold((int)sf[0].get_av()-15,0);
        sf[1].set_threshold((int)sf[1].get_av()-15,0);
        sf[2].set_threshold((int)sf[2].get_av()-15,0);
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
        sf[0].set_threshold((int)sf[0].get_av()-10,1);
        sf[1].set_threshold((int)sf[1].get_av()-10,1);
        sf[2].set_threshold((int)sf[2].get_av()-10,1);
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
  chk_button();

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
    if (i_send == 40) c_send = " ";
    else if (i_send == 30){
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
