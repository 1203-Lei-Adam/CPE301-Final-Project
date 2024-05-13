////Nikhil Anil, Adam Lei, Sumiran Marsani
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Stepper.h>

#define RDA 0x80
#define TBE 0x20 

#define Type DHT11
int stepsPerRev = 2038;
Stepper myStepper = Stepper(stepsPerRev, 2, 3, 4 , 5);

int sensePin=13;
  DHT HT(sensePin,Type);
  float humidity;
  float temperature;
  int INTtemp;
  int INThumi;
  

// Define Port D Register Pointers						
volatile unsigned char* port_d = (unsigned char*) 0x2B;						
volatile unsigned char* ddr_d = (unsigned char*) 0x2A;						
volatile unsigned char* pin_d= (unsigned char*) 0x29;	

volatile unsigned char* port_k = (unsigned char*) 0x108; 
volatile unsigned char* ddr_k  = (unsigned char*) 0x107; 
volatile unsigned char* pin_k  = (unsigned char*) 0x106; 

//UART registers
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//ADC registers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

//Timers
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

LiquidCrystal lcd(6, 7, 8, 9, 10, 11); //creates lcd object - figure out pins rs,en,d4,d5,d6,d7

enum States {
  DISABLED,
  IDLE, 
  ERROR,
  RUNNING,
};

void setup(){
  U0init(9600);

  adc_init(); //initializes the ADC
  *ddr_k &= 0b11110011;
  *port_k |= 0b00001100;

  *ddr_d |= 0b00000011;	
  lcd.begin(16, 2); //starts the lcd

}


void loop(){
  humidity=HT.readHumidity();
  temperature=HT.readTemperature();

  //working with motor
  bool moveLeft = false, moveRight = false;
  
  unsigned char adc_channel = 0;
  unsigned int adc_value = adc_read(adc_channel);

  if(adc_value>=200){
    *port_d |= 0b00000001;

  }
  else{
    *port_d &= 0b11111110;

  }

  writeToLCD();
  if(*pin_k & 0b00000100){
    moveLeft = 1;
    moveRight = 0;
    
  }
  else{
    moveLeft = 0;
    moveRight = 1;
  }
  if(*pin_k & 0b00001000){
    *port_d |= 0b00000010;
  }
  else{
    *port_d &= 0b11111101;
  }
  if(moveLeft == true  || moveRight == true){
    moveVent(moveLeft, moveRight);
  }
}

void moveVent(bool left, bool right){
  if(left == true){
    myStepper.setSpeed(2);
    myStepper.step(stepsPerRev);
  }

  else if(right == true){
    myStepper.setSpeed(2);
    myStepper.step(-stepsPerRev);
  }
}

void writeToLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  if(temperature>0){
  INTtemp=temperature;
  }
  else if(temperature<0){
  INTtemp=-1*temperature;
  }
  else{
    INTtemp--;
  }
  lcd.print(INTtemp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  
  if(humidity<0){
  INThumi=-1*humidity;
  }
  if(humidity>100){
  INThumi=humidity-100;
  }
  else{
    INTtemp++;
  }
  lcd.print(INThumi);
  lcd.print("%");
}

//Start of UART functions
void U0init(unsigned long U0baud)
{
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
}

unsigned char U0kbhit()
{
  return (*myUCSR0A & RDA);
}

unsigned char U0getchar()
{
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A &= TBE) == 0){};
  *myUDR0 = U0pdata;
}
//End of UART functions

//Start of ADC functions
void adc_init()
{
  
  *my_ADCSRA |= 0b10000000; 
  *my_ADCSRA &= 0b11011111; 
  *my_ADCSRA &= 0b11110111; 
  *my_ADCSRA &= 0b11111000; 
  
  *my_ADCSRB &= 0b11110111; 
  *my_ADCSRB &= 0b11111000; 
  *my_ADMUX  &= 0b01111111; 
  *my_ADMUX  |= 0b01000000; 
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11100000; 
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  
  *my_ADMUX  &= 0b11100000;
  
  *my_ADCSRB &= 0b11110111;
  
  if(adc_channel_num > 7)
  {

    adc_channel_num -= 8;
  
    *my_ADCSRB |= 0b00001000;
  }
  
  *my_ADMUX  += adc_channel_num;

  *my_ADCSRA |= 0x40;
  
  while((*my_ADCSRA & 0x40) != 0);
  
  return *my_ADC_DATA;
}
//End of ADC functions

void my_delay(unsigned int freq)
{
  // calc period
  double period = 1.0/double(freq);
  // 50% duty cycle
  double half_period = period/ 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  // start the timer
  * myTCCR1B |= 0b00000001;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8;   // 0b 0000 0000
  // reset TOV           
  *myTIFR1 |= 0x01;
}
