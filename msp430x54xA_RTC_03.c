#include <msp430.h>

int main(void)
{
    // Wyłączenie watchdoga
    WDTCTL = WDTPW | WDTHOLD;

    // Konfiguracja pinu diody
    P8DIR |= BIT0;       // Ustawienie P8.0 jako wyjście
    P8OUT &= ~BIT0;      // Wyłączenie diody przy starcie

    // Konfiguracja pinów kryształu
    P7SEL |= BIT0 | BIT1;   // P7.0 i P7.1 jako funkcja kryształu 

    // Konfiguracja oscylatora XT1
    UCSCTL6 &= ~XT1OFF;     // Włączenie XT1
    UCSCTL6 |= XCAP_0;      // Wyłączenie wewnętrznych kondensatorów

    // Czekaj na stabilizację kryształu
    do {
        UCSCTL7 &= ~(XT1LFOFFG + DCOFFG);  // Wyczyść flagi błędów
        SFRIFG1 &= ~OFIFG;                 // Wyczyść główną flagę błędu oscylatora
    } while (SFRIFG1 & OFIFG);              // Czekaj na ustabilizowanie się oscylatora

    // Konfiguracja RTC
    RTCCTL01 = RTCTEVIE | RTCSSEL_0 | RTCTEV_0;  // Źródło zegara: XT1, przerwanie co sekundę
    RTCPS0CTL = RT0PSDIV_7;  // Dzielnik /128
    RTCPS1CTL = RT1SSEL_2 | RT1PSDIV_3;  // Źródło: RT0PS, dzielnik /16

    // Włączenie przerwań i tryb niskiego poboru mocy
    __bis_SR_register(LPM3_bits | GIE);

    return 0;
}

// Procedura obsługi przerwania RTC
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    switch(__even_in_range(RTCIV, 16)) {
        case 4:  // Przerwanie od zdarzenia RTC
            P8OUT ^= BIT0;  // Przełączenie stanu diody
            break;
    }
}