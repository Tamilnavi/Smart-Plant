#include <Servo.h>
Servo s1,s2;

int touch = 8;
bool state = false;
int mistin = 12;
int mistop = 13;
bool s2_state = false;
void setup() {
  // put your setup code here, to run once:
  s1.attach(9);
  s2.attach(10);
  s1.write(25);
  s2.write(80);
  delay(1000);
  s2.detach();
  
  pinMode(touch, INPUT_PULLUP);
  pinMode(mistin, INPUT);
  pinMode(mistop, OUTPUT);
  digitalWrite(mistop, LOW);

  

}

void loop() {
  if(digitalRead(mistin) == HIGH && s2_state == false){
    s2.attach(10);
    s2.write(140);
    delay(100);
    s2.detach();
    s2_state = true;
    digitalWrite(mistop, HIGH);
  
  }
  if(digitalRead(mistin) == LOW && s2_state == true){
 
    digitalWrite(mistop, LOW);
    s2.attach(10);
    s2.write(80);
    delay(100);
    s2.detach();
    s2_state = false;
    
  }
  
  

if(digitalRead(touch) == HIGH && !state){
  s1.attach(9);
  delay(100);
  for( int i = 25 ; i <=100; i++){
      s1.write(i);
      delay(50);
  }
  state = true;
  delay(1000);
  s1.detach();

}
if(digitalRead(touch) == HIGH && state){
  s1.attach(9);
  delay(100);
  for( int i = 100 ; i >=25; i--){
      s1.write(i);
      delay(50);
  }
  state = false;
  delay(1000);
  s1.detach();
}

}
