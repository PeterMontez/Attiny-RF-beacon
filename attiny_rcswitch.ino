#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define TX_PIN PB3      // Transmitter connected to PB3
#define BUTTON_PIN PB4  // Pushbutton connected to PB4

volatile uint8_t wdt_counter = 0; // Counter for watchdog timer interrupts
volatile uint8_t button_pressed = 0; // Flag to indicate button press

void setup() {
    // Set transmitter pin as output
    DDRB |= (1 << TX_PIN);

    // Set button pin as input with pull-up resistor
    DDRB &= ~(1 << BUTTON_PIN);  // Set PB4 as input
    PORTB |= (1 << BUTTON_PIN);  // Enable pull-up resistor on PB4

    // Enable Pin Change Interrupt on PB4
    GIMSK |= (1 << PCIE);  // Enable Pin Change Interrupt
    PCMSK |= (1 << PCINT4); // Enable interrupt on PB4
}

void sendBit(bool bit) {
    if (bit) {
        PORTB |= (1 << TX_PIN);  // Transmit HIGH
        _delay_us(350);          // HIGH pulse duration
        PORTB &= ~(1 << TX_PIN); // Transmit LOW
        _delay_us(1050);         // LOW pulse duration
    } else {
        PORTB |= (1 << TX_PIN);  // Transmit HIGH
        _delay_us(350);          // HIGH pulse duration
        PORTB &= ~(1 << TX_PIN); // Transmit LOW
        _delay_us(350);          // LOW pulse duration
    }
}

void sendData(const char* data) {
    // Send synchronization pulse
    PORTB |= (1 << TX_PIN);
    _delay_us(350);
    PORTB &= ~(1 << TX_PIN);
    _delay_us(10500);

    // Send each character in the data string
    for (int i = 0; data[i] != '\0'; i++) {
        for (int j = 7; j >= 0; j--) {
            sendBit((data[i] >> j) & 1); // Send each bit (MSB first)
        }
    }

    // Turn off the transmitter
    PORTB &= ~(1 << TX_PIN);
}

void enterDeepSleep() {
    // Set sleep mode to POWER_DOWN (deep sleep)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Enable interrupts (if needed) before sleeping
    sei();

    // Put the microcontroller to sleep
    sleep_cpu();

    // Code will continue here after waking up
    sleep_disable();
}

// Pin Change Interrupt Service Routine
ISR(PCINT0_vect) {
    // Check if the button is pressed (active low)
    if (!(PINB & (1 << BUTTON_PIN))) {
        button_pressed = 1; // Set the button press flag
    }
}

// Watchdog Timer Interrupt Service Routine
ISR(WDT_vect) {
    // Increment the watchdog timer counter
    wdt_counter++;

    // Disable watchdog timer interrupt
    WDTCR &= ~(1 << WDIE);
}

int main(void) {
    setup();

    while (1) {
        // Check if the button was pressed
        if (button_pressed) {
            sendData("123456");  // Transmit the data
            button_pressed = 0;  // Reset the button press flag
        }

        // Transmit the data after 1 minute (60 seconds)
        if (wdt_counter >= 4) { // 3 × 16s + 1 × 12s = 60s
            sendData("123456");  // Transmit the data
            wdt_counter = 0;     // Reset the counter
        }

        // Configure the watchdog timer for 16 seconds (or 12 seconds for the last cycle)
        if (wdt_counter < 3) {
            WDTCR |= (1 << WDP3) | (1 << WDP2); // 16 seconds
        } else {
            WDTCR |= (1 << WDP3) | (1 << WDP0); // 12 seconds
        }
        WDTCR |= (1 << WDIE); // Enable watchdog interrupt

        enterDeepSleep(); // Enter deep sleep
    }
}
