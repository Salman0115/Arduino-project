#include <avr/io.h>
#include <avr/interrupt.h>
#define PRELOAD 0xFE0B // = 65536 - 500 decimal gives is 500 interrupts per second
bool FirstTime = true;
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
int x;
int y;
#define TEMP_OUT_H 0x41                                 // register address for temperature
#define TEMP_OUT_L 0x42                                 // register address for temperature
int16_t TEMP_OUT;
  int16_t AccX;
   int16_t AccY;
  int16_t AccZ;
float T;
String t;
int trimpotValue = 0;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MPU_addr 0x68  // I2C address of the MPU-6050

#define Row0 2
#define Row1 4
#define Row2 5
#define Row3 6
String display_or;

#define MPU_6050 0X68
#define PWR_MGMT_1 0x6B 
#define ACCEL_ZOUT_HI 0x3F
#define ACCEL_XOUT_HI 0x3B 
#define ACCEL_YOUT_HI 0x3D

#define Column0 7
#define Column1 10
#define Column2 A2
#define trimPot A3

// Define number of steps per revolution
// Our stepper has 20 steps per revolution
// This is equivalent to
// 18 degrees of a turn per step
#define stepsPerRevolution 20
float revPsec = 0.0;
// Give the motor control pins names:
#define pwmA 3
#define pwmB 11
#define breakA 9
#define breakB 8
#define dirA 12
#define dirB 13
#define sen0 A0
#define sen1 A1
String pass;
 bool qwerty;
// ---------------------------------------------------------
//                    MODULE 5 Variables
// ---------------------------------------------------------
// module5 is the login and scheduller
// module5 variables    
bool init_module5_clock;
// ---------------------------------------------------------
//                    MODULE 6 Variables
// ---------------------------------------------------------
// module6 is the trimpot reader
// module6 variables    
bool init_module6_clock;
// ---------------------------------------------------------
//                    MODULE 7 Variables
// ---------------------------------------------------------
// module7 is the motor monitor
// module7 variables    
bool init_module7_clock;
// ---------------------------------------------------------
//                    MODULE 8 Variables
// ---------------------------------------------------------
// module8 is the accelerometer driver
// module8 variables    
bool init_module8_clock;

// definition for the type of the button states
enum button_t { notPRESSED, partialPRESS, normalPRESS, heldPRESS, stuck};
button_t keyStatus[12]={notPRESSED};
// ---------------------------------------------------------
//                    MODULE 9 Variables
// ---------------------------------------------------------
// module9 is the Keypad driver
// module9 variables    
bool init_module9_clock;
// ---------------------------------------------------------
//                    MODULE 10 Variables
// ---------------------------------------------------------
// module10 is the Key display driver
// module10 variables    
bool init_module10_clock;

void testRow(char r) {
  switch (r) {
    case 0: {
      digitalWrite(Row0, 0);
      pinMode(Row0, OUTPUT);
      pinMode(Row1, INPUT_PULLUP);
      pinMode(Row2, INPUT_PULLUP);
      pinMode(Row3, INPUT_PULLUP);
      break;
    }
    case 1: {
      pinMode(Row0, INPUT_PULLUP);
      digitalWrite(Row1, 0);
      pinMode(Row1, OUTPUT);
      pinMode(Row2, INPUT_PULLUP);
      pinMode(Row3, INPUT_PULLUP);
      break;
    }
    case 2: {
      pinMode(Row0, INPUT_PULLUP);
      pinMode(Row1, INPUT_PULLUP);
      digitalWrite(Row2, 0);
      pinMode(Row2, OUTPUT);
      pinMode(Row3, INPUT_PULLUP);
      break;
    }
    case 3: {
      pinMode(Row0, INPUT_PULLUP);
      pinMode(Row1, INPUT_PULLUP);
      pinMode(Row2, INPUT_PULLUP);
      digitalWrite(Row3, 0);
      pinMode(Row3, OUTPUT);
      break;
    }
    default: {
      pinMode(Row0, INPUT_PULLUP);
      pinMode(Row1, INPUT_PULLUP);
      pinMode(Row2, INPUT_PULLUP);
      pinMode(Row3, INPUT_PULLUP);
      break;
    }
  }
}

bool testColumn(char c) {
  return (
    (c<0) ? true : (
      (c==0) ? digitalRead(Column0) : (
        (c==1) ? digitalRead(Column1) : (
          (c==2) ? digitalRead(Column2) : true
        )
      )
    )
  );
}
//////////////line///////////////
void setup() {
  // make sure this comes first in your code to avoid shorts
  pinMode(Column0, INPUT_PULLUP);
  pinMode(Column1, INPUT_PULLUP);
  pinMode(Column2, INPUT_PULLUP);

  pinMode(Row0, INPUT_PULLUP);
  pinMode(Row1, INPUT_PULLUP);
  pinMode(Row2, INPUT_PULLUP);
  pinMode(Row3, INPUT_PULLUP);
  testRow(-1); // do not test a row

  // Set the PWM and brake pins so that the direction pins can be used to control the motor:
  pinMode(dirA, OUTPUT);
  pinMode(dirB, OUTPUT);

  pinMode(pwmA, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(breakA, OUTPUT);
  pinMode(breakB, OUTPUT);

  digitalWrite(pwmA, HIGH);
  digitalWrite(pwmB, HIGH);
  digitalWrite(breakA, LOW);
  digitalWrite(breakB, LOW);

  Serial.begin(9600);
  Wire.begin();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address may also be 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed")); // has the video buffer got enough RAM?
    for(;;); // Don't proceed, loop forever
  }
  display.setTextWrap(false); // do not wrap the text
  display.setTextSize(1);     // Normal 1:1 pixel scale
  display.clearDisplay();
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();

  // initialize Timer1
  cli();         // disable global interrupts
  TCCR1A = 0;    // set entire TCCR1A register to 0
  TCCR1B = 0;    // set entire TCCR1B register to 0 
                 // (as we do not know the initial  values) 
  
  // enable Timer1 overflow interrupt:
  TIMSK1 |= (1 << TOIE1); //Atmega8 has no TIMSK1 but a TIMSK register
  // Preload 
  TCNT1= PRELOAD;
  TCCR1B |= (1 << CS11); // Sets bit CS11 in TCCR1B
  TCCR1B |= (1 << CS10); // and CS10
  //the clock source is divided by 64, i.e. one clock cycle every 64 / (16 * 10^6) = 4 * 10^(-6) = 0.000004s
  
  // This is achieved by shifting binary 1 (0b00000001)
  // to the left by CS11 or CS10 bits. This is then bitwise
  // OR-ed into the current value of TCCR1B, which effectively set
  // this one bit high.
  // enable global interrupts:
  sei();

  // initialise all modules in the system
  // snip
  // in other words, set all init_module_clock variables to true here
  init_module5_clock = true;
  init_module6_clock = true;
  init_module7_clock = true;
  init_module8_clock = true;
  init_module9_clock = true;
  init_module10_clock = true;
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);   
}

// here is the ISR executed by the CPU when the corresponding interrupt occurs
ISR(TIMER1_OVF_vect)
{
  TCNT1=PRELOAD; // reload the timer preload

  //////////////////////////////////////////////////////////
  // motor module
  //////////////////////////////////////////////////////////
  {
    static unsigned long m;
    static unsigned char thisStep = 0;

    if (((long)(micros()-m)) > (1000000L / (stepsPerRevolution*abs(revPsec)))) {
      // you can control the speed and direction by assigning the revPsec variable
      m=micros();

      switch (thisStep) {
        case 0:  // 01
          digitalWrite(dirA, LOW);
          digitalWrite(dirB, HIGH);
          if(revPsec>0) thisStep++; 
          else if(revPsec<0) thisStep=3; //direction
        break;
        case 1:  // 11
          digitalWrite(dirA, HIGH);
          digitalWrite(dirB, HIGH);
          if(revPsec>0) thisStep++;
          else if(revPsec<0) thisStep--; //direction
        break;
        case 2:  // 10
          digitalWrite(dirA, HIGH);
          digitalWrite(dirB, LOW);
          if(revPsec>0) thisStep++;
          else if(revPsec<0)  thisStep--; //direction
        break;
        case 3:  // 00
          digitalWrite(dirA, LOW);
          digitalWrite(dirB, LOW);
          if(revPsec>0) thisStep=0;
          else if(revPsec<0) thisStep--; //direction
        break;
        default: thisStep=0;
      }
    }
  }
}

/*
 * Top left is position 0,0
 * bottom right is position 20, 7
 * There are 8 lines of text at text size 1
 * There are 21 columns of text at size 1
 * These need to be scaled for different text sizes
 */
void oled(unsigned char x, unsigned char y, const __FlashStringHelper *s) {
  byte *s1 = (byte *)s;
  display.setCursor(x*6,y*8);                         // start co-ordinates
  for(byte i=0; s1[i]!=0; i++) display.print(F(" ")); // make space for the new text
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // white color for the new text
  display.setCursor(x*6,y*8);                         // start co-ordinates
  display.print(s);                                   // write the new text  
  display.display();                                  // show it
}

// returns the number on the key
char n(unsigned char kI) {
  if (kI>=11) return(-1);
  else if (kI==10) return(0);
  else return((kI+1)%10);
}

void loop() 
{

    { // module 5
      #define password 1234
      static unsigned long module_time, module_delay, userToken;
      static bool module_doStep;
      static char module_state=0; // state variable for module 6
      static unsigned char keyIndex;
      
        if (init_module5_clock) {
            module_delay = 5; 
            module_time=millis();
            module_doStep=false;
            init_module5_clock=false;
            module_state = 0;
        }
        else {
            unsigned long m;
            m=millis();
            if (m - module_time > module_delay) { // can you make this roll-over safe?
                module_time = m; module_doStep = true;
            }
            else module_doStep = false;
        }

        if (module_doStep) {
          switch(module_state){
            case -1: {
              // stop motor, apply the brakes
              // stop trimpot and motor monitor modules
              // stop accel and key displayer modules
              // stop the motor, zero the velocity of the motor
              // display "Invalid input." on the oled
            }
            case 0: {
              // stop motor, apply the brakes
              // stop trimpot and motor monitor modules
              // stop accel and key displayer modules
              // stop the motor, zero the velocity of the motor
              // display "Login password:" on the oled
              //oled(0, 5, F("HELD PRESS"));
              init_module9_clock = true;
              // userToken is zeroed      
              init_module10_clock = true;
                init_module8_clock = true;
                    init_module6_clock = true;
                
            }
            case 1: {
              // stop motor, apply the brakes
              // stop trimpot and motor monitor modules
              // stop accel and key displayer modules
              // stop the motor, zero the velocity of the motor
              // check the keyStatus for pressed keys
              // if no key is normally pressed, stay in this state
               init_module9_clock = false;
              // userToken is zeroed      
              init_module10_clock = false;
                init_module8_clock = false;
                    init_module6_clock = false;
               init_module6_clock = false;
                 init_module7_clock = false;
              for(keyIndex=0; keyIndex<12; keyIndex++)
              {

         
                if(keyStatus[keyIndex] == normalPRESS)
                {
                  module_state = 2;
                }
              }
              
              // else move to case 2
              break;
            }
            case 2: {
                 //Serial.println( T);
              // stop motor, apply the brakes
              // stop trimpot and motor monitor modules
              // stop accel and key displayer modules
              // stop the motor, zero the velocity of the motor
              // is # pressed? if so, check userToken == password and go to the open state 3
              // else go to the error state -1
              // check for validity of the input, i.e. ignore * button, etc
              // if all's well userToken = userToken*10+n(keyIndex) and go back to state 1
              init_module9_clock = false;
              // userToken is zeroed      
              init_module10_clock = false;
                init_module8_clock = false;
                    init_module6_clock = false;
               init_module6_clock = false;
               if(keyStatus[1] = partialPRESS)
               {
                module_state = 3;
               }

             if((keyIndex >=0) && (keyStatus<=9))
              {
              userToken= userToken*10+n(keyIndex);
              if(userToken == password)
              {
                Serial.println("qweas");
              }
              }
              Serial.println("qweas");
               
         break;
              
            }
            case 3:{
              Serial.println("aihfjkasnf");
              init_module8_clock = false;
              init_module9_clock = false;
              // userToken is zeroed      
              init_module10_clock = false;
                    init_module6_clock = false;
              // this is the open state
              // release all modules and let them run
              // draw the key grid on the oled. you may use |, -, + and space too
  
              // release the breaks
              // move to state 4
            }
            case 4: {
              // wait for *h+#h event, if so shut down and go to state 0
            }
            default: module_state=0;
          }
        }
    }

    { // module 6
      static unsigned long module_time, module_delay;
      static bool module_doStep;
      static unsigned char module_state=0; // state variable for module 6
          
        if (init_module6_clock) {
            module_delay = 300; 
            module_time=millis();
            module_doStep=false;
            init_module6_clock=false;
            module_state = 0;
        }
        else {
            unsigned long m;
            m=millis();
            if (m - module_time > module_delay) { // can you make this roll-over safe?
                module_time = m; module_doStep = true;
            }
            else module_doStep = false;
        }

        if (module_doStep) {
          //analogRead(trimPot) and calculate the revPsec variable for the revolutions per second  
        // trimpotValue = revPsec;
         trimpotValue = analogRead(trimPot);    
          // add a bit of hysteresis to the reading of the trimPot and check on the sensor pins 
          revPsec = 16.0/1023.0 * trimpotValue - 8;
               if(analogRead(sen0) == 0)
               {
                trimpotValue = 0.0;
                revPsec = 0.0;
                display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
                display.setCursor(70,50);
                display.print("motor off");
                 display.display();
               }
              else if(analogRead(sen1) == 0)
               {
                trimpotValue = 0.0;
                revPsec = 0.0;
                display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
                display.setCursor(70,50);
                display.print("motor off");
                 display.display();
               }
                 else
               {
                                display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
                display.setCursor(70,50);
                display.print("motor ok ");
                 display.display();
               }

   
               
          Serial.println(revPsec);
display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
display.setCursor(80,30);
display.print(revPsec);
display.display();
          // sen0 and sen1 to see if the motor is ok#
          
        }
    }

    { // module 7
      static unsigned long module_time, module_delay;
      static bool module_doStep;
      static unsigned char module_state=0; // state variable for module 7
      static byte s0=1, s1=1;
          
          if (init_module7_clock) {
            module_delay = 20; 
            module_time=millis();
            module_doStep=false;
            init_module7_clock=false;
            module_state = 0;
        }
        else {
            unsigned long m;
            m=millis();
            if (m - module_time > module_delay) { // can you make this roll-over safe?
                module_time = m; module_doStep = true;
            }
            else module_doStep = false;
        }

        if (module_doStep) {
          // analogRead(sen0) and analogRead(sen1) to see if the motor is ok

          
          // if the motor is off stop the trimpot module and assign 0.0 to the revPsec to stop the motor
          // else show "motor ok" and the revPsec on the oled
        }
    }

    { // module 8
      static unsigned long module_time, module_delay;
      static bool module_doStep;
      static unsigned char module_state=0, temp = 0; // state variable for module 10
      bool qwerty;
    
          if (init_module8_clock) {
            module_delay = 100; 
            module_time=millis();
            module_doStep=false;
            init_module8_clock=false;
            module_state = 0;
            temp = 0;
        }
        else {
            unsigned long m;
            m=millis();
            if (m - module_time > module_delay) { // can you make this roll-over safe?
                module_time = m; module_doStep = true;
            }
            else module_doStep = false;
            //submittting
        }

        if (module_doStep) {
          // this is the accelerometer. you know how to deal with it.
          // how do you display degC on the oled? remember, it is a graphical display
         switch(module_state)
         {
case 0:
display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
display.setCursor(80,40);
display.print(display_or);
display.display();
                display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
                display.setCursor(80,10);
                display.print(T);
                display.display();

          Wire.beginTransmission(MPU_6050);
          Wire.write(ACCEL_XOUT_HI);
          Wire.endTransmission(false);
          // Attempt to read 2 consecutive bytes from the MPU6050.
          // From above, these will be the MSB of the Z-component
          // of acceleration followed by its LSB.
          Wire.requestFrom(MPU_6050, 8 ,true); // read 2 registers
          AccX = Wire.read() << 8;
          AccX |= Wire.read();
          AccY = Wire.read() << 8;
          AccY |= Wire.read();        
          AccZ = Wire.read() << 8;
          AccZ |= Wire.read();
          TEMP_OUT = Wire.read()<<8|Wire.read(); 
          T = (TEMP_OUT/340.00) + 36.53;
          
         // char orienaion;
            if (AccZ > 14500)
            {
                       display_or = "Face Up";
                //Wire.write(orientation = 'AccZ');
            }
            else if (AccZ < -15500)
            {
                       display_or = "Upside D";
                //Wire.write(orientation = 'AccZ');
            }
            
           else if (AccY > 15000)
            {
                  //Serial.println("U");
               
        display_or = "Left   ";
                   
            }
              else if (AccY < -15000)
            {
                  //Serial.println("U");
               
        display_or = "Right   ";
                   
            }
            else if (AccX < -15000)
            {
              display_or = "Landscape";
                   
            }
            else if (AccX > 15000)
            {
              display_or = "Back 2f";
                   
            }
            
  
 break;
 default: module_state = 0;
 
          }
        }
    }
        
  

  
    { //module 9
      static unsigned long module_time, module_delay, debounce_count;
      static bool module_doStep, B1_state, B2_state;;
      static unsigned long module_keypadDebounceTime[12];
      unsigned long one, three, nine;
      static unsigned char state1=0, state2=0, state3=0, state4=0, state5=0, state6=0, state7=0, state8=0, state9=0,  stateS=0, state0=0, stateH=0; // state variable for module 9
    
          if (init_module9_clock) {
            module_delay = 10;
            module_time=millis();
            module_doStep=false;
            init_module9_clock=false;
            state1 = state2 = state3 = state4= state5=  state6=  state7 =  state8 = state9 = stateS = state0 = stateH =    0;

        }
        else {
            unsigned long m;
            m=millis();
            if (m - module_time > module_delay) { // can you make this roll-over safe?
                module_time = m; module_doStep = true;
            }
            else module_doStep = false;
        }

        if (module_doStep) {
                    oled(0, 2, F("---+--+---"));
          oled(0, 4, F("---+--+---"));
          oled(0, 6, F("---+--+---"));
          oled(3, 1, F("|"));
          oled(6, 1, F("|"));
          oled(3, 3, F("|"));
          oled(6, 3, F("|"));
          oled(3, 5, F("|"));
          oled(6, 5, F("|"));
          oled(3, 7, F("|"));
          oled(6, 7, F("|"));
          // this is my keypad module. it has 12 FSMs running it it as threads
          // chacking on the keys and updating the keyStatus[] array
          // this can be done in 4 modules of 3 threads each, i.e.
          // one module per row which will improve performance
        testRow(0);
        switch(state1)
        {
          case 0:
            keyStatus[1]=notPRESSED;
            if (testColumn(0)) state1 = 0;
            else {
              module_keypadDebounceTime[1] = module_time;
              state1 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[1];
          if (one < 300)                    keyStatus[1] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[1] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[1] = heldPRESS;
          else                            keyStatus[1] = stuck;

          if (testColumn(0)) state1 = 0;
                else state1 = 1;
          break;
          default: state1 = 0;
        }
        testRow(0);
         switch(state2)
        {
          case 0:       
            keyStatus[2]=notPRESSED;
            if (testColumn(1)) state2 = 0;
            else {
              module_keypadDebounceTime[2] = module_time;
              state2 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[2];
          if (one < 300)                    keyStatus[2] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[2] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[2] = heldPRESS;
          else                            keyStatus[2] = stuck;

          if (testColumn(1)) state2 = 0;
                else state2 = 1;
          break;
          default: state2 = 0;
        }
        testRow(0);
        switch(state3)
        {
          case 0:
            
            keyStatus[3]=notPRESSED;
            if (testColumn(2)) state3 = 0;
            else {
              module_keypadDebounceTime[3] = module_time;
              state3 = 1;
            }
          break;
          case 1:
          three = millis()-module_keypadDebounceTime[3];
          if (three < 300)                    keyStatus[3] = partialPRESS;
          else if (300 <= three && three < 1500)  keyStatus[3] = normalPRESS;
          else if (1500 <= three && three < 5000) keyStatus[3] = heldPRESS;
          else                            keyStatus[3] = stuck;

          if (testColumn(2)) state3 = 0;
                else state3 = 1;
          break;
          default: state3 = 0;
        }
     //   Serial.println(keyStatus[3]);
                 testRow(1);
            switch(state4)
        {
          case 0:

            keyStatus[4]=notPRESSED;
            if (testColumn(0)) state4 = 0;
            else {
              module_keypadDebounceTime[4] = module_time;
              state4 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[4];
          if (one < 300)                    keyStatus[4] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[4] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[4] = heldPRESS;
          else                            keyStatus[4] = stuck;

          if (testColumn(0)) state4 = 0;
                else state4 = 1;
          break;
          default: state4 = 0;
        }
                    testRow(1);
            switch(state5)
        {
          case 0:

            keyStatus[5]=notPRESSED;
            if (testColumn(1)) state5 = 0;
            else {
              module_keypadDebounceTime[5] = module_time;
              state5 = 1;
            }
          break;
                   case 1:
          one = millis()-module_keypadDebounceTime[5];
          if (one < 300)                    keyStatus[5] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[5] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[5] = heldPRESS;
          else                            keyStatus[5] = stuck;

          if (testColumn(1)) state5 = 0;
                else state5 = 1;
          break;
          default: state5 = 0;
        }
                    testRow(1);
                    switch(state6)
        {
          case 0:

            keyStatus[6]=notPRESSED;
            if (testColumn(2)) state6 = 0;
            else {
              module_keypadDebounceTime[6] = module_time;
              state6 = 1;
            }
          break;
                    case 1:
          one = millis()-module_keypadDebounceTime[6];
          if (one < 300)                    keyStatus[6] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[6] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[6] = heldPRESS;
          else                            keyStatus[6] = stuck;

          if (testColumn(2)) state6 = 0;
                else state6 = 1;
          break;
          default: state6 = 0;
        }
                    testRow(2);
                    switch(state7)
        {
          case 0:

            keyStatus[7]=notPRESSED;
            if (testColumn(0)) state7 = 0;
            else {
              module_keypadDebounceTime[7] = module_time;
              state7 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[7];
          if (one < 300)                    keyStatus[7] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[7] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[7] = heldPRESS;
          else                            keyStatus[7] = stuck;

          if (testColumn(0)) state7 = 0;
                else state7 = 1;
          break;
          default: state7 = 0;
        }
                    testRow(2);
                    switch(state8)
        {
          case 0:

            keyStatus[8]=notPRESSED;
            if (testColumn(1)) state8 = 0;
            else {
              module_keypadDebounceTime[8] = module_time;
              state8 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[8];
          if (one < 300)                    keyStatus[8] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[8] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[8] = heldPRESS;
          else                            keyStatus[8] = stuck;

          if (testColumn(1)) state8 = 0;
                else state8 = 1;
          break;
          default: state9 = 0;
        }
                    testRow(2);
         switch(state9)
        {
          case 0:

            keyStatus[9]=notPRESSED;
            if (testColumn(2)) state9 = 0;
            else {
              module_keypadDebounceTime[9] = module_time;
              state9 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[9];
          if (one < 300)                    keyStatus[9] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[9] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[9] = heldPRESS;
          else                            keyStatus[9] = stuck;

          if (testColumn(2)) state9 = 0;
                else state9 = 1;
          break;
          default: state9 = 0;
        }
                         testRow(3);
        switch(stateS)
        {
          case 0:

            keyStatus[10]=notPRESSED;
            if (testColumn(0)) stateS = 0;
            else {
            module_keypadDebounceTime[10] = module_time;
            stateS = 1;
            }
          break;
         case 1:
          one = millis()-module_keypadDebounceTime[10];
          if (one < 300)                    keyStatus[10] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[10] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[10] = heldPRESS;
          else                            keyStatus[10] = stuck;

          if (testColumn(0)) stateS = 0;
           else stateS = 1;
          break;
          default: stateS = 0;
          
        }
                    testRow(3); 
        switch(state0)
        {
          case 0:

            keyStatus[0]=notPRESSED;
            if (testColumn(1)) state0 = 0;
            else {
              module_keypadDebounceTime[0] = module_time;
              state0 = 1;
            }
          break;
          case 1:
          one = millis()-module_keypadDebounceTime[0];
          if (one < 300)                    keyStatus[0] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[0] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[0] = heldPRESS;
          else                            keyStatus[0] = stuck;

          if (testColumn(1)) state0 = 0;
                else state0 = 1;
          break;
          default: state0 = 0;
        }
         testRow(3);                    
        switch(stateH)
        {
          case 0:

            keyStatus[11]=notPRESSED;
            if (testColumn(2)) stateH = 0;
            else {
              module_keypadDebounceTime[11] = module_time;
              stateH = 1;
            }
          break;
           case 1:
          nine = millis()-module_keypadDebounceTime[11];
          if (one < 300)                    keyStatus[11] = partialPRESS;
          else if (300 <= one && one < 1500)  keyStatus[11] = normalPRESS;
          else if (1500 <= one && one < 5000) keyStatus[11] = heldPRESS;
          else                            keyStatus[11] = stuck;

          if (testColumn(2)) stateH = 0;
                else stateH = 1;
          break;
          default: stateH = 0;
        }
    }
        

    { // module 10
      static unsigned long module_time, module_delay;
      static bool module_doStep;
      static unsigned char module_state=0; // state variable for module 10
      static unsigned char state1=0, state2=0, state3=0, state4=0, state5=0, state6=0, state7=0, state8=0, state9=0,  stateS=0, state0=0, stateH=0;
    
          if (init_module10_clock) {
            module_delay = 101; 
            module_time=millis();
            module_doStep=false;
            init_module10_clock=false;
            module_state = 0;
            state1 = state2 = state3 = state4= state5=  state6=  state7 =  state8 = state9 = stateS = state0 = stateH =    0;
        }
        else {
            unsigned long m;
            m=millis();
            if (m - module_time > module_delay) { // can you make this roll-over safe?
                module_time = m; module_doStep = true;
            }
            else module_doStep = false;
        }

        if (module_doStep) {
          // this is my key displayer module
          // it too runs 12 threads one per key
          // again, this can be run as 4 modules of 3 threads each similar to the 
          // keypad module 9
         switch(keyStatus[1])
        {
          case partialPRESS:
            oled(1, 1, F("P"));
//           pressed(0);
          break;
          case normalPRESS:
            oled(1, 1, F("N"));          
          break;
          case heldPRESS:
           oled(1, 1, F("H"));
          break;
           case stuck:
            oled(1, 1, F("S"));
          break;
          default: oled(1, 1, F(" "));
        }
        //Serial.println(keyStatus[2], DEC);
         switch(keyStatus[2])
        {
          case partialPRESS:
            oled(5, 1, F("P"));
          break;
          case normalPRESS:
            oled(5, 1, F("N"));          
          break;
          case heldPRESS:
           oled(5, 1, F("H"));
          break;
           case stuck:
            oled(5, 1, F("S"));
          break;
          default: oled(5, 1, F(" "));
        }
                 switch(keyStatus[3])
        {
          case partialPRESS:
            oled(8, 1, F("P"));
          break;
          case normalPRESS:
            oled(8, 1, F("N"));          
          break;
          case heldPRESS:
           oled(8, 1, F("H"));
          break;
           case stuck:
            oled(8, 1, F("S"));
          break;
          default: oled(8, 1, F(" "));
        }
         switch(keyStatus[4])
        {
          case partialPRESS:
            oled(1,3, F("P"));
          break;
          case normalPRESS:
            oled(1, 3, F("N"));          
          break;
          case heldPRESS:
           oled(1, 3, F("H"));
          break;
           case stuck:
            oled(1, 3, F("S"));
          break;
          default: oled(1, 3, F(" "));
        }
         switch(keyStatus[5])
        {
          case partialPRESS:
            oled(5,3, F("P"));
          break;
          case normalPRESS:
            oled(5, 3, F("N"));          
          break;
          case heldPRESS:
           oled(5, 3, F("H"));
          break;
           case stuck:
            oled(5, 3, F("S"));
          break;
          default: oled(5, 3, F(" "));
        }
                 switch(keyStatus[6])
        {
          case partialPRESS:
            oled(8,3, F("P"));
          break;
          case normalPRESS:
            oled(8, 3, F("N"));          
          break;
          case heldPRESS:
           oled(8, 3, F("H"));
          break;
           case stuck:
            oled(8, 3, F("S"));
          break;
          default: oled(8 , 3, F(" "));
        }
                 switch(keyStatus[7])
        {
          case partialPRESS:
            oled(1, 5, F("P"));
          break;
          case normalPRESS:
            oled(1, 5, F("N"));          
          break;
          case heldPRESS:
           oled(1, 5, F("H"));
          break;
           case stuck:
            oled(1, 5, F("S"));
          break;
          default: oled(1, 5, F(" "));
        }
         switch(keyStatus[8])
        {
          case partialPRESS:
            oled(5, 5, F("P"));
          break;
          case normalPRESS:
            oled(5, 5, F("N"));          
          break;
          case heldPRESS:
           oled(5, 5, F("H"));
          break;
           case stuck:
            oled(5, 5, F("S"));
          break;
          default: oled(5, 5, F(" "));
        }
                 switch(keyStatus[9])
        {
          case partialPRESS:
            oled(8,5, F("P"));
          break;
          case normalPRESS:
            oled(8, 5, F("N"));          
          break;
          case heldPRESS:
           oled(8, 5, F("H"));
          break;
           case stuck:
            oled(8,5, F("S"));
          break;
          default: oled(8,5, F(" "));
        }
                 switch(keyStatus[10])
        {
          case partialPRESS:
            oled(1, 7, F("P"));
          break;
          case normalPRESS:
            oled(1, 7, F("N"));          
          break;
          case heldPRESS:
           oled(1, 7, F("H"));
          break;
           case stuck:
            oled(1, 7, F("S"));
          break;
          default: oled(1, 7, F(" "));
        }
         switch(keyStatus[0])
        {
          case partialPRESS:
            oled(5, 7, F("P"));
          break;
          case normalPRESS:
            oled(5,7, F("N"));          
          break;
          case heldPRESS:
           oled(5, 7, F("H"));
          break;
           case stuck:
            oled(5, 7, F("S"));
          break;
          default: oled(5, 7, F(" "));
        }
        switch(keyStatus[11])
        {
          case partialPRESS:
            oled(8, 7, F("P"));
          break;
          case normalPRESS:
            oled(8, 7, F("N"));          
          break;
          case heldPRESS:
           oled(8, 7, F("H"));
          break;
           case stuck:
            oled(8, 7, F("S"));
          break;
          default: oled(8, 7, F(" "));
        }
        }
    }
}

}
    
