#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <MsTimer2.h>
#include <EEPROM.h>

#define joy_x A0
#define joy_y A1
#define joy_c 2
#define p_fan_i 3
#define p_fan_o 4
#define p_printer 5 //pin_printer

/*-------Due to pulled-uped relay------*/ 
/*------------HIGH = 1 = off-----------*/ 
#define on 0
#define off 1
#define eco 2
int p_led[2]={6,7}, s_led = 0; // s_led=0 -> on / s_led=1 -> off / s_led=2 -> eco

/*------------Setting array order-----------*/ 
int setting[4];
#define vent_reference 0  // if setting[vent_reference]=0 -> by Time / 1 -> by Dust level 
#define time_critic 1
#define pm10_critic 2
#define pm25_critic 3

#define delayt 200  //delay time when setting, power, ect change

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int c_1=0, c_2=0, c_3=0, depth=0, c_3_c=0, s_printer, s_fan_i, s_fan_o, pm10, pm25, temp; 
unsigned long pasttime = 0;//for wait_printer
unsigned long p_time = 0;//for sleep mode
int wait_printer = 0; // = 0 -> nothing / = 1 -> Time / 2-> Dust
int wait_fan = 0; // = 0 ->nothing / =1 -> Dust level
void load(){

  int i=0;
  u8g2.clearBuffer();
  for(i=15;i>-1;i--){
    u8g2.drawBox(0,i*4,128,8);
    if(i<7){
      u8g2.setDrawColor(0);
      u8g2.drawStr(3,35,"MANAGER LOADING...");
      u8g2.setDrawColor(1); 
    }   
    u8g2.sendBuffer();
    delay(10);
  }
  delay(1000);
  u8g2.setDrawColor(0);
  for(i=15;i>-1;i--){
    u8g2.drawBox(0,i*4,128,8);
    u8g2.sendBuffer();
    delay(10);
  }
  u8g2.setDrawColor(1);
  u8g2.clearBuffer();
  delay(200);
}
void s_page(){
  u8g2.clearBuffer();					
  u8g2.setFont(u8g2_font_ncenB08_tr);	
  u8g2.drawButtonUTF8(64, 10, U8G2_BTN_INV|U8G2_BTN_BW2|U8G2_BTN_HCENTER, 0,  0,  0, "3D PRINT MANAGER" );
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 12, 128, 2);
  u8g2.setDrawColor(1);
  
  u8g2.drawStr(10,26,"TEMP");
  u8g2.drawStr(51,25,":");
  //u8g2.setFont(u8g2_font_unifont_t_symbols);
  	
  u8g2.drawStr(10,43,"PM 2.5");
  u8g2.drawStr(51,42,":");
  	

  u8g2.drawStr(10,60,"PM 1.0");
  u8g2.drawStr(51,59,":");

  u8g2.setFont(u8g2_font_8x13_t_symbols);
  u8g2.drawUTF8(87, 29, "℃");	
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawUTF8(87, 43, "ug/m3");
  u8g2.drawUTF8(87, 60, "ug/m3");	

  u8g2.drawFrame(101,21,11,6);
  if(wait_fan == 0){
    u8g2.drawBox(102,22,4,4);
  }
  else{
    u8g2.drawBox(107,22,4,4);
  }
  u8g2.sendBuffer();		
}
void check_joy(){
  int y;
  y=analogRead(joy_y);
  
  if(y > 900){
    if(depth == 1){
        if(c_1 == 4){
          c_1 -= 4;
        }    
        else{
          c_1 += 1;
        }
    }
    else if(depth == 2){
      c_2 += 1;
    }
    else if(depth >= 3){
      c_3 -= 1;
    }
    delay(170);
    p_time = millis();
  }
  else if(y < 200){
    if(depth == 1){
      if(c_1 == 0){
        c_1 += 4;
      }
      else{
        c_1 -= 1;
      }
    }
    else if(depth == 2){
      c_2 -= 1;
    }
    else if(depth >= 3){
      c_3 += 1;
    }
    delay(170);
    p_time = millis();
  }
  ///*
  if((millis() - p_time > 10000)){
    depth = 0;
    c_1 = 0;
    c_2 = 0;
    c_3 = 0;
    c_3_c = 0;
  }
  //*/
  /*
    Serial.print("c_1: ");
    Serial.println(c_1);
    Serial.print("c_2: ");
    Serial.println(c_2);
    Serial.print("c_3: ");
    Serial.println(c_3);
  */
}
void d_check(){       //fine dust & temperature sensor check
  //temp=analogRead;
  pm25=analogRead(A3);
  pm10=pm25;
}
void setup(){
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  pinMode(joy_x, INPUT);
  pinMode(joy_y, INPUT);
  pinMode(joy_c, INPUT_PULLUP); 

  pinMode(p_printer,OUTPUT);
  digitalWrite(p_printer, off);
  s_printer = off;

  pinMode(p_fan_i,OUTPUT);
  digitalWrite(p_fan_i, off);
  s_fan_i = off;

  pinMode(p_fan_o,OUTPUT);
  digitalWrite(p_fan_o, off);
  s_fan_o = off;

  pinMode(p_led[0],OUTPUT);
  digitalWrite(p_led[0], off);
  pinMode(p_led[1],OUTPUT);
  digitalWrite(p_led[1], off);
  s_led = off;
  
  MsTimer2::set(3000,d_check);
  MsTimer2::start();

  //Serial.begin(9600);
  setting[0] = EEPROM.read(0);
  setting[1] = EEPROM.read(1);
  setting[2] = EEPROM.read(2);
  setting[3] = EEPROM.read(3);
  //load();
}

void loop() {

  int i, x, y, j; 
  //j=analogRead(A2);
  if(digitalRead(joy_c) == 0 /*|| j>700*/){
    depth+=1;
    //Serial.println(j);
    //Serial.print("depth: ");
    //Serial.println(depth);
    p_time = millis();
    delay(200);
  }
  

  if(depth == 0){
    s_page();
  }
  else if(depth != 0){
    check_joy();
    u8g2.clearBuffer();	
    if (depth == 1){
      u8g2.setFont(u8g2_font_ncenB08_tr);	
      u8g2.drawButtonUTF8(64, 10, U8G2_BTN_INV|U8G2_BTN_BW2|U8G2_BTN_HCENTER, 0,  0,  0, "MENU" );
      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 12, 128, 2);
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_6x12_t_symbols );
      
      u8g2.drawFrame(6,(c_1+1)*10 + 3,116,11);
    
      u8g2.drawStr(10, 22,"Printer On/Off" );
      u8g2.drawStr(10, 32,"Fan Control" );
      u8g2.drawStr(10, 42,"LED Control" );
      u8g2.drawStr(10, 52,"Setting" );
      u8g2.drawStr(10, 62,"Exit" );
    }
    else if(depth == 2){
      if(c_1 == 0){
        u8g2.setFont(u8g2_font_ncenB08_tr);	
        u8g2.drawButtonUTF8(64, 10,U8G2_BTN_BW0|U8G2_BTN_HCENTER, 0,  0,  0, "< Printer On/Off >" );
        u8g2.setFont(u8g2_font_6x12_t_symbols );
        
        if(c_2 >= 3){
          c_2 -= 3;
        }
        else if(c_2 <= -1){
          c_2 += 3;
        }
        u8g2.drawFrame(6,(c_2+1)*10 + 3,116,11);
        u8g2.drawStr(10, 22,"On/Off" );
        u8g2.drawStr(10, 32,"Off with Vent" );     
        u8g2.drawStr(10, 42,"EXIT" );       
      }
      else if(c_1 == 1){
        u8g2.setFont(u8g2_font_ncenB08_tr);	
        u8g2.drawButtonUTF8(64, 10,U8G2_BTN_BW0|U8G2_BTN_HCENTER, 0,  0,  0, "< Fan Control >" );
        u8g2.setFont(u8g2_font_6x12_t_symbols );
        
        if(c_2 >= 4){
          c_2 -= 4;
        }
        else if(c_2 <= -1){
          c_2 += 4;
        }
        u8g2.drawFrame(6,(c_2+1)*10 + 3,116,11);
        u8g2.drawStr(10, 22,"In Fan On/Off" );
        u8g2.drawStr(10, 32,"Out Fan On/Off" );     
        u8g2.drawStr(10, 42,"Fan If Dust" );   
        u8g2.drawStr(10, 52,"EXIT" );             
      }
      else if(c_1 == 2){
        u8g2.setFont(u8g2_font_ncenB08_tr);	
        u8g2.drawButtonUTF8(64, 10,U8G2_BTN_BW0|U8G2_BTN_HCENTER, 0,  0,  0, "< LED Control >" );
        u8g2.setFont(u8g2_font_6x12_t_symbols );
        
        if(c_2 >= 3){
          c_2 -= 3;
        }
        else if(c_2 <= -1){
          c_2 += 3;
        }
        u8g2.drawFrame(6,(c_2+1)*10 + 3,116,11);
        u8g2.drawStr(10, 22,"On/Off" );
        u8g2.drawStr(10, 32,"Eco" );      
        u8g2.drawStr(10, 42,"EXIT" );             
      }
      else if(c_1 == 3){
        u8g2.setFont(u8g2_font_ncenB08_tr);	
        u8g2.drawButtonUTF8(64, 10,U8G2_BTN_BW0|U8G2_BTN_HCENTER, 0,  0,  0, "< Setting >" );
        u8g2.setFont(u8g2_font_6x12_t_symbols );
        
        if(c_2 >= 5){
          c_2 -= 5;
        }
        else if(c_2 <= -1){
          c_2 += 5;
        }
        u8g2.drawFrame(6,(c_2+1)*10 + 3,116,11);
        u8g2.drawStr(10, 22,"Vent mode" );
        u8g2.drawStr(10, 32,"Time critic" ); 
        u8g2.drawStr(10, 42,"PM 2.5 critic" );     
        u8g2.drawStr(10, 52,"PM 1.0 critic" );   
        u8g2.drawStr(10, 62,"EXIT" );         
      }
      else if(c_1 == 4){
          c_1=c_2=0;
          depth=0;
        }
    }
    else if(depth >= 3){ //depth >= 3
      if(c_1 == 0){ //printer on/off
        if(c_2 == 0){       //on/off
          s_printer = !s_printer;
          delay(150);
          digitalWrite(p_printer,s_printer);
          delay(delayt);
          depth = 2;
        }
        else if(c_2 == 1 ){  //off with vent 
          if(s_printer == on){
            digitalWrite(p_printer,off);
            s_printer = off;
            digitalWrite(p_fan_i,on);
            s_fan_i = on;
            
            if(setting[vent_reference] == 0){ //vent by time
              pasttime = millis();
              wait_printer = 1;
            }
            else if(setting[vent_reference == 1]){  //vent by dustlevel
              wait_printer = 2;
            }
          }
          depth=2;
        }
        else{
          c_1=0;
          c_2=0;
          depth=1;
        }
      }
      else if(c_1 == 1){  //fan control
        if(c_2 == 0){ //in fan
          s_fan_i = !s_fan_i;
          delay(150);
          digitalWrite(p_fan_i,s_fan_i);
          delay(delayt);
          depth = 2;
        }
        else if(c_2 == 1){  //out fan
          s_fan_o = !s_fan_o;
          delay(150);
          digitalWrite(p_fan_o,s_fan_o);
          delay(delayt);
          depth = 2;
        }
        else if(c_2 == 2){  //fan by dust 
          wait_fan = !wait_fan;
          depth=2;
        }
        else {
          c_1=1;
          c_2=0;
          depth=1;
        }
      }
      else if(c_1 == 2){  //Led control
        if(c_2 == 0){ //led on/off
          s_led = !s_led;
          delay(150);
          digitalWrite(p_led[0],s_led);
          digitalWrite(p_led[1],s_led);
          delay(delayt);
          depth = 2;
        }
        else if(c_2 == 1){  //eco mode on
          digitalWrite(p_led[0], off);
          digitalWrite(p_led[1], on);
          s_led = eco;
          depth=2;
        }
        else{
          c_1=2;
          c_2=0;
          depth=1;
        }
      }
      else if(c_1 == 3){  //setting
        if(c_2 == 0){ //vent mode
          if(depth == 3){
            if(c_3_c == 0){
              c_3 = EEPROM.read(vent_reference);
              c_3_c = 1;
            }
            if(c_3 >= 2){
              c_3 -= 2;
            }
            else if(c_3 <= -1){
              c_3 += 2;
            }
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x12_t_symbols );
            u8g2.drawFrame(6,(c_3+1)*20 -2,116,11);
            u8g2.drawStr(10, 27,"Time" );
            u8g2.drawStr(10, 47,"Dust Level" );  
          }      
          else if(depth >= 4){
            if(c_3 == 0){
              EEPROM.update(0,0);
              setting[0] = 0;
              delay(delayt);
            }
            else if (c_3 == 1){
              EEPROM.update(0,1);
              setting[0] = 1;
              delay(delayt);
            }
            c_3 = 0;
            depth = 2;
            c_3_c = 0;
          }
        }
        else if(c_2 == 1){  //time critic
          if(depth == 3){
            if(c_3_c == 0){
              c_3 = EEPROM.read(1);
              c_3_c = 1;
            }
            u8g2.setFont(u8g2_font_courB24_tr );
            if(c_3 >= 100 || c_3 <= 0){
              c_3 = 1;
            }
            if(c_3<10){
              u8g2.setCursor(35,45);
              u8g2.print(c_3);
            }  
            else if(c_3<100){
              u8g2.setCursor(15,45);
              u8g2.print(c_3);
            }
            u8g2.drawFrame(15,50,41,2);
            u8g2.drawStr(65,45,"min");    
          }
          if(depth >= 4){
            u8g2.setFont(u8g2_font_6x12_t_symbols );
            setting[1] = c_3;
            EEPROM.update(1,c_3);
            c_3 = 0;
            c_3_c = 0;
            depth = 2;
          }
        }
        else if(c_2 == 2){  //pm2.5critic
          if(depth == 3){
            if(c_3_c == 0){
              c_3 = EEPROM.read(2);
              c_3_c = 1;
            }
            u8g2.setFont(u8g2_font_courB24_tr );
            if(c_3 >= 100 || c_3 <= 0){
              c_3 = 1;
            }
            u8g2.setFont(u8g2_font_courB24_tr );
            if(c_3<10){
              u8g2.setCursor(40,45);
              u8g2.print(c_3);
            }  
            else if(c_3<100){
              u8g2.setCursor(20,45);
              u8g2.print(c_3);
            }
            u8g2.drawFrame(20,50,41,2);
            u8g2.setFont(u8g2_font_lubI12_tf );
            u8g2.drawStr(65,45,"ug/m3");  
          }
          if(depth >= 4){
            u8g2.setFont(u8g2_font_6x12_t_symbols );
            setting[2] = c_3;
            EEPROM.update(2,c_3);
            c_3 = 0;
            c_3_c = 0;
            depth = 2;
          }
        }
        else if(c_2 == 3){  //pm10critic
          if(depth == 3){
            if(c_3_c == 0){
              c_3 = EEPROM.read(3);
              c_3_c = 1;
            }
            u8g2.setFont(u8g2_font_courB24_tr );
            if(c_3 >= 100 || c_3 <= 0){
              c_3 = 1;
            }
            u8g2.setFont(u8g2_font_courB24_tr );
            if(c_3<10){
              u8g2.setCursor(40,45);
              u8g2.print(c_3);
            }  
            else if(c_3<100){
              u8g2.setCursor(20,45);
              u8g2.print(c_3);
            }
            u8g2.drawFrame(20,50,41,2);
            u8g2.setFont(u8g2_font_lubI12_tf );
            u8g2.drawStr(65,45,"ug/m3");  
          }
          if(depth >= 4){
            u8g2.setFont(u8g2_font_6x12_t_symbols );
            setting[3] = c_3;
            EEPROM.update(3,c_3);
            c_3 = 0;
            c_3_c = 0;
            depth = 2;
          }
        }
        else{
          c_1=3;
          c_2=0;
          depth=1;
        }
      }
    }
    u8g2.sendBuffer();    	
  }
  if(wait_printer == 0){
    (void)0;
  }
  else if(wait_printer == 1){
    if((millis() - pasttime) >= (setting[1]*60000)){
      digitalWrite(p_fan_i,off);
      s_fan_i = off;
      wait_printer = 0;
    }
  }
  else if(wait_printer == 2){
    if(pm25 < setting[2] || pm10 < setting[3]){
     digitalWrite(p_fan_i,off);
      s_fan_i = off;
      wait_printer = 0;
    }
  }
  if(wait_fan == 0){
    (void)0;
  }
  else if(wait_fan == 1){
    if(pm25 < setting[2] || pm10 < setting[3]){
      digitalWrite(p_fan_i,off);
      s_fan_i = off;
    }
    else{
      digitalWrite(p_fan_i,on);
      s_fan_i = on;
    }
  }
  /*
  Serial.print("pm10:");
  Serial.println(pm10);
  Serial.print("c_1:");
  Serial.println(c_1);
  Serial.print("c_2:");
  Serial.println(c_2);
  Serial.print("c_3:");
  Serial.println(c_3);
  Serial.print("depth:");
  Serial.println(depth);
  Serial.print("s_printer:");
  Serial.println(s_printer);
  Serial.print("s_fan_i:");
  Serial.println(s_fan_i);
  Serial.print("s_fan_o:");
  Serial.println(s_fan_o);
  Serial.print("wait_printer:");
  Serial.println(wait_printer);
  Serial.print("wait_fan:");
  Serial.println(wait_fan);
  */
  
}
