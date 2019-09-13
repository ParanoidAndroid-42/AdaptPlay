#include <Mouse.h>    // default
#include <Keyboard.h> // default

/*
Pins: (with joystick on left)

A0 -> right movement sensor
A1 -> left movement sensor
A2 -> forward movement sensor
A3 -> backward movement sensor

2 -> left click
3 -> right click

*/

// Settings
//-------------------//
bool right_hand = true;    // Right handed mode
bool WASD_mode = false;     // WASD mode
float sensor_threshold = 18; // Sensor LR threshold
const int average = 20;     // number of samples taken for average
int mouse_speed = 4;        // mouse movement speed
//-------------------//

unsigned long timer;
unsigned long timer_goal;
 
int sensor;
int average_loop = average;
float sensor_average_right;
float sensor_average_left;
float sensor_average_forward;
float sensor_average_back;
float average_right;
float average_left;
float average_forward;
float average_back;

bool is_moving_left = false;
bool is_moving_right = false;
bool is_moving_forward = false;
bool is_moving_back = false;

//calibration
int total_value;
float average_value;

float right_offset;
float left_offset;
float back_offset;
float forward_offset;

float calibrated_sensor_value;
float threshold;

unsigned long acc_timer;

boolean isPressedRight;
boolean isPressedLeft;

bool w_pressed, a_pressed, s_pressed, d_pressed;
void setup() {
  pinMode(A0, INPUT);//Right sensor
  pinMode(A1, INPUT);//Left sensor
  pinMode(A2, INPUT);//Forward sensor
  pinMode(A3, INPUT);//Back sensor
  //Serial.begin(9600); //Enable to output sensor values to serial monitor
  pinMode(2, INPUT_PULLUP);//Mouse click
  pinMode(3, INPUT_PULLUP);//Mouse click
  calibrate();//Zero out any variation in sensor reading
  threshold = calibrated_sensor_value - sensor_threshold;//Calculate the threshold based on calibrated values
}

void loop() {
  for(average_loop > 0; average_loop--;){ // Take an average of sensor readings to smooth out variations
    sensor_average_forward += analogRead(A0); 
    sensor_average_back += analogRead(A1);
    sensor_average_left += analogRead(A2);
    sensor_average_right += analogRead(A3);
  }
  //Calculate the average for each direction
  average_right = sensor_average_right/average;
  average_left = sensor_average_left/average;
  average_forward = sensor_average_forward/average;
  average_back = sensor_average_back/average;

  //Print sensor values for debuging
  Serial.print(" Back0 " );
  Serial.print(average_forward - forward_offset);
  Serial.print(" Left1 ");
  Serial.print(average_back - back_offset);
  Serial.print(" Forward2 ");
  Serial.print(average_left - left_offset);
  Serial.print(" Right3 ");
  Serial.println(average_right - right_offset);
  Serial.print(" Threshold ");
  Serial.println(threshold);

//  Serial.print(" 0 |" );
//  Serial.print(analogRead(A0));
//  Serial.print(" 1 |");
//  Serial.print(analogRead(A1));
//  Serial.print(" 2 |");
//  Serial.print(analogRead(A2));
//  Serial.print(" 3 |");
//  Serial.println(analogRead(A3));
//  Serial.print(" Threshold ");
//  Serial.println(threshold);
//  
  if(right_hand){ //Check mode and run appropriate function
    if(WASD_mode){
      WASD_inverted(); 
    }
    doButtons_inverted();
    mouse_move_inverted(mouse_speed);
  }
  else{
    if(WASD_mode){
      WASD();
    }
    doButtons();
    mouse_move(mouse_speed);
  }
  
  //Reset sensor averages for next loop
  sensor_average_right=0;
  sensor_average_left=0;
  sensor_average_forward=0;
  sensor_average_back=0;
  average_loop=average;
}

void mouse_move(int velocity){//Move the mouse
  if((average_forward - forward_offset) < threshold && average_right > 10 && !is_moving_left && !is_moving_right && !is_moving_back){
    Mouse.move(-velocity,0,0);
    is_moving_forward=true;
  }
  else is_moving_forward=false;

  if((average_back - back_offset) < threshold && average_left > 10 && !is_moving_left && !is_moving_right && !is_moving_forward){
    Mouse.move(velocity,0,0);
    is_moving_back=true;
  }
  else is_moving_back=false;

  if((average_right - right_offset) < threshold && average_forward > 10 && !is_moving_left && !is_moving_forward && !is_moving_back){
    Mouse.move(0,-velocity,0);
    is_moving_right=true;
  }
  else is_moving_right=false;

   if((average_left - left_offset) < threshold && average_back > 10 && !is_moving_right && !is_moving_forward && !is_moving_back){
    Mouse.move(0,velocity,0);
    is_moving_left=true;
  }
  else is_moving_left=false;
}

void mouse_move_inverted(int velocity){//Move the mouse with all axis inverted for right hand mode
  if((average_forward - forward_offset) < threshold && average_right > 10 && !is_moving_left && !is_moving_right && !is_moving_back){
    Mouse.move(velocity,0,0);
    is_moving_forward=true;
  }
  else is_moving_forward=false;

  if((average_back - back_offset) < threshold && average_left > 10 && !is_moving_left && !is_moving_right && !is_moving_forward){
    Mouse.move(-velocity,0,0);
    is_moving_back=true;
  }
  else is_moving_back=false;

  if((average_right - right_offset) < threshold && average_forward > 10 && !is_moving_left && !is_moving_forward && !is_moving_back){
    Mouse.move(0,velocity,0);
    is_moving_right=true;
  }
  else is_moving_right=false;

   if((average_left - left_offset) < threshold && average_back > 10 && !is_moving_right && !is_moving_forward && !is_moving_back){
    Mouse.move(0,-velocity,0);
    is_moving_left=true;
  }
  else is_moving_left=false;
}

void calibrate(){
  total_value = analogRead(A0) + analogRead(A1) + analogRead(A2) + analogRead(A3);//Add up the value of all sensors
  average_value = total_value/4;//Devide to find the average
  Serial.println(average_value);//Print for debuging
  
  //Calculate the offset required to make sensor readings equal when stick is neutral
  forward_offset = analogRead(A0) - average_value;
  back_offset = analogRead(A1) - average_value;
  left_offset = analogRead(A2) - average_value;
  right_offset = analogRead(A3) - average_value;
 
  calibrated_sensor_value = analogRead(A2)- left_offset;//Calculate and save the calibrated zero value of the sensors (used to calculate offsets)
}

void doButtons(){ //Left and right mouse butons
  if (digitalRead(2) == LOW && !isPressedLeft && millis() > timer) {
    Mouse.press(MOUSE_LEFT);
    isPressedLeft = true;
    timer = millis() + 400;
  } else {
    isPressedLeft = false;
    Mouse.release(MOUSE_LEFT);
  }

  if (digitalRead(3) == LOW && !isPressedRight && millis() > timer) {
    Mouse.press(MOUSE_RIGHT);
    isPressedRight = true;
    timer = millis() + 400;
  } else {
    isPressedRight = false;
    Mouse.release(MOUSE_RIGHT);
  }
}

void doButtons_inverted(){//Left and right mouse buttons inverted
  if (digitalRead(3) == LOW && !isPressedLeft && millis() > timer) {
    Mouse.press(MOUSE_LEFT);
    isPressedLeft = true;
    timer = millis() + 400;
  } else {
    isPressedLeft = false;
    Mouse.release(MOUSE_LEFT);
  }

  if (digitalRead(2) == LOW && !isPressedRight && millis() > timer) {
    Mouse.press(MOUSE_RIGHT);
    isPressedRight = true;
    timer = millis() + 400;
  } else {
    isPressedRight = false;
    Mouse.release(MOUSE_RIGHT);
  }
}

void WASD(){ // send W,A,S or D keys instead of moving mouse
  
  if((average_right - right_offset) < threshold && !s_pressed && !d_pressed && !a_pressed){
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

  if((average_left - left_offset) < threshold && !w_pressed && !a_pressed && !d_pressed){
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

  if((average_forward - forward_offset) < threshold && !w_pressed && !s_pressed && !a_pressed){
    if(d_pressed == false){
      Keyboard.press('a');
      d_pressed = true;
    }
  }
  else{
    if(d_pressed == true){
      Keyboard.release('a');
      d_pressed = false;
    }
  }

  if((average_back - back_offset) < threshold && !w_pressed && !d_pressed && !s_pressed){
    if(a_pressed == false){
      Keyboard.press('d');
      a_pressed = true;
    }
  }
  else{
    if(a_pressed == true){
      Keyboard.release('d');
      a_pressed = false;
    }
  }     
}

void WASD_inverted(){ // send W,A,S or D keys inverted
  if((average_right - right_offset) < threshold && !w_pressed && !d_pressed && !a_pressed){
    if(w_pressed == false){
      Keyboard.press('s');
      w_pressed = true;
    }
  }
  else{
    if(w_pressed == true){
      Keyboard.release('s');
      w_pressed = false;
    }
  }

  if((average_left - left_offset) < threshold && !s_pressed && !a_pressed && !d_pressed){
    if(s_pressed == false){
      Keyboard.press('w');
      s_pressed = true;
    }
  }
  else{
    if(s_pressed == true){
      Keyboard.release('w');
      s_pressed = false;
    }
  }

  if((average_forward - forward_offset) < threshold && !w_pressed && !s_pressed && !a_pressed){
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

  if((average_back - back_offset) < threshold && !w_pressed && !d_pressed && !s_pressed){
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
}
