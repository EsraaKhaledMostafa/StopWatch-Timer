/*
 * Mini_Project2.c
 *
 *  Created on: Sep 14, 2023
 *      Author: Esraa Khaled
 */

/*LIBRARIES*/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* GLOBAL VARIABLES*/

// array of Time[6] = {sec0,sec1,min0,min1,hour0,hour1}
unsigned char Time[6] = {0}; //initialize with zeros

//flags to switch between timer and stop watch modes
unsigned char flag_timer = 1;
unsigned char flag_stopWatch = 1;
unsigned char countdown = 0;

//flags for buttons to set the time
unsigned char flag_sec0 = 1;
unsigned char flag_sec1 = 1;
unsigned char flag_min0 = 1;
unsigned char flag_min1 = 1;
unsigned char flag_hour0 = 1;
unsigned char flag_hour1 = 1;
unsigned char flag_setTime = 1;

/*FUNCTION PROTOTYPES*/
void GPIO_init(void);
void _7Segments_Display(void);
void Timer1_CTC_init(void);
void INT0_init(void);
void INT1_init(void);
void INT2_init(void);
void Set_Time(void);
void Set_Mode(void);

/*MAIN*/

int main(void)
{
	//initializing GPIO pins
	GPIO_init();

	//initializing Timer and interrupts
	Timer1_CTC_init();
	INT0_init();
	INT1_init();
	INT2_init();

	while(1)
	{
		_7Segments_Display();
		Set_Mode();
		Set_Time();
	}
}


/*FUNCTIONS*/

//Pins Initialization
void GPIO_init(void)
{
	DDRD &= 0xE0; //PD0 - PD3 --> i/p pins
	PORTD |= (1<<PD2);// activate internal pull up resistor for button at INT0

	DDRB = 0x80;  //PB0 - PB6 --> i/p pins & PB7 --> o/p pin
	PORTB |= (1<<PB2); //activating the internal pull up resistor for button at INT2

	DDRC |= 0x0F; //PC0 - PC3 --> o/p pins
	DDRA |= 0x3F; //PA0 - PA5 --> o/p pins

	SREG |= (1<<7); //enabling global interrupt
}

//Displaying Time on multiplexed 7 segments
void _7Segments_Display(void)
{
	unsigned char i = 0;
	//Multiplexed technique to power on all the 6 segments together
	while(i<6)
	{
		PORTA = (PORTA & 0xC0);
		PORTA |= (1<<i); //enable 1 segment at a time
		PORTC =  (PORTC & 0xF0) | (Time[i] & 0x0F); //displaying at each segment the corresponding digit of time
		_delay_us(1);
		i++;
	}
}

//TIMER
void Timer1_CTC_init(void)
{
	TCNT1 = 0; //initial value for counting
	OCR1A = 15625; //Top value for compare match to match and give interrupt every 1 sec

	TIMSK |= (1<<OCIE1A); //enabling the CTC module interrupt

	TCCR1A = (1<<FOC1A); //configuring non PWM mode
	TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS11); // choosing CTC mode and adjusting clock source for prescaler = 64
}

ISR(TIMER1_COMPA_vect)
{
	//Timer Mode ON
	if(countdown == 1)
	{
		//decrement the seconds
		if(Time[0] > 0)
		{
			Time[0]--;
		}
		else if(Time[1] > 0)
		{
			Time[1]--;
			Time[0] = 9;
		}
		//decrements the minutes if the seconds are 0
		else if(Time[2] > 0)
		{
			Time[2]--;
			Time[1] = 5;
			Time[0] = 9;
		}
		else if(Time[3] > 0)
		{
			Time[3]--;
			Time[2] = 9;
			Time[1] = 5;
			Time[0] = 9;
		}
		//decrements the hours if the minutes and seconds are 0
		else if(Time[4] > 0)
		{
			Time[4]--;
			Time[3] = 5;
			Time[2] = 9;
			Time[1] = 5;
			Time[0] = 9;
		}
		else if(Time[5] > 0)
		{
			Time[5]--;
			Time[4] = 9;
			Time[3] = 5;
			Time[2] = 9;
			Time[1] = 5;
			Time[0] = 9;
		}
		//turning off the timer and turning on the buzzer since time = 0
		else
		{
			TCCR1B &=  ~(1<<CS10) & ~(1<<CS11) & ~(1<<CS12); //turning the timer clock off
			PORTB |= (1<<PB7); //turning on the buzzer
			_delay_ms(50);  //waiting for a short delay for minimizing the sound noise
			PORTB &= ~(1<<PB7); //turning off the buzzer
		}

	}
	//Stop Watch Mode ON
	else
	{
		//incrementing 1 second every interrupt
		Time[0]++;

		//checking the seconds --> max = 59
		if(Time[0] > 9)
		{
			Time[0] = 0;
			Time[1]++;
		}
		if(Time[1] > 5)
		{
			Time[1] = 0;
			Time[2]++;
		}

		//checking the minutes --> max = 59
		if(Time[2] > 9)
		{
			Time[2] = 0;
			Time[3]++;
		}
		if(Time[3] > 5)
		{
			Time[3] = 0;
			Time[4]++;
		}

		//checking the hours --> max display for the 7 segments == 99 = 4 days and 3 hours
		if(Time[4] > 9)
		{
			Time[4] = 0;
			Time[5]++;
		}
		if(Time[5] > 9)
		{
			Time[5] = 0;
		}
	}
	TIFR |= (1<<OCF1A); //clearing the Timer Flag before leaving the ISR
}

//Interrupts
void INT0_init(void)
{
	MCUCR |= (1<<ISC01); //configuring INT0 with falling edge
	MCUCR &= ~(1<<ISC00);

	GICR |= (1<<INT0); //enabling INT0 module
}

ISR (INT0_vect)
{
	//RESET
	unsigned char i;
	for(i = 0; i < 6; i++)
	{
		Time[i] = 0; //clearing the time
	}
	Timer1_CTC_init(); //start counting from the beginning
	GIFR |= (1<<INTF0);//clearing the INT0 Flag before leaving the ISR
}

void INT1_init(void)
{
	MCUCR |= (1<<ISC10) | (1<<ISC11); //configuring INT1 with rising edge

	GICR |= (1<<INT1); //enabling INT1 module
}

ISR (INT1_vect)
{
	//PAUSE
	TCCR1B &=  ~(1<<CS10) & ~(1<<CS11) & ~(1<<CS12); //clear the timer clock to stop the counting
	GIFR |= (1<<INTF1); //clearing the INT1 Flag before leaving the ISR
}


void INT2_init(void)
{
	MCUCSR &= ~(1<<ISC2); //configuring INT2 with falling edge

	GICR |= (1<<INT2); //enabling INT2 module
}

ISR (INT2_vect)
{
	//RESUME
	Timer1_CTC_init(); //calling timer function to start counting again
	GIFR |= (1<<INTF2); //clearing the INT2 Flag before leaving the ISR
}


//Added Functionalities
void Set_Time(void)
{
	unsigned char i;

	//button of setting time
	if(PIND & (1<<PD0))
	{
		_delay_ms(30);
		if(PIND & (1<<PD0))
		{
			if(flag_setTime == 1)
			{
				for(i = 0; i < 6; i++)
				{
					Time[i] = 0; //clearing the time
				}
				TCCR1B &=  ~(1<<CS10) & ~(1<<CS11) & ~(1<<CS12);
				flag_setTime = 0;
			}
		}
	}
	else
	{
		flag_setTime = 1;
	}

	//button of sec 0
	if(PINB & (1<<PB0))
	{
		_delay_ms(30);
		if(PINB & (1<<PB0))
		{
			if (flag_sec0 == 1)
			{
				if(Time[0] != 9)
				{
					Time[0]++;

				}
				else
				{
					Time[0] = 0;

				}
				flag_sec0 = 0;
			}
		}
	}
	else
	{
		flag_sec0 = 1;
	}

	//button of sec 1
	if(PINB & (1<<PB1))
	{
		_delay_ms(30);
		if(PINB & (1<<PB1))
		{
			if (flag_sec1 == 1)
			{
				if(Time[1] != 5)
				{
					Time[1]++;
				}
				else
				{
					Time[1] = 0;

				}
				flag_sec1 = 0;
			}
		}
	}
	else
	{
		flag_sec1 = 1;
	}

	//button of min 0
	if(PINB & (1<<PB3))
	{
		_delay_ms(30);
		if(PINB & (1<<PB3))
		{
			if (flag_min0 == 1)
			{
				if(Time[2] != 9)
				{
					Time[2]++;
				}
				else
				{
					Time[2] = 0;
				}
				flag_min0 = 0;
			}
		}
	}
	else
	{
		flag_min0 = 1;
	}

	//button of min 1
	if(PINB & (1<<PB4))
	{
		_delay_ms(30);
		if(PINB & (1<<PB4))
		{
			if (flag_min1 == 1)
			{
				if(Time[3] != 5)
				{
					Time[3]++;
				}
				else
				{
					Time[3] = 0;
				}
				flag_min1 = 0;
			}
		}
	}
	else
	{
		flag_min1 = 1;
	}



	//button of hour 0
	if(PINB & (1<<PB5))
	{
		_delay_ms(30);
		if(PINB & (1<<PB5))
		{
			if (flag_hour0 == 1)
			{
				if(Time[4] != 9)
				{
					Time[4]++;
				}
				else
				{
					Time[4] = 0;
				}
				flag_hour0 = 0;
			}
		}
	}
	else
	{
		flag_hour0 = 1;
	}

	//button of hour 1
	if(PINB & (1<<PB6))
	{
		_delay_ms(30);
		if(PINB & (1<<PB6))
		{
			if (flag_hour1 == 1)
			{
				if(Time[5] != 9)
				{
					Time[5]++;
				}
				else
				{
					Time[5] = 0;
				}
				flag_hour1 = 0;
			}
		}
	}
	else
	{
		flag_hour1 = 1;
	}
}

void Set_Mode(void)
{
	//button of timer mode (counting down)
	if(PIND & (1<<PD1))
	{
		_delay_ms(30);
		if(PIND & (1<<PD1))
		{
			if(flag_timer == 1)
			{
				countdown = 1;
				Timer1_CTC_init();
				flag_timer = 0;
			}
		}
	}
	else
	{
		flag_timer = 1;
	}

	//button of Stop watch mode (counting up)
	if(PIND & (1<<PD4))
	{
		_delay_ms(30);
		if(PIND & (1<<PD4))
		{
			if(flag_stopWatch == 1)
			{
				countdown = 0;
				Timer1_CTC_init();
				flag_stopWatch = 0;
			}
		}
	}
	else
	{
		flag_stopWatch = 1;
	}
}

