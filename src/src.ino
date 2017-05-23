# include <Arduino.h>
#include <Adafruit_ATParser.h>
#include <Adafruit_BLE.h>
#include <Adafruit_BLEBattery.h>
#include <Adafruit_BLEEddystone.h>
#include <Adafruit_BLEGatt.h>
#include <Adafruit_BLEMIDI.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <Adafruit_BluefruitLE_UART.h>
#include "keyboardSetup.h"




const int i_pin_vib = 5;
const int i_pin_swit = 6;
const int i_pin_f[3] = {A0,A1,A2};



//Send Character
String c_send = "-";


const char ca_map[27] = {'0','a','b','c','d','e','f','g',
                         'h','i','j','k','l','m','n','o',
                         'qp','q','r','s','t','u','v','w',
                         'x','y','z'};


int i_ind = 0;

int totals[3];

void buzz(int _t, int _s, bool _d, int _dl);
byte get_finger(bool _f);
boolean key_check(byte _y_key);

byte l_s_b[3] = {10,10,10};


String s_keyboard_name = "RupertM_Keyboard";

int threshold_vals[3][3] = {{200,500,800},{450,700,900},{450,700,900}};

float finger_vals[3] = {0.0,0.0,0.0};

byte fin_vals[3] = {0,0,0};

byte y_loop = 0;
byte y_high = 0;

boolean b_send = false;
boolean b_zero = true;
boolean b_receive = false;
boolean haptic[3] = {false,false,false};

// Send Val
int i_send;


void setup(){
  setupKeyboard(s_keyboard_name);
  pinMode(i_pin_vib,OUTPUT);
  pinMode(i_pin_swit, INPUT_PULLUP);
  Serial.begin(9600);

}
void loop(){
  delay(100);
  byte y_output = get_finger(false);
//  Serial.print(y_loop);
//  Serial.print(" ");
//  Serial.println(y_output);
  if (y_output == 0 && b_zero == false){ // if current state is zero, and zero is yet to be received, mark zero received
    b_zero = true;
    y_loop = 0;
    for (int i = 0; i<3;i++) haptic[i] = false;
  }
  if (y_output != 0 && b_zero) b_receive = true; //if receiving a character other then zero,
  if (y_loop <12&&b_receive){ //if looped less then 5 times and still receiving a characeter other then 0
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
  bool buzz_loop = false;
  for (int i = 0; i<3;i++){
    if (l_s_b[i] < 10) l_s_b[i]++;

    finger_vals[i] = 0.5*finger_vals[i]+ (0.5*(analogRead(i_pin_f[i])));

    if (finger_vals[i] < threshold_vals[i][2] && finger_vals[i] > threshold_vals[i][1]){
      fin_vals[i] = 1;
      l_s_b[i] = 0;
    } else if (finger_vals[i] < threshold_vals[i][1]){
      fin_vals[i] = 2;
      l_s_b[i] = 0;
      if (haptic[i] == false){
         buzz_loop = true;
         haptic[i] = true;
       }
    } else {
      fin_vals[i] = 0;
    }
    if (buzz_loop) buzz(15, 255, false, 0);
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
  if(y_loop >= 11){
    return true;
    //y_loop=0;
  } else {
    y_loop++;
    return false;
  }
}
