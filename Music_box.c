//  ACLK = REFO = 32768Hz, MCLK = SMCLK = DCODIV ~1MHz.
//   Built with ode Composer Studio v6.2
//******************************************************************************

//BRCLK=SMCLK-1MHZ
//BAUDRATE =9600BPS

#include <msp430.h>

//Private function declaration
void space();
void displayString(char* string);
void convertIntToString(int number);
void addEnterToTXBuffer(void);
void menu(void);

//Global Variables
char charGlobal;
volatile unsigned int globalCounter = 0;
volatile unsigned int direction = 0;
volatile unsigned int menuOnce = 0;
volatile unsigned int servoFlag = 0;
unsigned int stepServo = 0;
unsigned int stepServoBase = 0;
int increasePwm = 0;

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

	//============================PIN CONFIGURATION======================
	P1DIR |= BIT0;
	P1OUT |= BIT0;

	//CONFIGURE PWM PINS P2.0 CCR1 P2.1 CCR2
	P2DIR |= BIT0 + BIT1;
	P2SEL0 |= BIT0 + BIT1;

	//Same option for TIMER B0
	P1DIR |= BIT6 | BIT7;                      // P1.6 and P1.7 output
	P1SEL1 |= BIT6 | BIT7;                     // P1.6 and P1.7 alternate function

	//============================TIMER B1 CONFIGURATION======================
	//DC MOTOR CONTROL WITH DEFAULT 75% DUTY CYCLE. IT USES TIMER B1 REGISTERS
	TB1CCR0 = 100-1;                                  // PWM Period
	TB1CCTL1 = OUTMOD_7;                              // CCR1 reset/set
	TB1CCR1 = 75;                                     // CCR1 PWM duty cycle
	TB1CTL = TBSSEL_1 | MC_1 | TBCLR;                 // ACLK, up mode, cleBr TBR

	//============================TIMER B0 CONFIGURATION======================
	//2 SERVO MOTORS ARE CONTROLLED WITH TIMER B0 WITH PERIOD 20000. DUTY CYCLE IS CONTROLLED BY TB0CCR1 & TB0CCR2
	TB0CTL = TBSSEL_2 | TBCLR;                          // ACLK, clear TBR

	//Counter Registers
	TB0CCR0 = 20000 - 1;                         // PWM Period
	TB0CCTL1 = OUTMOD_7;                         // CCR1 reset/set
	TB0CCR1 = 0;                                 // CCR1 PWM duty cycle
	TB0CCTL2 = OUTMOD_7;                              // CCR2 reset/set
	TB0CCR2 = 0;
	TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;     // SMCLK, up mode, clear TBR

	//============================CLOCK CONFIGURATION======================
	//CONFIG SMCLK FOR BAUDRATE
	FRCTL0 = FRCTLPW | NWAITS_2;
	__bis_SR_register(SCG0);                           // disable FLL
	CSCTL3 |= SELREF__REFOCLK;                         // Set REFO as FLL reference source
	CSCTL0 = 0;                                        // clear DCO and MOD registers
	CSCTL1 |= DCORSEL_5;                               // Set DCO = 16MHz
	CSCTL2 = FLLD_0 + 487;                             // DCOCLKDIV = 16MHz
	__delay_cycles(3);
	__bic_SR_register(SCG0);                           // enable FLL
	while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));         // FLL locked
	CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;         // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
	CSCTL5 = DIVM_4 | DIVS_0;
	PM5CTL0 &= ~LOCKLPM5;                       	   // Disable the GPIO power-on default high-impedance mode to activate previously configured port settings

	//============================UART A1 AND A0 CONFIGURATION======================
	//Configure UART pins
	P4SEL0 |= BIT3 | BIT2;                      // set 2-UART pin as UART function

	//GOOD UART
	// Configure UART FOR PC
	UCA1CTLW0 |= UCSWRST;
	UCA1CTLW0 |= UCSSEL_2 ;                    // set SMCLK as BRCLK
	UCA1BR0 = 0X06;                            //6
	UCA1BR1 = 0x00;                            //0
	UCA1MCTLW = 0x2081;
	UCA1CTLW0 &= ~UCSWRST;					   // Initialize eUSCI
	UCA1IE |= UCRXIE;		                   // Enable USCI_A0 RX interrupt

	//TEST UART 9600 OK
	//UCA1BR0 = 0X01;		//6
	//UCA1BR1 = 0x00;		//0
	//UCA1MCTLW = 0x00A1;

	UCA0BR0 = 0x06;		//6
	UCA0BR1 = 0x00;		//0
	UCA0MCTLW = 0x2081;
	UCA0CTLW0 &= ~UCSWRST;		// Initialize eUSCI
	UCA0IE |= UCRXIE;		// Enable USCI_A0 RX interrupt

	//============================MENU CONFIGURATION======================
	//Present the main menu once at the begining
	if(menuOnce == 0)
	{
		menuOnce = 1;
		menu();
	}

	//Global variable to keep track of main loop execution
	globalCounter++;
	__bis_SR_register(GIE);         // Enter LPM3, interrupts enabled

}

//ADD THE SPACE CHARACTER TO UART STRINGS
void space()
{
    while(!(UCA1IFG&UCTXIFG));
    UCA1TXBUF = ' ';
}

//CONTROL THE MOTORS VIA UART A1
//````````  UART A1 ``````````````````````
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
  switch(__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG))
	{
		case USCI_NONE:
		{
			break;	
		}

		case USCI_UART_UCRXIFG:
		{
			while(!(UCA1IFG&UCTXIFG));	  
			charGlobal = UCA1RXBUF;

			if(charGlobal == '2')
			{
				if(direction == 0)
				{
					P3DIR |= BIT0;
					P3OUT |= BIT0;

					P3DIR |= BIT5;
					P3OUT &= ~BIT5;
					direction = 1;
					__delay_cycles(100000);
				}
				else if(direction == 1)
				{
					P3DIR |= BIT0;
					P3OUT &= ~BIT0;
					P3DIR |= BIT5;
					P3OUT |= BIT5;
					direction = 2;
					__delay_cycles(100000);
				}
				else if(direction == 2)
				{
					P3DIR |= BIT0;
					P3OUT &= ~BIT0;
					P3DIR |= BIT5;
					P3OUT &= ~BIT5;
					direction = 0;
					__delay_cycles(100000);
				}
			}

		    if(charGlobal == '3')
			{
				P1DIR |= BIT0;
				P1OUT |= BIT0;
				TB0CCR1 = 2000;
			}

			if(charGlobal == 'a')
			{
				displayString("a to left, d to right");
				addEnterToTXBuffer();
				stepServo = TB0CCR1;
				stepServo+=20;
				if(stepServo < 930) stepServo = 930;
				if(stepServo > 1650) stepServo = 1650;
				TB0CCR1 = stepServo ;
				convertIntToString(TB0CCR1);
				addEnterToTXBuffer();
			}

			if(charGlobal == 'd')
			{
				displayString("a to left, d to right");
				addEnterToTXBuffer();

				stepServo = TB0CCR1;
				stepServo-=20;
				if(stepServo < 930) stepServo = 930;
				if(stepServo > 1650) stepServo = 1650;
				TB0CCR1 = stepServo ;
				convertIntToString(TB0CCR1);
				addEnterToTXBuffer();
			}
			
			  //BASE SERVO w s
			if(charGlobal == 'w')
			{
				displayString("w to down, s to up");
				addEnterToTXBuffer();
				stepServoBase = TB0CCR2;
				stepServoBase+=40;
				if(stepServoBase < 1100) stepServoBase = 1100;
				if(stepServoBase > 2680) stepServoBase = 2680;
				TB0CCR2 = stepServoBase ;
				convertIntToString(TB0CCR2);
				addEnterToTXBuffer();
			}

			if(charGlobal == 's')
			{
				displayString("w to down, s to up");
				addEnterToTXBuffer();

				stepServoBase = TB0CCR2;
				stepServoBase-=40;
				if(stepServoBase < 1100) stepServoBase = 1100;
				if(stepServoBase > 2680) stepServoBase = 2680;
				TB0CCR2 = stepServoBase ;
				convertIntToString(TB0CCR2);
				addEnterToTXBuffer();
			}

			if(charGlobal == '1')
			{
				P1DIR |= BIT0;
				P1OUT ^= BIT0;
				int i = 0;

				switch(servoFlag)
				{
					case 0 :
					{
						char* servo = "Controlling servo\r\n\0";
						displayString(servo);
						addEnterToTXBuffer();
						servoFlag = 1;
						break;
					}

					case 1 :
					{
						TB0CCR1 = 1650;
						convertIntToString(1650);
						servoFlag = 2;
						TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
						break;
					}

					case 2:
					{
						TB0CCR1 = 930;
						convertIntToString(930);
						servoFlag = 0;
						TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
						break;
					}

					case 3:
					{
						displayString("a to the left, s to the right");
						convertIntToString(TB0CCR1);
						addEnterToTXBuffer();
						char left = 'a';
						char right = '.';
						int stepServo = 0;

						//LIMIT GUARD
						if(stepServo < 350) stepServo = 350;
						if(stepServo > 2350) stepServo = 2350;

						if(UCA1RXBUF == 'a')
							{
								TB0CCR1 = 100 ;
								convertIntToString(TB0CCR1);
								addEnterToTXBuffer();
							}

						if(UCA1RXBUF == 's')
							{
								stepServo--;
								convertIntToString(TB0CCR1);
								addEnterToTXBuffer();
							}

						TB0CCR1 = stepServo;
						servoFlag = 0;
						break;					
					}
				}
			}
			
			if(charGlobal == '5')
			{
				int i = 0;
				char s[]= "Pwm value:";
				while(i < 11)
				{
					while(!(UCA1IFG&UCTXIFG));
					UCA1TXBUF = s[i];
					i++;
				}
				
				i = 0;

				switch(increasePwm)
				{
					case 0 :
					{
						TB1CCR1 = 10;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '1';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						increasePwm = 1;
						break;
					}

					case 1:
					{
						TB1CCR1 = 20;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '2';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 2;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 2:
					{
						TB1CCR1 = 30;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '3';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 3;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 3:
					{
						TB1CCR1 = 40;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '4';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 4;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 4:
					{
						TB1CCR1 = 50;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '5';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 5;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 5:
					{
						TB1CCR1 = 60;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '6';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 6;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 6:
					{
						TB1CCR1 = 70;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '7';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 7;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 7:
					{
						TB1CCR1 = 80;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '8';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 8;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 8:
					{
						while(!(UCA1IFG&UCTXIFG));
						TB1CCR1 = 90;
						space();
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '9';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 9;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						break;
					}

					case 9:
					{
						TB1CCR1 = 100;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '1';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '0';
						increasePwm = 10;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						while(!(UCA1IFG&UCTXIFG));
						break;
					}

					case 10:
					{
						UCA1TXBUF = 0xFFFF;
						TB1CCR1 = 0;
						while(!(UCA1IFG&UCTXIFG));
						UCA1TXBUF = '\r';
						while(!(UCA1IFG&UCTXIFG));
						increasePwm = 0;
						break;       
					}
				}
			}
			
			if(charGlobal == '6')
			{
			  char charGlobalCounter[10];
			  int i = 0;
			  char s[] = "Hello\n\r";
				  while(i < 10)
				  {
					while(!(UCA1IFG&UCTXIFG));
					UCA1TXBUF = s[i];
					i++;
				  }

			  TB0CCR1 = 2000;
			}
			break;
		}
		
		case USCI_UART_UCTXIFG:
		{
			break;
		}
			
		case USCI_UART_UCSTTIFG:
		{
			break;
		}

		case USCI_UART_UCTXCPTIFG:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

//USING UART A0
//A0 UART VECTOR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
	switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
	{
		case USCI_NONE: break;
		case USCI_UART_UCRXIFG:
			while(!(UCA0IFG&UCTXIFG));
			UCA0TXBUF = UCA0RXBUF;
			if(UCA0TXBUF == '9') 
				TB1CCR1 = 100;
			else 
				TB1CCR1 = 50;
			__no_operation();
			break;
		case USCI_UART_UCTXIFG: break;
		case USCI_UART_UCSTTIFG: break;
		case USCI_UART_UCTXCPTIFG: break;
		default: break;
	}
}

void menu()
{
	char* welcome = "Welcome to Motor interface\n\r\n\r\0";
	char* firstLine = " Press 1 to control the servo motor\n\r\0";
	char* secondLine = " Press 2 sequentially to change spinning direction and to stop the DC motor\n\r\0";
	char* thirdLine = " Press 5 to increase the duty cycle from 0% to 100%\n\r\0";
	displayString(welcome);
	displayString(firstLine);
	displayString(secondLine);
	displayString(thirdLine);
}

void displayString(char* string)
{
	while(*string != '\0')
	{
		while(!(UCA1IFG&UCTXIFG));
		UCA1TXBUF = *string++;
	}
}

void convertIntToString(int number)
{
	static const unsigned int bufSize = 10;
	unsigned int string[bufSize];
	unsigned int index = 0;

	//Init buffer with NULL value(int 0)
	for(index = 0; index < bufSize; index++)
	{
		string[index] = 0;
	}

	while(number > 0)
	{
		index--;
		int a = number % 10 + 48;
		//if(a == 48) a = '0';
		number /= 10;
		string[index] = a;
	}

	for(index = 0;index < bufSize; index++)
	{
		while(!(UCA1IFG&UCTXIFG));
		UCA1TXBUF = string[index];
	}
}

void addEnterToTXBuffer()
{
	while(!(UCA1IFG&UCTXIFG));
	UCA1TXBUF = '\r';
	while(!(UCA1IFG&UCTXIFG));
	UCA1TXBUF = '\n';
}
