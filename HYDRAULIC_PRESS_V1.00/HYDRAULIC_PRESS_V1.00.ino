/*
 * 
 * 
 * 
 */


#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>


unsigned long t1, t2, t3, t4;

char key;
byte keypadState;
int ledPin = 13;
char input[3];
int keyPressCount = 0;
int blockHeight = 240;
char lastKeyPressed;
int pressureMeasurementCount = 0;
float averagePressure = 0;
float averageBar = 0;
uint16_t timeOfFlight;

int sensorVal;
float voltage;
float averageVoltage = 0;

float pressure_pascal;
float pressure_bar;
float pressure_psi;

int bottom_up_start_position;
int bottom_up_first_press_position;
int bottom_up_final_press_position;
int bottom_up_max;

const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 24, 26, 28};
byte colPins[COLS] = {23, 25, 27, 29}; 

char keyBuffer[10]; // buffer to hold ascii key value

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
//LiquidCrystal lcd(6,7,8,9,10,11);//(rs,e,d4,d5,d6,d7)
LiquidCrystal_I2C lcd(0x27, 20, 4);

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measure;

#define TOP_DOWN_LIMIT_SWITCH_PIN A13
#define LEFT_RIGHT_LIMIT_SWITCH_PIN A15
#define PRESSURE_SWITCH_PIN A0

#define BOTTOM_UP_PUSH_RELAY_PIN 31 // bottom-up extend
#define BOTTOM_UP_PULL_RELAY_PIN 33 // bottom-up retract
#define TOP_DOWN_PUSH_RELAY_PIN 35  // top-down extend
#define TOP_DOWN_PULL_RELAY_PIN 37  // top-down retract
#define LEFT_RIGHT_PUSH_RELAY_PIN 39  // left-right extend
#define LEFT_RIGHT_PULL_RELAY_PIN 41  // left-right retract
#define CYLINDER_1_PUSH_RELAY_PIN 43  
#define CYLINDER_1_PULL_RELAY_PIN 45
#define CYLINDER_2_PUSH_RELAY_PIN 47
#define CYLINDER_2_PULL_RELAY_PIN 49
#define CYLINDER_3_PUSH_RELAY_PIN 51
#define CYLINDER_3_PULL_RELAY_PIN 53

#define ACTUATOR_12V_PUSH_RELAY_PIN 43
#define ACTUATOR_12V_PULL_RELAY_PIN 45
#define ACTUATOR_24V_PUSH_RELAY_PIN 47
#define ACTUATOR_24V_PULL_RELAY_PIN 49

#define LED_REFRESH_RATE   1000  // main screen refresh rate in mS
#define LOX_REFRESH_RATE   10

#define RELAY_ON LOW
#define RELAY_OFF HIGH

#define KEYPAD_PAUSE 500
#define VL53L0X_PAUSE 100

// Automatic Sequence Steps
#define _CLOSE_TOP          1
#define _CLOSE_MOLD         2
#define _MID_BOTTOM_PRESS   3
#define _MAX_TOP_PRESS      4
#define _MAX_BOTTOM_PRESS   5
#define _RETRACT_TOP_PRESS  6
#define _OPEN_TOP           7
#define _PUSH_OUT_BLOCK     8
#define _PUSH_BLOCK_AWAY    9
#define _RETRACT_BOTTOM_PRESS 10
#define _BOTTOM_UP_START    11
#define _LOAD_SAND          12



void setup(){

  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  
 lcd.init();
  lcd.backlight();

  //keypad.addEventListener(keypadEvent); // Add an event listener for this keypad


  pinMode(TOP_DOWN_LIMIT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(LEFT_RIGHT_LIMIT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(PRESSURE_SWITCH_PIN, INPUT);

  pinMode(BOTTOM_UP_PUSH_RELAY_PIN, OUTPUT);
  pinMode(BOTTOM_UP_PULL_RELAY_PIN, OUTPUT);
  pinMode(TOP_DOWN_PUSH_RELAY_PIN, OUTPUT);
  pinMode(TOP_DOWN_PULL_RELAY_PIN, OUTPUT);
  pinMode(LEFT_RIGHT_PUSH_RELAY_PIN, OUTPUT);
  pinMode(LEFT_RIGHT_PULL_RELAY_PIN, OUTPUT);
  pinMode(CYLINDER_1_PUSH_RELAY_PIN, OUTPUT);
  pinMode(CYLINDER_1_PULL_RELAY_PIN, OUTPUT);
  pinMode(CYLINDER_2_PUSH_RELAY_PIN, OUTPUT);
  pinMode(CYLINDER_2_PULL_RELAY_PIN, OUTPUT);
  pinMode(CYLINDER_3_PUSH_RELAY_PIN, OUTPUT);
  pinMode(CYLINDER_3_PULL_RELAY_PIN, OUTPUT);

  digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(TOP_DOWN_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(TOP_DOWN_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(LEFT_RIGHT_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_1_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_1_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_2_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_2_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_3_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_3_PULL_RELAY_PIN, RELAY_OFF);

  
  if (!lox.begin()) {
    lcd.clear();
    lcd.print("VL53L0X Failed");
    delay(3000);
    lcd.clear();
  }

  showInfo();
  showDistance();

  update_tofVariables(blockHeight);

  t1 = millis();
  t4 = millis();

}//setup()

void showInfo()
{
  lcd.clear();
  lcd.setCursor(0,0);
  //lcd.print("Height=240mm");
  lcd.print("Height = ");
  lcd.print((String)blockHeight);
  lcd.print(" mm");
}//showInfo

void showDistance()
{
  lcd.setCursor(0,3);
  
  //lox.rangingTest(&measure, false); 
    
  if (measure.RangeStatus != 4)
  { 
    lcd.print("                    ");
    lcd.setCursor(0,3);
    lcd.print((String)timeOfFlight);
    lcd.print(" mm");
  }
  else
  {
    lcd.print("                    ");
    lcd.setCursor(0,3);
    lcd.print("Range out of bounds");
    //lcd.print("    ");
  }
}

void stopAutomaticSequence(){

  digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(TOP_DOWN_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(TOP_DOWN_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(LEFT_RIGHT_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_1_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_1_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_2_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_2_PULL_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_3_PUSH_RELAY_PIN, RELAY_OFF);
  digitalWrite(CYLINDER_3_PULL_RELAY_PIN, RELAY_OFF);
  
  showInfo();
}

//////
void loop(){
   
  key = keypad.getKey();
  keypadState = keypad.getState();
  
  if(key == '*' /*&& keypadState == PRESSED*/ )
  {
    startAutomaticSequence();
  }
  
  if(key == '1' /*&& keypadState == PRESSED*/ )
  {  
    close_top();
  }

  if(key == '2' /*&& keypadState == PRESSED*/ )
  {  
    close_mold();
  }

  if(key == '3' /*&& keypadState == PRESSED*/ )
  {  
    mid_bottomPress();
  }

  if(key == '4' /*&& keypadState == PRESSED*/ )
  {  
    max_topPress();
  }

  if(key == '5' /*&& keypadState == PRESSED*/ )
  {  
    max_bottomPress();
  }

  if(key == '6' /*&& keypadState == PRESSED*/ )
  {  
    retract_topPress();
  }

  if(key == '7' /*&& keypadState == PRESSED*/ )
  {  
    open_top();
  }

  if(key == '8' /*&& keypadState == PRESSED*/ )
  {  
    push_outBlock();
  }

  if(key == '9' && keypadState == PRESSED)
  {  
    push_blockAway();
  }

  if(key == '0' /*&& keypadState == PRESSED*/ )
  {  
    retract_bottomPress();
  }

  if(key == 'A' /*&& keypadState == PRESSED*/ )
  {  
    bottom_startPosition();
  }

  if(key == 'B' /*&& keypadState == PRESSED*/ )
  {  
    load_sand();
  }

  if (key == 'D' /*&& keypadState == PRESSED*/ )
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Input block height:");
    lcd.setCursor(0,3);
    lcd.print("D: Confirm C: Clear");
    
    lcd.setCursor(0,1);
    
    keypadState = -1;

    lastKeyPressed = key;
    
    do
    {
      key = keypad.getKey();
      keypadState = keypad.getState();

      if( key == 'D' )
      {
        if( blockHeight < 50 || blockHeight > 270 )
        {
          lcd.setCursor(0,2);
          lcd.print("Invalid input.");
          
          keyPressCount = 0;
          
          delay(1000);
          
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Input block height:");
          lcd.setCursor(0,3);
          lcd.print("Press D to confirm.");
          lcd.setCursor(0,1);
        }
        else
          break;
      }

      if( key == 'C' )
      {
          keyPressCount = 0;
          lcd.setCursor(0,1);
          lcd.print("   ");
          lcd.setCursor(0,1);
      }

      if( key != lastKeyPressed )
      {
        lastKeyPressed = key;
        
        if( key >= '0' && key < 'A' )
        {
          keyPressCount++;

          if( keyPressCount > 3 )
          {
            keyPressCount = 0;
            
            for(int i = 0; i < 3; i++)
            {
              input[i] = 0;
            }
            
            lcd.setCursor(0,1);
            lcd.print("Invalid Input.");
            delay(1000);
            lcd.setCursor(0,1);
            lcd.print("                    ");
            lcd.setCursor(0,1);
          }

          switch( keyPressCount )
          {
            case 0: break;
            case 1: input[2] = key;
                    blockHeight = input[2] - '0';
                    lcd.print((String)key);
                    break;
                    
            case 2: 
                    input[1] = input[2];
                    input[2] = key;
                    blockHeight = ((input[1] - '0') * 10) + ((input[2] - '0') * 1);
                    lcd.print((String)key);
                    break;
                  
            case 3:
                    input[0] = input[1];
                    input[1] = input[2];
                    input[2] = key;
                    blockHeight = ((input[0] - '0') * 100) + ((input[1] - '0') * 10) + ((input[2] - '0') * 1);
                    lcd.print((String)key);
                    break;
          }
          
          update_tofVariables(blockHeight);

          /*if( keyPressCount == 3 )
          {
            keyPressCount = 0;
          }*/
        
        }
        
      }
    }while(1);
      
      /*digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_ON);
      while(keypadState != RELEASED)
      {
        key = keypad.getKey();
        keypadState = keypad.getState();
        itoa(key, keyBuffer, 10);
        lcd.print(keyBuffer);  
      }
      lcd.setCursor(0,1);
      lcd.print("released A"); 
      digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_OFF);
      delay(KEYPAD_PAUSE);
      lcd.clear();*/

      lcd.clear();
  }


  t2 = millis();
  t3 = millis();
  
  if( t2 - t1 > LED_REFRESH_RATE )
  {
    showInfo();
    showDistance();
    
    t1 = millis();  // reset counter
  }

  if( t3 - t4 > LOX_REFRESH_RATE )
  {
    timeOfFlight = read_timeOfFlight();
    
    t4 = millis();  // reset counter
  }
   
}//loop()

/////// Functions ///////////////
void close_top(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Close Top");

  key = keypad.getKey();
  
  while( !check_leftRightSensor() && key != '#' )
  {
    digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_ON);  // retract

    key = keypad.getKey();
  }
  
  digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_OFF);
}

void bottom_startPosition(void)
{
  uint16_t tofValue;
  
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Bottom start");

  tofValue = read_timeOfFlight();
  
  //showDistance();
  t3 = millis();

  do
  {
    if( tofValue >= bottom_up_start_position )
    {
      digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_ON);  // extend
    }
    else if( tofValue < bottom_up_start_position )
    {
      digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);  // extend
      
      while( read_pressureSwitch() < 512 )
      {
        digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_ON);  // retract

        //t4 = millis();
        //if( t4 - t3 > 100 )
        //{
          //tofValue = read_timeOfFlight();
          
          //showDistance();
          //t3 = millis();
        //}
       }
      
      digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_OFF);  // retract
    }
    else if( tofValue == bottom_up_start_position )
    {
      digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);
      digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_OFF);  // extend
      break;
    }

    tofValue = read_timeOfFlight();

    //t4 = millis();
    //if( t4 - t3 > LOX_REFRESH_RATE )
    //{
      //showDistance();
      //t3 = millis();
    //}
    
  }while( tofValue != bottom_up_start_position );
}

void load_sand(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Load Sand");

  key = keypad.getKey();
  
  while( read_pressureSwitch() < 512 && key != '#' )
  {
    digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }

  digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_OFF);
}

void close_mold(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Close mold");

  key = keypad.getKey();
  
  while( !check_topDownSensor() && key != '#' )
  {
    digitalWrite(TOP_DOWN_PUSH_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }
  
  digitalWrite(TOP_DOWN_PUSH_RELAY_PIN, RELAY_OFF);
}

void mid_bottomPress(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Mid bottom press");

  key = keypad.getKey();
  
  while( read_timeOfFlight() <= bottom_up_first_press_position && key != '#' )
  {
    digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }
  
  digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);
}

void max_bottomPress(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Max bottom press");

  key = keypad.getKey();

  while( read_timeOfFlight() <= bottom_up_final_press_position && key != '#' )
  {
    digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }
  
  digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);
}


void max_topPress(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Max top press");

  key = keypad.getKey();

  while( read_pressureSwitch() < 512 && key != '#' )
  {
    digitalWrite(TOP_DOWN_PUSH_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }
  
  digitalWrite(TOP_DOWN_PUSH_RELAY_PIN, RELAY_OFF);
}

void retract_topPress(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Retract top press");

  key = keypad.getKey();

  while( read_pressureSwitch() < 512 && key != '#' )
  {
    digitalWrite(TOP_DOWN_PULL_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }
  
  digitalWrite(TOP_DOWN_PULL_RELAY_PIN, RELAY_OFF);
}

void open_top(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Open Top");

  key = keypad.getKey();
  
  while( read_pressureSwitch() < 512 && key != '#' )
  {
    digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_ON);  // retract

    key = keypad.getKey();
  }
  
  digitalWrite(LEFT_RIGHT_PULL_RELAY_PIN, RELAY_OFF);
}

void push_outBlock(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Push out block");

  key = keypad.getKey();

  while( read_timeOfFlight() <= bottom_up_max && key != '#' )
  {
    digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }

  digitalWrite(BOTTOM_UP_PUSH_RELAY_PIN, RELAY_OFF);  // extend
}

void push_blockAway(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Push block away");

  key = keypad.getKey();
  
  while( !check_leftRightSensor() && key != '#' )
  {
    digitalWrite(LEFT_RIGHT_PUSH_RELAY_PIN, RELAY_ON);  // extend

    key = keypad.getKey();
  }
  
  digitalWrite(LEFT_RIGHT_PUSH_RELAY_PIN, RELAY_OFF);
}

void retract_bottomPress(void)
{
  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("Retract bottom press");

  key = keypad.getKey();
  
  while( !check_leftRightSensor() && key != '#' )
  {
    digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_ON);  // retract

    key = keypad.getKey();
  }
  
  digitalWrite(BOTTOM_UP_PULL_RELAY_PIN, RELAY_OFF);
}

// Returns 1 if IR beam is broken
unsigned char check_leftRightSensor(void)
{
  int irSensor2;

  irSensor2 = analogRead(LEFT_RIGHT_LIMIT_SWITCH_PIN);

  if( irSensor2 < 512 ) // IR beam present
    return 0;
  else if( irSensor2 >= 512 ) // IR beam not present
    return 1; 
}

// Returns 1 if IR beam is broken
unsigned char check_topDownSensor(void)
{
  int irSensor;
  
  irSensor = analogRead(TOP_DOWN_LIMIT_SWITCH_PIN);
  
  if( irSensor < 512 ) // IR beam present
    return 1;
  else if( irSensor >= 512 ) // IR beam not present
    return 0; 
}

uint16_t read_timeOfFlight(void)
{
  lox.rangingTest(&measure, false);
    
  if (measure.RangeStatus != 4)
    return measure.RangeMilliMeter;
  else
    return 0;
}

void update_tofVariables(int height)
{
  bottom_up_start_position = 600 - (height * 1.7);
  bottom_up_first_press_position = 600 - (height * 1.2);
  bottom_up_final_press_position = 600 - height;
}

int read_pressureSwitch(void)
{
  int input;
  
  input = analogRead(PRESSURE_SWITCH_PIN);

  return input;
}

void load_pallet(void)
{

  digitalWrite(ACTUATOR_12V_PUSH_RELAY_PIN, RELAY_ON);  // Push pallet 1cm
  delay(2000);
  digitalWrite(ACTUATOR_12V_PUSH_RELAY_PIN, RELAY_OFF);

  digitalWrite(ACTUATOR_24V_PUSH_RELAY_PIN, RELAY_ON);  // Push pallet to next loading position
  delay(3200);
  digitalWrite(ACTUATOR_24V_PUSH_RELAY_PIN, RELAY_OFF);

  digitalWrite(ACTUATOR_12V_PULL_RELAY_PIN, RELAY_ON);  // Push pallet to next loading position
  delay(2000);
  digitalWrite(ACTUATOR_12V_PULL_RELAY_PIN, RELAY_OFF);

  // Have to include function for pallet switch reading
}

void doStep(unsigned char step)
{
  switch(step)
  {
    case _CLOSE_TOP: close_top(); break;
    
    case _CLOSE_MOLD: close_mold(); break;
    
    case _MID_BOTTOM_PRESS: mid_bottomPress(); break;
    
    case _MAX_TOP_PRESS: max_topPress(); break;
    
    case _MAX_BOTTOM_PRESS: max_bottomPress(); break;
    
    case _RETRACT_TOP_PRESS: retract_topPress(); break;
    
    case _OPEN_TOP: open_top(); break;
    
    case _PUSH_OUT_BLOCK: push_outBlock(); break;
    
    case _PUSH_BLOCK_AWAY: push_blockAway(); break;
    
    case _RETRACT_BOTTOM_PRESS: retract_bottomPress(); break;
    
    case _BOTTOM_UP_START: bottom_startPosition(); break;
    
    case _LOAD_SAND: load_sand(); break;
    
    default: break;
  }
}

void startAutomaticSequence(void)
{
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Automatic Sequence");

  for(int j = 0; j < 2; j++)
  {
    for(int i = 0; i < 12; i++)
    {
      key = keypad.getKey();
    
      if( key == '#' )
      {
        stopAutomaticSequence();
        break;
      }
      else
      {
        doStep(i+1);
        //delay(1000);
      }
    }
  }

  //load_pallet();
}
