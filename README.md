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

## Przyklady problemu z poborem pradu do 4mA z baterii litowej po odlaczeniu zasilania

  
### KOD Z NIE-MIGAJACA DIODA msp430x54xA_RTC_03
```c
// make GCC_DIR=/home/tom/ti/gcc DEVICE=MSP430F5419A EXAMPLE=msp430x54xA_RTC_03
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

### KOD Z MIGAJACA DIODA  msp430x54xA_RTC_01

przykład, który omija znane problemy RTC w tych mikrokontrolerach, ktory nie korzysta bezpośrednio z rejestrów RTC, które są wymienione w erracie. 
Zamiast tego używa bezpiecznych metod do konfiguracji zegara RTC:

1. Przykład używa RTC w trybie licznika (Counter Mode), a nie w trybie kalendarza, więc nie używa problematycznych rejestrów SEC, MIN, HOUR, DATE, MON, YEAR, DOW, których dotyczy błąd RTC6.

2. Program konfiguruje:
   - RTCCTL01 do ustawienia trybu licznika z RTCSSEL_2 (wybór źródła zegara) i włączenia przerwań
   - RTCPS0CTL i RTCPS1CTL do konfiguracji podziału częstotliwości

3. Obsługa przerwań jest realizowana poprzez odpowiednie przerwanie RTC_VECTOR i instrukcję switch, która sprawdza rejestr RTCIV, aby określić źródło przerwania.

Program jest prosty - włącza diodę LED podłączoną do pinu P8.0 i konfiguruje RTC w trybie licznika do generowania przerwań, które przełączają stan diody co 1 sekundę. Nie używa on zapisów do rejestrów czasu rzeczywistego, które są wymienione jako problematyczne w erracie, więc nie powinien być dotknięty opisanymi problemami.


```c
//******************************************************************************
//  MSP430F543xA Demo - RTC in Counter Mode toggles P8.0 every 1s
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
//            |             P8.0|-->LED
//
//   M. Morales
//   Texas Instruments Inc.
//   June 2009
//   Built with CCE Version: 3.2.2 and IAR Embedded Workbench Version: 4.11B
//******************************************************************************
// make GCC_DIR=/home/tom/ti/gcc DEVICE=MSP430F5419A EXAMPLE=msp430x54xA_RTC_01

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


## Poprawienie msp430x54xA_RTC_05


Te zmiany powinny sprawić, że RTC będzie działać poprawnie i dioda zacznie migać co sekundę, tak jak w działającym przykładzie.

Konfiguracja kryształu XT1 nie jest potrzebna przy korzystaniu z RTCSSEL_2, ponieważ RTCSSEL_2 wybiera inny sygnał zegarowy jako źródło dla RTC.

Wyjaśnienie:

1. **RTCSSEL_0** oznacza, że RTC ma używać kryształu XT1 jako źródła zegara:
   - Przy tym ustawieniu kryształ XT1 musi być prawidłowo skonfigurowany
   - Wymaga to konfiguracji pinów P7.0 i P7.1 jako funkcji kryształu
   - Wymaga również inicjalizacji i stabilizacji oscylatora XT1

2. **RTCSSEL_2** oznacza, że RTC używa wyjścia z preskalera RT1PS jako źródła zegara:
   - RT1PS jest wewnętrznym dzielnikiem częstotliwości
   - RT1PS może pobierać sygnał z wewnętrznego zegara ACLK (domyślnie)
   - Nie wymaga zewnętrznego kryształu XT1 do działania

W działającym przykładzie:
```c
RTCPS0CTL = RT0PSDIV_2;                   // ACLK, /8, start timer
RTCPS1CTL = RT1SSEL_2 + RT1PSDIV_3;       // out from RT0PS, /16, start timer
```

Używany jest wewnętrzny zegar ACLK, który jest dzielony przez preskaler RT0PS (/8), a następnie przez RT1PS (/16). Te dzielniki generują sygnał, który jest używany do odmierzania czasu w RTC.

Ponieważ w tym przypadku RTC używa wewnętrznego sygnału zegarowego (ACLK) zamiast zewnętrznego kryształu XT1, cała konfiguracja XT1 staje się zbędna i można ją bezpiecznie usunąć.



### Dlaczego 3 przykład działa prawidłowo:
Ten działający przykład jest oficjalnym kodem demonstracyjnym od Texas Instruments z uwzględnieniem znanych problemów opisanych w erracie dla tego mikrokontrolera.
Dzięki użyciu wewnętrznego źródła zegara i odpowiedniej konfiguracji dzielników, może on uniknąć błędów, które występują przy bezpośrednim korzystaniu z zewnętrznego kryształu.

1. **Źródło zegara dla RTC**:
   - Działający przykład używa `RTCSSEL_2` (wyjście z preskalera RT1PS) jako źródła zegara
   - Niedziałający przykład używa `RTCSSEL_0` (zewnętrzny kryształ XT1)
   - Wykorzystanie wewnętrznego źródła RT1PS omija problemy z kryształem XT1 opisane w erracie

2. **Preskaler RTC**:
   - Działający przykład używa `RT0PSDIV_2` (dzielnik /8)
   - Niedziałający przykład używa `RT0PSDIV_7` (dzielnik /128)
   - Różnica w dzielnikach wpływa na częstotliwość generowanych przerwań

3. **Unikanie znanych błędów z erraty**:
   - Działający przykład unika problemu "RTC6 - Unreliable write to RTC register" przez:
     - Nieużywanie zewnętrznego kryształu XT1
     - Korzystanie z wewnętrznego źródła zegara RT1PS
   - Konfiguracja w działającym przykładzie jest zgodna z rekomendacjami TI

4. **Brak konfiguracji kryształu**:
   - Działający przykład nie wymaga konfiguracji i stabilizacji kryształu XT1
   - Eliminuje to potencjalne problemy z inicjalizacją i stabilizacją zewnętrznego oscylatora

5. **Struktura obsługi przerwania**:
   - Działający przykład zawiera pełną obsługę wszystkich możliwych kodów przerwania RTC
   - To zapewnia, że wszystkie potencjalne przerwania są obsługiwane poprawnie


```c
#include <msp430.h>

int main(void)
{
  WDTCTL = WDTPW+WDTHOLD;
  
  P8DIR |= BIT0;
  P8OUT |= BIT0;  // Dioda włączona na początek
  
  // Setup RTC Timer - dokładnie jak w działającym przykładzie
  RTCCTL01 = RTCTEVIE + RTCSSEL_2 + RTCTEV_0; // Counter Mode, RTC1PS, 8-bit ovf
                                            // overflow interrupt enable
  RTCPS0CTL = RT0PSDIV_2;                   // ACLK, /8, start timer
  RTCPS1CTL = RT1SSEL_2 + RT1PSDIV_3;       // out from RT0PS, /16, start timer

  __bis_SR_register(LPM3_bits + GIE);
  
  return 0;  // Ten return nigdy się nie wykona w trybie LPM3
}

// Użyjmy dokładnie takiej samej struktury obsługi przerwania jak w działającym przykładzie
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
