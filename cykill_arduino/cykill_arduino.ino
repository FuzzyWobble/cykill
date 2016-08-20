//fuzzy wobble july 2016
//fuzzywobble.com

/*
 
  Bike Wires:
  
  Over approx a 10deg pedal rotation the resistance accross the wires will drop from ~230kohm to 1ohm
  If we make these wires part of a voltage divider, we can read Vout, and check when it drops to 1ohm
  R2 = 22kohm Resistor
  R1 = Bike Wires
  
  Vin-----R1-----o-----R2-----|Gnd
                 |
                 |
           Vout (Arduino A2)
           
  Vout = Vin*(R2/(R2+R1))
  When R1 = 230 kohm
  Vout = 5.0*(22/(22+230)) != 5V
  When R1 = 1 ohm
  Vout = 5.0*(22000/(22000+1)) = 5V (1023)   

  So when a person is pedalling the bike, we can measure the number of 5V (1023) readings we have in Arduino A2. 
  The fewer readings, the faster the pedalling. 
  
*/

//0: startup
//1: input 
//2: running
//3: game over (shutdown)
//4: done
//5: error
int state = 0;

//__________________________________ neo pixel LED ring 
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(16, 2, NEO_GRB + NEO_KHZ800);
int pixel_ring_order[12] = {15,14,13,12,11,10,9,8,7,6,5,4}; //we are neglecting 4 pixels and running in reverse, so let's create a new order.

//__________________________________ neo pixel LED bars
Adafruit_NeoPixel pixels_intensity = Adafruit_NeoPixel(8, 9, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_duration = Adafruit_NeoPixel(8, 10, NEO_GRB + NEO_KHZ800);

//__________________________________intensity
int intensity_val_prev = -1;
int intensity_read = 0;
int intensity = 7; //4-10

//__________________________________ duration
int duration_val_prev = -1;
int duration_read = 0;
int duration = 40; //20-60 min

//__________________________________ runner
float level = 12.0;
float level_reduction = 0.0008; //how much the level is subtracted each loop. may need to adjust this for faster microprocessors. 
int read_streak = 0;
int bike_speed; //0-100
long duration_start_ms;
int duration_brightness;
float complete_pct;

//__________________________________ other
bool flip_startup = true;
bool flip_gameover = true;
bool flip_done = true;



//#####################################################################
//#####################################################################
void setup() {

  Serial.begin(9600);

  //__________________________________neo pixel LEDs 
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(20); //These LEDs are insanelly bright. Must turn down brightness.
  pixels_intensity.begin();
  pixels_intensity.setBrightness(20); //These LEDs are insanelly bright. Must turn down brightness.
  pixels_duration.begin();
  pixels_duration.setBrightness(20); //These LEDs are insanelly bright. Must turn down brightness.

  //__________________________________intensity
  pinMode(A0,INPUT); //intensity analog in is A0

  //__________________________________duration
  pinMode(A1,INPUT); //duration analog in is A1

  //__________________________________on/off or start/stop
  pinMode(8,INPUT_PULLUP); //on/off is on pin 8
  pinMode(13,OUTPUT); //power tail
  digitalWrite(13,LOW); //power tail off

  //__________________________________runner
  pinMode(A2,INPUT); //read bike on pin A2

  //__________________________________check if in off state
  if(digitalRead(8)==LOW){ //was left on
    Serial.println("Error. Toggle was left on.");
    state = 5; //error
  }

}


    void set_intensity_LED(int n){
      for(int i=0;i<8;i++){
        pixels_intensity.setPixelColor(7-i, pixels.Color(0,0,0)); 
        pixels_intensity.show();
      }
      for(int i=0;i<n+1;i++){
        pixels_intensity.setPixelColor(7-i, pixels.Color(0,255,0)); 
        pixels_intensity.show();        
      }
    }
    void set_duration_LED(int n){
      for(int i=0;i<8;i++){
        pixels_duration.setPixelColor(7-i, pixels.Color(0,0,0)); 
        pixels_duration.show();
      }
      for(int i=0;i<n+1;i++){
        pixels_duration.setPixelColor(7-i, pixels.Color(0,255,0)); 
        pixels_duration.show();        
      }
    }


//#####################################################################
//#####################################################################
void loop() {





  //_____________________________________________________
  //_____________________________________________________ state 0, startup
  if(state==0){

    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
      pixels.show();
    }
    for(int i=0;i<8;i++){
      pixels_intensity.setPixelColor(7-i, pixels.Color(0,0,0)); 
      pixels_intensity.show();
      pixels_duration.setPixelColor(7-i, pixels.Color(0,0,0));
      pixels_duration.show();
     }
    
    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,150,0)); 
      pixels.show(); 
      delay(110); 
    }
    for(int i=0;i<8;i++){
      pixels_intensity.setPixelColor(7-i, pixels.Color(0,150,0)); 
      pixels_intensity.show(); 
      pixels_duration.setPixelColor(7-i, pixels.Color(0,150,0));
      pixels_duration.show(); 
      delay(110); 
    }

    delay(500);
    
    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
      pixels.show();
    }
    for(int i=0;i<8;i++){
      pixels_intensity.setPixelColor(7-i, pixels.Color(0,0,0)); 
      pixels_intensity.show();
      pixels_duration.setPixelColor(7-i, pixels.Color(0,0,0));
      pixels_duration.show();
     }
    
     state = 1;
    
  }




  
  //_____________________________________________________
  //_____________________________________________________ state 1, input
  if(state==1){


    // SET INTENSITY
    //read intensity 
    intensity_read = map(analogRead(A0), 0, 1023, 0, 7);
    //if knob is turned set intensity
    if(abs(intensity_read-intensity_val_prev) >= 1){
      set_intensity_LED(intensity_read);
      intensity_val_prev = intensity_read;
      intensity = map(intensity_val_prev,0,7,4,11);
    }
    //if not set yet, set intensity
    if(intensity_val_prev==-1){
      set_intensity_LED(intensity_read);
    }


    // SET DURATION
    //read intensity 
    duration_read = map(analogRead(A1), 0, 1023, 0, 7);
    //if knob is turned set intensity
    if(abs(duration_read-duration_val_prev) >= 1){
      set_duration_LED(duration_read);
      duration_val_prev = duration_read;
      duration = map(intensity_val_prev,0,7,10,60);
    }
    //if not set yet, set intensity
    if(duration_val_prev==-1){
      set_duration_LED(duration_read);
    }
   

    // SWITCH TO STATE 2
    if(digitalRead(8)==LOW){ 
      Serial.println("Duration: "+String(duration));
      Serial.println("Intensity: "+String(intensity));
      duration_start_ms = millis();
      digitalWrite(13,HIGH); //power on!
      state = 2; 
    }

    // WHITE BLINKING
    pixels.setBrightness(20);
    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(255,255,255));
      pixels.show(); 
    }
    delay(150);
    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
      pixels.show(); 
    }
    delay(150);
    
  }





  //_____________________________________________________
  //_____________________________________________________ state 2, running
  if(state==2){

    //__ CALCULATE LEVEL (0-12) 
    if(analogRead(A2)>1000){
      read_streak++;
      Serial.println("+");
      if(read_streak>100){
        level -= 1.0;
        read_streak = 0;
      }
    }else{
      if(read_streak>0){
        Serial.println("STREAK:"+(String)read_streak);
        //the ranges 6-12 are totally random and depend complete on speed of microprocessor
        if(read_streak<6){read_streak=6;} 
        if(read_streak>12){read_streak=12;}
        read_streak = map(read_streak,6,12,0,80); 
        bike_speed = 100 - read_streak;
        Serial.println("SPEED:"+(String)bike_speed);
        level += (((float)bike_speed/100.0))*1.5;
        if(level>=12){level = 12.01;}
      }
      read_streak = 0;
    }
    level -= (level_reduction*intensity);
    if(level<=1){
      level = 0;
      state = 3; //!state!
    }
    //Serial.println("LEVEL:"+(String)level);
    

    //__ SHOW LEVEL ON RING
    pixels.setBrightness(20);
    for(int i=0;i<(int)level;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(255,255,255));
      pixels.show(); 
    }
    for(int i=(int)level;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
      pixels.show(); 
    }

    //__ DURATION LED FADES OUT
        complete_pct = (float)( ((float)millis()-(float)duration_start_ms) / ((float)duration*1000.0) );
    //    //Serial.println(complete_pct);
    //    duration_brightness = 100 - (100*complete_pct);
    //    analogWrite(5,(int)duration_brightness);

    //__ IS TIME UP?
    if(complete_pct>=1.0){
      state = 4; //!state!
    }
    
  }





  //_____________________________________________________
  //_____________________________________________________ state 3, game over
  if(state==3){

    digitalWrite(13,LOW); //power off!

    pixels.setBrightness(20); 
    
    if(flip_gameover){
      for(int i=0;i<12;i++){
        pixels.setPixelColor(pixel_ring_order[i], pixels.Color(255,0,0));
        pixels.show(); 
      }      
    }else{
      for(int i=0;i<12;i++){
        pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
        pixels.show(); 
      }        
    }
    flip_gameover = !flip_gameover;

    // SWITCH TO STATE 0
    if(digitalRead(8)!=LOW){ 
      for(int i=0;i<12;i++){
        pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
        pixels.show(); 
      } 
      flip_startup = true;
      level = 12.0;
      state = 0; 
      //digitalWrite(13,HIGH); //ummm should we have this
    }
    
    delay(50);
    
  }




  //_____________________________________________________
  //_____________________________________________________ state 4, done
  if(state==4){

    //__ RING BLINKS GREEN
    pixels.setBrightness(20); 
    if(flip_done){
      for(int i=0;i<12;i++){
        pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,255,0));
        pixels.show(); 
      }      
    }else{
      for(int i=0;i<12;i++){
        pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
        pixels.show(); 
      }        
    }
    flip_done = !flip_done;

    //__ SWITCH TO STATE 0
    if(digitalRead(8)!=LOW){ 
      for(int i=0;i<12;i++){
        pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
        pixels.show(); 
      } 
      flip_startup = true;
      level = 12.0;
      state = 0; //!state!
    }
    
    delay(250);
        
  }



  
  //_____________________________________________________
  //_____________________________________________________ state 5, error
  if(state==5){
    pixels.setBrightness(20);
    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(255,0,0));
      pixels.show(); 
    }
    delay(150);
    for(int i=0;i<12;i++){
      pixels.setPixelColor(pixel_ring_order[i], pixels.Color(0,0,0));
      pixels.show(); 
    }
    delay(150);
  }




}
