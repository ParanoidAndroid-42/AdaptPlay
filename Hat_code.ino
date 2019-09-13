// 4/9/2019 - Nathan Fuentes - version 1.0

/* LCD Arduino
    1 --> 5v
    2 --> GND
    3 --> 7
    4 --> 6
    5 --> 5
    6 --> 4
    7 --> 8
    8 --> 5v / pin 9  
*/

#include <Wire.h> // default
#include <Mouse.h> // default
#include <EEPROM.h> // default
#include <PCD8544.h> // https://www.arduinolibraries.info/libraries/pcd8544
#include <Keyboard.h> // default
#include <MPU6050_tockn.h> // https://github.com/tockn/MPU6050_tockn


//Settings:
//-------------------------------------------------//
bool mouse_acceleration = false; // turn on or off mouse acceleration
bool mouse_mode = true; // IMU moves the mouse
bool mouse_mode_z = false; // IMU moves mouse. used Z axis for lef/right //add acceleration
bool WASD_mode = false; // IMU activates W,A,S,D keys 
int default_speed = 1; // speed the mouse moves when acceleration is off
int velocity = 1; // mouse speed when acceleration is on (minimum is 1)
int mouse_speed_delay = 10; // slown the mouse down with a delay when acceleration is on
int mouse_acc_delay = 300; // delay before mouse accelerates
int max_speed = 5; // speed the mouse accelerates too

int threshold = 17; // IMU left/right threshold      
int forward_threshold = 30; // IMU forward threshold
int back_threshold = 5; // IMU back threshold
//-------------------------------------------------//
int i=10;

bool is_moving_left = false;
bool is_moving_right = false;
bool is_moving_forward = false;
bool is_moving_back = false;
bool is_moving_z_right = false;
bool is_moving_z_left = false;


int acc_timer = mouse_acc_delay; // time before mouse accelerates
unsigned long target_time = 0; //Overflow 49.71 days. possibly not bad.
bool w_pressed, a_pressed, s_pressed, d_pressed;

unsigned long activity_timer; // currently not used //Overflow

static PCD8544 lcd; // not really sure

int up = 16; // menu up button
int select = 14; // menu select button
int down = 10; // menu down button

bool up_state; // menu up state
bool select_state; // menu select state
bool down_state; // menu down state

int page = 0; // page number in settings menu
int item = 0; // menu item number

unsigned long timer; // time before button presses register in settings menu //Overflow

int devices; // number of devices detected by detect_imu
byte error, adress; // used in detect_imu
byte val;
bool has_run = false;
bool calibration_override = false; // used to override calibration if IMU isn't detected
MPU6050 mpu6050(Wire);
void setup() {
  Serial.begin(9600); // start Serial for debuggin
  Wire.begin(); // Init IIC bus
  // Setup inputs //
  pinMode(up, INPUT_PULLUP);
  pinMode(select, INPUT_PULLUP);
  pinMode(down, INPUT_PULLUP);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  check_saved(); // Load saved values from EEPROM
  lcd.begin(84, 48); // Init LCD
  lcd.setContrast(60); // set LCD contrast
  detect_imu(); // Check if IMU is connected
  lcd.setCursor(0,2); // set cursor to line 2
  lcd.print("Calibrating..."); // print to lcd
  Serial.begin(9600);
  //while(!Serial); // Stops the program untill the Serial monitor is started. Uncomment for debugging
  Mouse.begin(); // Init mouse
  Keyboard.begin(); // Init keyboard
  mpu6050.begin(); // Init IMU
  if(calibration_override==false){ // Callibration override used to bypass error when IMU is not connected durring startup.
    mpu6050.calcGyroOffsets(true); // Run IMU callibration
  }
  lcd.clear(); // clear display
  display_menu(); // Run the display_menu function once to draw the settings menu
  timer = millis() + (200); // This prob isn't needed but...
}
void loop() {
  mpu6050.update(); // Get values from IMU
  if(mouse_mode==true && mouse_acceleration==true){ // If the acceleration setting is on
    if(millis() >= target_time){ // only run if the current millis() time has past the target_time. this is used to slow  the mouse down below a valocity value of 1 (1 is the minium value for velocity)
      mouse_move(velocity);  // start moving the mouse
      if(mouse_is_moving()){ // if the mouse is moving 
        if(acc_timer <= 0)velocity = max_speed; // if the acc_timer has reached zero, set the mouse velocity to max_speed
        acc_timer--; // subtract one from the acc_timer
      }
      else{ // once mouse stops moving 
        velocity = 1; // set velocity to one
        acc_timer = mouse_acc_delay; // reset the acc_timer to the mouse_acc_delay value
      }
      target_time = millis() + mouse_speed_delay; // set target_time to the current millis() value plus the amount of delay before the function runs again.
    }
  }
  if(mouse_mode==true && mouse_acceleration==false){ // if acceleration mode is off, move mouse at fixed speed
    mouse_move(default_speed); // move the mouse at default_speed. default speed is used because "velocity" is dedicated to the accelecration function
  }
  if(WASD_mode==true){ // if WASD mode is on run the WASD function
    WASD();
  }
  if(mouse_mode_z == true){ // Note: add acceleration.. smooth ramp?
    mouse_move_z(default_speed);  
  }

//  if(user_activity()){
//    activity_timer = millis() + 50;
//  }
  if(user_activity()){ // only run if user activity is detected. if the display is updated every loop it slows everything down WAY to much and is simply unnecessary
    display_menu(); // update the menu display
  }  
}
//----------------Mouse and Keyboard---------------------------------------------//
void mouse_move(int cursor_speed){ // Moves the mouse
  if(mpu6050.getAngleX() > threshold && !is_moving_left && !is_moving_forward && !is_moving_back){ // get IMU value, make sure mouse isn't moving, if not, move it
    Mouse.move(cursor_speed,0,0);
    is_moving_right = true;
  }
  else is_moving_right = false;
  if(mpu6050.getAngleX() < -threshold && !is_moving_right && !is_moving_forward && !is_moving_back){ // do the same for all axis
    Mouse.move(-cursor_speed,0,0);
    is_moving_left = true;
  }
  else is_moving_left = false;
  if(mpu6050.getAngleY() > forward_threshold && !is_moving_left && !is_moving_right && !is_moving_back){
    Mouse.move(0,cursor_speed,0);
    is_moving_forward = true;
  }
  else is_moving_forward = false;
  if(mpu6050.getAngleY() < -back_threshold && !is_moving_left && !is_moving_right && !is_moving_forward){
    Mouse.move(0,-cursor_speed,0);
    is_moving_back = true;  
  }
  else is_moving_back = false;
}
void mouse_move_z(int cursor_speed){ // same as mouse_move but uses z axis for left and right.
  if(mpu6050.getAngleZ() > threshold && !is_moving_back && !is_moving_right && !is_moving_z_right){
    Mouse.move(-cursor_speed,0,0);
    is_moving_z_left = true;
  }
  else is_moving_z_left = false;
  if(mpu6050.getAngleZ() < -threshold && !is_moving_back && !is_moving_right && !is_moving_z_left){
    Mouse.move(cursor_speed,0,0);
    is_moving_z_right = true;
  }
  else is_moving_z_right = false;
  if(mpu6050.getAngleY() > forward_threshold && !is_moving_back && !is_moving_z_left && !is_moving_z_right){
    Mouse.move(0,cursor_speed,0);
    is_moving_forward = true;
  }
  else is_moving_forward = false;
  if(mpu6050.getAngleY() < -back_threshold && !is_moving_forward && !is_moving_z_left && !is_moving_z_right){
    Mouse.move(0,-cursor_speed,0); 
   is_moving_back = true;   
  }
  else is_moving_back = false;
}
void WASD(){ // send W,A,S or D keys instead of moving mouse
  if(mpu6050.getAngleX() > threshold){
    if(d_pressed == false){
      Keyboard.press('d');
      d_pressed = true;
    }
  }
  else{
    if(d_pressed == true){
      Keyboard.release('d');
      d_pressed = false;
    }
  }

  if(mpu6050.getAngleX() < -threshold){
    if(a_pressed == false){
      Keyboard.press('a');
      a_pressed = true;
    }
  }
  else{
    if(a_pressed == true){
      Keyboard.release('a');
      a_pressed = false;
    }
  }

  if(mpu6050.getAngleY() > forward_threshold){
    if(w_pressed == false){
      Keyboard.press('w');
      w_pressed = true;
    }
  }
  else{
    if(w_pressed == true){
      Keyboard.release('w');
      w_pressed = false;
    }
  }

  if(mpu6050.getAngleY() < -back_threshold){
    if(s_pressed == false){
      Keyboard.press('s');
      s_pressed = true;
    }
  }
  else{
    if(s_pressed == true){
      Keyboard.release('s');
      s_pressed = false;
    }
  }     
}
bool mouse_is_moving(){ // function to check if the mouse is moving
  if(mpu6050.getAngleX() > threshold || mpu6050.getAngleX() < -threshold || mpu6050.getAngleY() > forward_threshold || mpu6050.getAngleY() < -back_threshold){
    return true;
  }
  else{
    return false;
  }
}
//-------------------------------------------------------------------------------//

//----------------LCD-MENU---------------------------------------------//
void menu_cycle(){ // cycle through menu items and pages
  if(down_state==false && item >= 3 && page < 3 && millis() > timer){ // if the down button is pressed and the last menu item is selected. go to next page
    page++;
    item=0;
    lcd.clear();
    timer = millis() + (200);
  }
  if(up_state==false && item <= 0 && page > 0 && millis() > timer){ // if the up button is pressed and the first menu item is selected go to the previouse page
    page--;
    item=0;
    lcd.clear();
    timer = millis() + (200);
  }
  if(down_state==false && item < 3 && millis() > timer){ // if the down button is pressed and the last item in not already selected go to the next item
    item++;
    timer = millis() + (200);
  }
  if(up_state==false && item > 0 && millis() > timer){ // if the up button is pressed and the first item is not already selected go to the previouse item
    item--; 
    timer = millis() + (200); 
  }
}
void check_saved(){ // read saved setting values from EEPROM
  if(EEPROM.read(0) < 255){ // read adress 0 of EEPROM and assign the value to mouse_mode
    if(EEPROM.read(0)==1) mouse_mode=true; 
    if(EEPROM.read(0)==0) mouse_mode=false;
  }
  if(EEPROM.read(1) < 255){ // do the same for the other settings
    if(EEPROM.read(1)==1) WASD_mode=true;
    if(EEPROM.read(1)==0) WASD_mode=false;
  }
  if(EEPROM.read(2) < 255){
    if(EEPROM.read(2)==1)  mouse_mode_z=true;
    if(EEPROM.read(2)==0)  mouse_mode_z=false;
  }
  if(EEPROM.read(3) < 255){
    if(EEPROM.read(3)==1)  mouse_acceleration=true;
    if(EEPROM.read(3)==0)  mouse_acceleration=false;
  }
  if(EEPROM.read(4) < 255){
    mouse_acc_delay = EEPROM.read(4) * 4;
  }
  if(EEPROM.read(5) < 255){
    mouse_speed_delay = EEPROM.read(5);
  }
  if(EEPROM.read(6) < 255){
    threshold = EEPROM.read(6);
  }
  if(EEPROM.read(7) < 255){
    forward_threshold = EEPROM.read(7);
  }
  if(EEPROM.read(8) < 255){
    back_threshold = EEPROM.read(8);
  } 
}
bool user_activity(){ // check if user is pressing any of the menu buttons
  up_state = digitalRead(up);
  select_state = digitalRead(select);
  down_state = digitalRead(down);
  if(select_state==false || up_state==false || down_state==false){ // if they are 
    return true; // return true
  }
  else{
    return false;
  }
}
void display_menu(){ // display/update the menu
  up_state = digitalRead(up);        // read the button states
  select_state = digitalRead(select);
  down_state = digitalRead(down);

  //-------Page_0--------//
  if(page == 0){
    lcd.setCursor(0, 0);
    lcd.print("Operation Mode");
    
    lcd.setCursor(0,2); 
    lcd.print("Mouse");    
    if(item == 0) lcd.setInverseOutput(true);
    if(mouse_mode==true) lcd.print(" ON ");
    if(mouse_mode==false) lcd.print(" OFF");
    lcd.setInverseOutput(false);
    

    lcd.setCursor(0,3);   
    lcd.print("WASD");   
    if(item == 1) lcd.setInverseOutput(true);
    if(WASD_mode==true) lcd.print(" ON ");
    if(WASD_mode==false) lcd.print(" OFF");
    lcd.setInverseOutput(false);
    

    lcd.setCursor(0,4);  
    lcd.print("Mouse-Z");  
    if(item == 2) lcd.setInverseOutput(true);
    if(mouse_mode_z==true) lcd.print(" ON ");
    if(mouse_mode_z==false) lcd.print(" OFF");
    lcd.setInverseOutput(false);

    lcd.setCursor(0,5);
    lcd.print("Speed");
    if(item == 3) lcd.setInverseOutput(true);
    lcd.print(" ");
    lcd.print(default_speed);
    lcd.setInverseOutput(false);
    
    
    
    if(item==0 && select_state==false && millis() > timer){
      mouse_mode = !mouse_mode;
      if(mouse_mode==true){
        WASD_mode=false; 
        mouse_mode_z=false;
      }
      timer = millis() + (200);
    }
    if(item==1 && select_state==false && millis() > timer){
      WASD_mode = !WASD_mode;
      if(WASD_mode==true){
        mouse_mode=false;
        mouse_mode_z=false;
      }
      timer = millis() + (200);
    }
    if(item==2 && select_state==false && millis() > timer){
      mouse_mode_z = !mouse_mode_z;
      if(mouse_mode_z==true){
        mouse_mode=false;
        WASD_mode=false;
      }
      timer = millis() + (200);
    }
    while(item==3 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        default_speed = default_speed -1;
        lcd.clearLine();
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        default_speed = default_speed +1;
        lcd.clearLine();
        timer = millis() + 200;
      }
    }
    menu_cycle();    
  }
  //-------Page_1--------//
  if(page == 1){
    lcd.setCursor(0, 0);
    lcd.print("Acceleration");
    
    lcd.setCursor(0,2);    
    lcd.print("Acc"); 
    if(item == 0) lcd.setInverseOutput(true); 
    if(mouse_acceleration==true) lcd.print(" ON ");
    if(mouse_acceleration==false) lcd.print(" OFF");
    lcd.setInverseOutput(false);
    

    lcd.setCursor(0,3);
    lcd.print("Acc delay");
    if(item == 1) lcd.setInverseOutput(true);
    lcd.print(" ");
    lcd.print(mouse_acc_delay);
    lcd.setInverseOutput(false);
    
    
    lcd.setCursor(0,4); 
    lcd.print("Spd delay");
    if(item == 2) lcd.setInverseOutput(true);
    lcd.print(" ");
    lcd.print(mouse_speed_delay);
    lcd.setInverseOutput(false);

    lcd.setCursor(0,5);
    lcd.print("Max speed");
    if(item == 3) lcd.setInverseOutput(true);
    lcd.print(" ");
    lcd.print(max_speed);
    lcd.setInverseOutput(false);

    if(item==0 && select_state==false && millis() > timer){
      mouse_acceleration = !mouse_acceleration;
      timer = millis() + 200;
    }

    while(item==1 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        lcd.setCursor(0,3);
        lcd.clearLine();
        mouse_acc_delay = mouse_acc_delay -25;
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        lcd.setCursor(0,3);
        lcd.clearLine();
        mouse_acc_delay = mouse_acc_delay +25;
        timer = millis() + 200;
      }
    }
    while(item==2 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        lcd.setCursor(0,4);
        lcd.clearLine();
        mouse_speed_delay = mouse_speed_delay -1;
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        lcd.setCursor(0,4);
        lcd.clearLine();
        mouse_speed_delay = mouse_speed_delay +1; 
        timer = millis() + 200;
      }
    }
    while(item==3 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        lcd.setCursor(0,5);
        max_speed = max_speed -1;
        lcd.clearLine();
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        lcd.setCursor(0,5);
        max_speed = max_speed +1;
        lcd.clearLine();
        timer = millis() + 200;
      }
    }
    menu_cycle(); 
  }
  //-------Page_2--------//

  if(page==2){
    lcd.setCursor(0, 0);
    lcd.print("Thresholds");

    lcd.setCursor(0,2);
    lcd.print("LR"); 
    if(item == 0) lcd.setInverseOutput(true);   
    lcd.print(" ");
    lcd.print(threshold);
    lcd.setInverseOutput(false);

    lcd.setCursor(0,3);
    lcd.print("Fore");
    if(item == 1) lcd.setInverseOutput(true); 
    lcd.print(" ");
    lcd.print(forward_threshold);
    lcd.setInverseOutput(false);

    lcd.setCursor(0,4);
    lcd.print("Aft");
    if(item == 2) lcd.setInverseOutput(true);
    lcd.print(" ");
    lcd.print(back_threshold);
    lcd.setInverseOutput(false);

    lcd.setCursor(0,5);
    lcd.print("   ");
    if(item == 3) lcd.setInverseOutput(true);
    lcd.print("  ");
    lcd.setInverseOutput(false);


    while(item==0 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        lcd.setCursor(0,2);
        threshold = threshold -1;
        lcd.clearLine();
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        lcd.setCursor(0,2);
        threshold = threshold +1;
        lcd.clearLine();
        timer = millis() + 200;
      }
    }
    while(item==1 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        lcd.setCursor(0,3);
        forward_threshold = forward_threshold -1;
        lcd.clearLine();
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        lcd.setCursor(0,3);
        forward_threshold = forward_threshold +1;
        lcd.clearLine();
        timer = millis() + 200;
      }
    }
    while(item==2 && select_state==false && millis() > timer){
      up_state = digitalRead(up);
      select_state = digitalRead(select);
      down_state = digitalRead(down);
      
      if(down_state == false && millis() > timer){
        lcd.setCursor(0,4);
        back_threshold = back_threshold -1;
        lcd.clearLine();
        timer = millis() + 200;
      }
      if(up_state == false && millis() > timer){
        lcd.setCursor(0,4);
        back_threshold = back_threshold +1;
        lcd.clearLine();
        timer = millis() + 200;
      }
    }
    menu_cycle();       
  }
  
  if(page==3){
    lcd.setCursor(0, 0);
    lcd.print("Save");
    
    lcd.setCursor(0, 2);
    if(item == 0) lcd.setInverseOutput(true);
    lcd.print("Save");
    lcd.setInverseOutput(false);

    if(item==0 && select_state==false && millis() > timer){
      EEPROM.put(0, mouse_mode);
      EEPROM.put(1, WASD_mode);
      EEPROM.put(2, mouse_mode_z);
      EEPROM.put(3, mouse_acceleration);
      EEPROM.put(4, mouse_acc_delay/4);
      EEPROM.put(5, mouse_speed_delay);
      EEPROM.put(6, threshold);
      EEPROM.put(7, forward_threshold);
      EEPROM.put(8, back_threshold);
      lcd.setCursor(0,3);
      lcd.print("Save Complete!");
      delay(2000);
      lcd.setCursor(0,3);
      lcd.clearLine();   
      timer = millis() + (200);
    }    
    menu_cycle();
  } 
}
//--------------------------------------------------------------------//
void detect_imu(){ // detect if IMU is connected
  select_state = digitalRead(select);
  for(adress=1; adress < 127; adress++){
    Wire.beginTransmission(adress);
    error = Wire.endTransmission();
    if(error==0){
      lcd.clear();
      Serial.print("IMU Detected!");
      lcd.setCursor(0,2);
      lcd.print("IMU Detected!");
      delay(1000);
      lcd.clear();
      devices++;
    }
    else if(error==4){
      lcd.clear();
      Serial.print("unknown error");
      lcd.setCursor(0,2);
      lcd.print("Unknown Error!");
      while(true){}
    }
  }
  if(devices==0 && has_run==false){
    Serial.print("IMU Not Found!");
    lcd.setCursor(0,2);
    lcd.print("IMU Not Found!");
    has_run=true;
  }
  if(select_state==false){
    calibration_override=true;
    return;
  }
  if(devices==0){ 
    detect_imu();
  }
}
