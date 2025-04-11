# msp430

errata na stronie [errata-slaz282ac.pdf](https://github.com/dialogedge/msp430/blob/main/errata-slaz282ac.pdf)
dotyczy mikrokontrolera z rodziny MSP430 (F541x/F543x) i opisuje trzy problemy:


1. **RTC3 - Port interrupt may be missed on entry to LPMx.5**
   - **Problem**: Gdy przerwanie portu występuje w małym oknie czasowym (około 1 cyklu MCLK) wejścia urządzenia do trybu niskiego poboru mocy LPM3.5 lub LPM4.5, istnieje możliwość, że przerwanie zostanie utracone i nie wywoła wybudzenia z trybu LPMx.5.
   - **Rozwiązanie**: Nie podano rozwiązania dla tego problemu.

2. **RTC6 - Unreliable write to RTC register**
   - **Problem**: Dostęp zapisu do rejestrów RTC (SEC, MIN, HOUR, DATE, MON, YEAR, DOW) może dawać nieoczekiwane wyniki. W konsekwencji adresowany rejestr może nie zawierać zapisanych danych lub dane mogą być przypadkowo zapisane do innych rejestrów RTC.
   - **Rozwiązanie**: Zaleca się używanie procedur biblioteki RTC, dostępnych jako przykłady kodu F541x/F543x na stronie MSP430 Code Examples (www.ti.com/msp430 > Software > Code Examples), które używają starannie wyrównanych instrukcji MOV. Biblioteka jest wymieniona jako [RTC_Workaround.zip](http://www.ti.com/lit/zip/slac166) i zawiera przykładowe projekty [CCE](RTC_Workaround_CCS) i [IAR](RTC_Workaround_IAR) , które pokazują prawidłowe użycie.

3. **SYS10 - RTC frequency adjustment step size issue**
   - **Problem**: W trybie BCD rozmiar kroku regulacji częstotliwości RTC wynosi +8ppm/-4ppm, co jest dwukrotnie większe niż określono w instrukcji użytkownika. W trybie BCD dla kalibracji w górę daje to rozmiar kroku na krok 8ppm (1024 cykle) zamiast 4ppm (512 cykli). Dla kalibracji w dół daje to rozmiar kroku na krok 4ppm (512 cykli) zamiast 2ppm (256 cykli). W trybie binarnym rozmiar kroku wynosi +4ppm/-2ppm zgodnie ze specyfikacją.
   - **Rozwiązanie**: W trybie BCD można zapisać połowę wartości kalibracji do rejestru RTCCAL, aby skompensować podwojony rozmiar kroku.

Dodatkowo errata wspomina o problemie z sekwencją wejściową BSL (Bootstrap Loader), która podlega określonym wymaganiom czasowym - faza niska pinu TEST/SBWTCK nie może przekraczać 15μs, co jest szybsze niż większość portów szeregowych PC może zapewnić.

## Przyklad problemu z poborem pradu do 4mA z baterii litowej po odlaczeniu zasilania



  
KOD Z NIE-MIGAJACA DIODA
```c
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

```

+ Port P8.0 powinien co sekunde dawac sygnal na przemian niski i wysoki a nie robi tego w trybie RTC
+ Ponizej program dziala poprawnie, dioda miga

kluczowe różnice, które wyjaśniają dlaczego drugi program działa poprawnie (dioda miga), a pierwszy nie:

### Źródło zegara dla RTC:
- Pierwszy przykład: `RTCCTL01 = RTCTEVIE | RTCSSEL_0 | RTCTEV_0;` używa RTCSSEL_0 (kryształ XT1)
- Drugi przykład: `RTCCTL01 = RTCTEVIE + RTCSSEL_2 + RTCTEV_0;` używa RTCSSEL_2 (wyjście RT1PS)

### Konfiguracja preskalera:
- Pierwszy przykład: RT0PSDIV_7 (÷128) i RT1SSEL_2 | RT1PSDIV_3 (źródło RT0PS, ÷16)
- Drugi przykład: RT0PSDIV_2 (÷8) i RT1SSEL_2 + RT1PSDIV_3 (źródło RT0PS, ÷16)

Problem jest związany jest z błędem opisanym w erracie "RTC6 - Unreliable write to RTC register". 
W pierwszym kodzie uywamy XT1, ale zapisy do rejestrów RTC **mogą być niestabilne i dawać nieoczekiwane wyniki**. 

### Drugi kod (działający):
1. Używa innego źródła zegara dla RTC (RTCSSEL_2 zamiast RTCSSEL_0)
2. Używa innych ustawień preskalera
3. Jest oficjalnym przykładem od TI, który prawdopodobnie zawiera obejścia znanych błędów

### Jak naprawi pierwszy kod?
Propozycje:
1. Zmienić źródło zegara z XT1 (RTCSSEL_0) na RT1PS (RTCSSEL_2)
2. Dostosować ustawienia preskalera do działającego przykładu
3. Użyć biblioteki RTC_Workaround wymienionej w erracie (http://www.ti.com/lit/zip/slac166)

Dodatkowo, pierwszy kod może być dotknięty przez problem opisany w erracie "SYS10 - RTC frequency adjustment step size issue", co wpływa na dokładność odmierzania czasu.

KOD Z MIGAJACA DIODA  
```c
//******************************************************************************
//  MSP430F543xA Demo - RTC in Counter Mode toggles P1.0 every 1s
//
//  This program demonstrates RTC in counter mode configured to source from ACLK
//  to toggle P1.0 LED every 1s.
//
//                MSP430F5438A
//             -----------------
//        /|\ |                 |
//         |  |                 |
//         ---|RST              |
//            |                 |
//            |             P1.0|-->LED
//
//   M. Morales
//   Texas Instruments Inc.
//   June 2009
//   Built with CCE Version: 3.2.2 and IAR Embedded Workbench Version: 4.11B
//******************************************************************************
// make GCC_DIR=/mnt/c/ti/gcclin DEVICE=MSP430F5419A EXAMPLE=msp430x54xA_RTC_01

#include <msp430.h>

int main(void)
{
  WDTCTL = WDTPW+WDTHOLD;
  
  P8DIR |= BIT0;
  P8OUT |= BIT0;
  

  // Setup RTC Timer
  RTCCTL01 = RTCTEVIE + RTCSSEL_2 + RTCTEV_0; // Counter Mode, RTC1PS, 8-bit ovf
                                            // overflow interrupt enable
  RTCPS0CTL = RT0PSDIV_2;                   // ACLK, /8, start timer
  RTCPS1CTL = RT1SSEL_2 + RT1PSDIV_3;       // out from RT0PS, /16, start timer

  __bis_SR_register(LPM3_bits + GIE);
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(RTC_VECTOR))) RTC_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(RTCIV,16))
  {
    case 0: break;                          // No interrupts
    case 2: break;                          // RTCRDYIFG
    case 4:                                 // RTCEVIFG
      P8OUT ^= BIT0;
      break;
    case 6: break;                          // RTCAIFG
    case 8: break;                          // RT0PSIFG
    case 10: break;                         // RT1PSIFG
    case 12: break;                         // Reserved
    case 14: break;                         // Reserved
    case 16: break;                         // Reserved
    default: break;
  }
}
```
