# MSP430xx_DC_AND_SERVO_CONTROL

This project uses the MSP430FR2355 microcontroller.
It is an example of bare metal programming that uses register manipulation to achieve several goals:
1. Control the spinning direction and speed of a DC motor
2. Control two servo motors that move along X and Y axis 
3. Provide a basic interface via menu and keyboard selection for controlling the motors.
 
Features of the program:
1. DC Motor Control: Adjust speed with PWM duty cycles (default 75% duty cycle).
2. Servo Motor Control: Adjust angles of two servo motors using PWM signals.
3. UART Communication: Interface with a PC for sending and receiving control commands.
4. Menu System: Provides a command menu for interacting with the system.

Pin Configuration
1. P1.0: LED indicator.
2. P2.0, P2.1: PWM outputs for servo motors (CCR1 & CCR2).
3. P1.6, P1.7: PWM outputs for DC motor control.
4. P4.2, P4.3: UART pins for communication (UCA0 & UCA1).

Timer Configuration
1. Timer B0: Configured for servo motor PWM control with a period of 20,000 cycles.
2. Timer B1: Configured for DC motor PWM control with a period of 100 cycles and a default duty cycle of 75%.

Clock Configuration
1. ACLK: Set to REFO (~32.768 kHz) for various timing functions.
2. SMCLK: Configured for UART communication with a baud rate of 9600 bps.

UART Configuration
UCA0 & UCA1: Configured for UART communication with a baud rate of 9600 bps. UCA0 is used for testing, and UCA1 is used for controlling and receiving commands.

Menu Commands:

'1': Toggle the LED and cycle through servo positions.

'2': Change the spinning direction of the DC motor.

'3': Set servo position to a predefined value.

'a': Move the servo motor to the left.

'd': Move the servo motor to the right.

'w': Move the base servo down.

's': Move the base servo up.

'5': Incrementally increase the PWM duty cycle of the DC motor.

'6': Send a hello message and set a servo position.

