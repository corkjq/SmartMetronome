#include "mbed.h"
#include "C12832.h"

C12832 lcd(p5, p7, p6, p8, p11);
Serial pc(USBTX, USBRX);
           
void stop_session(void);
void print_time(void);
void input_detection (void);
int bpm_select(void);
char difficulty_select(void);
float session_time_select(void);

Ticker metronome_rate, hit_detect, countdown, pattern_metronome, pattern_metronome2, pattern_rate, pattern_rate2;
Timer t1, t2;
Timeout finish;
DigitalOut beatLED(LED1);
DigitalOut patternLED(LED2);
DigitalOut patternLED2(LED3);
DigitalOut hitLED(LED4);
DigitalOut greenLED(p23);
DigitalOut redLED(p24); 
PwmOut speaker(p26);
AnalogIn sensor(p17); 
AnalogIn pot1(p19);
AnalogIn pot2(p20);

int counter;
float tempo_period, bpm, accuracy, max_hits, successful_hits, session_time, diff_period, sensor_value; 
char difficulty, c_bpm, c_sess, c_diff, c, mode; 

void stop_session(){ //After session report
  if (mode == 'p'){
    max_hits = session_time*3;
  }
  else {
    max_hits = session_time*(bpm/60);
  }
    
  if (successful_hits > max_hits){
    accuracy = 1.00;
  }
  else {
    accuracy = successful_hits/max_hits;
  }

  pc.printf(" After Session Report:\n");
  if (accuracy*100 < 25.00){
    pc.printf(" Your session has ended, You need more practice!\n");
  }
  else if (accuracy*100 < 50.00 && accuracy*100 > 25.00){
    pc.printf(" Your session has ended, Good job!\n");  
  }
  else if (accuracy*100 < 75.00 && accuracy*100 > 50.00){
    pc.printf(" Your session has ended, Great job! Well done!!\n");
  }
  else {
    pc.printf(" Your session has ended, Amazing! You're a pro!\n");  
  }
  pc.printf(" You had %.3f successful hits!\n", successful_hits);
  pc.printf(" Accuracy: %.3f%%\n", accuracy*100);
  beatLED = 0;
  greenLED = 0;
  redLED = 0;
  speaker = 0;
  wait(3);
  lcd.cls();
  exit(0);
}

void ISR1(){ //Metronome ticker
    speaker = 0.50*pot1;
    beatLED = !beatLED;
    wait(0.1);
    speaker = 0;
    beatLED = !beatLED;
}

void ISR3(){ //Additional Metronome for pattern mode
    wait(0.2);
    speaker = 0.50*pot1;
    patternLED = !patternLED;
    wait(0.1);
    speaker = 0;
    patternLED = !patternLED;
}

void ISR4(){ //Additional Metronome for pattern mode
    wait(0.4);
    speaker = 0.50*pot1;
    patternLED2 = !patternLED2;
    wait(0.1);
    speaker = 0;
    patternLED2 = !patternLED2;
}

void ISR2(){ //Hit detect ticker
    t1.start();
    input_detection();
}

void ISR5(){ //Additional hit detect for pattern mode
    wait(0.2);
    t1.start();
    input_detection();
}

void ISR6(){ //Additional hit detect for pattern mode
    wait(0.4);
    t1.start();
    input_detection();
}

void input_detection(){ //Hit detetion function
    if (t1.read() < diff_period){
        if (sensor.read() > 0.013f){ //Sensor sensitivity setting
            greenLED = 1;
            redLED = 0;
            successful_hits = successful_hits + 1;
        }
        else {
            greenLED = 0;
            redLED = 1;
        }   
    }
    t1.stop();
}

int bpm_select(){ //BPM selection
  c = pc.getc();
  switch (c){
      case 'a': bpm = 60.000;
      break;
      case 'b': bpm = 65.000;
      break;
      case 'c': bpm = 70.000;
      break;
      case 'd': bpm = 75.000;
      break;
      case 'e': bpm = 80.000;
      break;
      case 'f': bpm = 85.000;
      break;
      case 'g': bpm = 90.000;
      break;
      case 'h': bpm = 95.000;
      break;
      case 'i': bpm = 100.000;
      break;
      case 'j': bpm = 105.000;
      break;
      case 'k': bpm = 110.000;
      break;
      case 'l': bpm = 115.000;
      break;
      case 'm': bpm = 120.000;
      break;
      case 'n': 
        bpm = 90.000;
        mode = 'p';
      break;
  }
  return bpm;
}

float session_time_select(){ //Session Length selection
  c = pc.getc();
  if (c == 'a'){
      session_time = 10.000;
  }
  else if (c == 'b'){
      session_time = 30.000;
  }
  else if (c == 'c'){
      session_time = 60.000;
  }
  else if (c == 'd'){
      session_time = 300.000;
  }
  else if (c == 'e'){
      session_time = 600.000;
  }
  else if (c == 'f'){
      session_time = 900.000;
  }
  return session_time;
}

int main() {
  beatLED = 0;
  patternLED = 0;
  patternLED2 = 0;
  hitLED = 0;
  greenLED = 1;
  redLED = 0;
  diff_period = 0.0;
  mode = 't';
  
  pc.printf("Mbed is ready. Choose settings!\n");
  bpm = bpm_select();
  c_diff = pc.getc();
  if (c_diff == 'a'){
    difficulty = 'e';
    diff_period = 0.8;
  }
  else if (c_diff == 'b'){
    difficulty = 'm';
    diff_period = 0.4;
  }
  else if (c_diff == 'c'){
    difficulty = 'h';
    diff_period = 0.2;
  }
  session_time = session_time_select();
  
  counter = session_time;
  tempo_period = 60/bpm;
  
  lcd.printf("BPM= %.3f\n", bpm); //Information displayed on LCD on mbed board
  lcd.printf("Diff= %c\n", difficulty);
  lcd.printf("Session Time= %f\n", session_time);
  speaker.period(1.0/350.0); //Speaker settings ready
  
  if (mode == 'p'){ //Tickers and configuration if Pattern Mode is selected
    metronome_rate.attach(&ISR1, 1);
    pattern_metronome.attach(&ISR3, 1);
    pattern_metronome2.attach(&ISR4, 1);
    speaker.period(1.0/250.0);
    hit_detect.attach(&ISR2, 1);
    pattern_rate.attach(&ISR5, 1);
    pattern_rate2.attach(&ISR6, 1);
    
    finish.attach(&stop_session, session_time);
  }
  else { //Tickers and configuration if Tempo/BPM Mode is selected
    metronome_rate.attach(&ISR1, tempo_period);
    wait(tempo_period*4);
    speaker.period(1.0/250.0);
    metronome_rate.attach(&ISR1, tempo_period);
    hit_detect.attach(&ISR2, tempo_period-(diff_period/2));
    
    finish.attach(&stop_session, session_time);
  }
     
}