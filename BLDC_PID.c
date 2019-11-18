#include <mega128.h>
#include <stdio.h>
#include <delay.h>

#define MOTOR2_DIRECTION PORTB.3
#define MOTOR1_DIRECTION PORTB.4

unsigned char usart_buf[128];
unsigned char RXC_BUFF[128];
unsigned char data_buf = 0;
unsigned char hall_sensor_value = 0;
unsigned int round = 0;
unsigned int second_cnt = 0;
unsigned int OCR_SET = 0;
unsigned char motorPWM = 0;
int OCR_val = 0;


void GPIO_SETUP(void)
{
    //PWM
    DDRB.6 = 1;
    DDRB.7 = 1;    

    //motor direction
    DDRB.4 = 1;
    DDRB.3 = 1;

    PORTB.4 = 0;
    PORTB.3 = 0;
    //nBrake;   
    DDRB.2 = 1;
    DDRB.5 = 1;
    
    PORTB.2 = 0;
    PORTB.5 = 0;
}

void USART0_init(void)
{
    UCSR0A = 0x00;
    UCSR0B = (1<<RXEN0)|(1<<RXCIE0);                           
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
    UCSR0C &= ~(0<<UMSEL0);

    UBRR0H = 0;
    UBRR0L = 95;
}

void USART1_init(void)
{
    UCSR1A = 0x00;
    UCSR1B = (1<<TXEN1)|(1<<RXEN0)|(1<<RXCIE0);                           
    UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);
    UCSR1C &= ~(0<<UMSEL1);

    UBRR1H = 0;
    UBRR1L = 95;
}


void Data_Tx1(unsigned char bData)
{
    while(!(UCSR1A & (1<<UDRE1))); //wait until data can be loaded.
    UDR1 = bData; //data load to TxD buffer
}

unsigned char Data_Rx0(void)
{
    while(!(UCSR0A & (1<<RXC0))); // RXC0 flag is USART Receive Complete
    return UDR0;
}


//sprintf printing function
void string_tx1(unsigned char *str)
{
    while (*str)
    {
        Data_Tx1(*str++);
    }
}

void EXT_INT_init(void)
{
    EICRA = 0x02;
    EIMSK = 0x01;
    DDRD.0 = 0;
}

void TIMER_init(void)                            
{
    //TIMER2
    TCCR2 = (1<<WGM21)|(1<<WGM20);
    TCCR2 |= (1<<CS20);
    OCR2 = 1;
    
    TCCR1A = (1<<WGM11)|(1<<COM1C1)|(1<<COM1B1);    TCCR1B = (1<<WGM13)|(1<<WGM12); // WGM bit setting
    TCCR1B |= (1<<CS10); // Clock source choie
    
    OCR1B = 0x00;
    OCR1CH = 0x00;
    OCR1CL = 0x00;
    ICR1 = 300; //664
    
    TIMSK = (1<<TOIE2);
}

void producePWM(void)
{
    if(OCR_val < 0)
    {
        MOTOR1_DIRECTION = 1;
        MOTOR2_DIRECTION = 1;
        OCR_SET = -(OCR_val)*(3);
        
        OCR1B = OCR_SET;
        OCR1CH = (OCR_SET & 0xFF00) >> 3;
        OCR1CL = 0x00FF & (OCR_SET);
    }
    else
    {
        MOTOR1_DIRECTION = 0;
        MOTOR2_DIRECTION = 0;
        OCR_SET = (OCR_val)*3;

        OCR1B = OCR_SET;
        OCR1CH = (OCR_SET & 0xFF00) >> 8;
        OCR1CL = 0x00FF & (OCR_SET);
    }
}

interrupt [TIM2_OVF] void timer2_overflow(void)
{
    second_cnt++;
    if(second_cnt == 228)
       second_cnt = 0;
}

interrupt [USART1_RXC]void int_USART1(void)
{
    data_buf = UDR1;

    if(data_buf == '1' && OCR_val < 100)
        OCR_val ++;
    else if(data_buf == '2' && OCR_val > -100)
        OCR_val --;
    else if(data_buf == '0')
        OCR_val = 0;
}

interrupt [EXT_INT0] void hall_sensor_detection(void)
{
    hall_sensor_value++;
    
    if(hall_sensor_value == 11)
    {
        hall_sensor_value = 0;
        round ++;
    } 
}

void main(void)
{
    USART1_init();
    TIMER_init();
    GPIO_SETUP();
    EXT_INT_init();

    #asm("sei")      
   
    while(1)
    {
        if(second_cnt == 227)
        {
            sprintf(usart_buf, "Motor Speed : %u km/h \r\n OCR2 : %u \r\n", (unsigned int)((round*3.14)/4), OCR2);
            string_tx1(usart_buf);
            sprintf(usart_buf, "HALL sensor data : %u \r\n round : %u \r\n", hall_sensor_value, round);
            string_tx1(usart_buf);
            round = 0;
        }
        
        producePWM();
    }
}