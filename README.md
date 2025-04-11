# msp430

errata na stronie  
+ [errata-slaz282ac.pdf](https://github.com/dialogedge/msp430/blob/main/errata-slaz282ac.pdf)
dotyczy mikrokontrolera z rodziny MSP430 (najprawdopodobniej F541x/F543x) i opisuje trzy problemy:


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
