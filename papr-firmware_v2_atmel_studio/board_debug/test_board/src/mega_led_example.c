#define F_CPU 8000000UL

#include <asf.h>
#include <avr/io.h>
#include <util/delay.h>

#define Off(led_gpio) gpio_set_pin_high(led_gpio)
#define On(led_gpio) gpio_set_pin_low(led_gpio)

// Battery Outputs
#define LED_RED IOPORT_CREATE_PIN(PORTC, 0)
#define LED_YELLOW IOPORT_CREATE_PIN(PORTC, 1)
#define LED_GREEN IOPORT_CREATE_PIN(PORTC, 2)

#define LED_WHITE IOPORT_CREATE_PIN(PORTC, 3)

#define LED_BLUE_1 IOPORT_CREATE_PIN(PORTC, 4)
#define LED_BLUE_2 IOPORT_CREATE_PIN(PORTC, 5)
#define LED_BLUE_3 IOPORT_CREATE_PIN(PORTD, 0)

#define LED_DELAY 1000

int main (void)
{
	int leds[] = {
		LED_RED,
		LED_YELLOW,
		LED_GREEN,
		
		LED_WHITE,
		
		LED_BLUE_1,
		LED_BLUE_2,
		LED_BLUE_3,
	};
	
	for (int i = 0; i <  sizeof(leds)/sizeof(LED_RED); i++) {
		ioport_configure_pin(leds[i], IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH);
	}
			
	while(1){
		//button_state = ioport_get_pin_level(GPIO_PUSH_BUTTON_0);
		
		for (int i = 0; i <  sizeof(leds)/sizeof(LED_RED); i++) {
			On(leds[i]);
		}
		_delay_ms(LED_DELAY);
		
		for (int i = 0; i <  sizeof(leds)/sizeof(LED_RED); i++) {
			Off(leds[i]);
		}
		_delay_ms(LED_DELAY);
	}	
}
