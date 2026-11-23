# ControlUnit – Gesamtidee

## Zweck

Die STM32L071-ControlUnit sendet das digitale Carrera-Signal an die Bahn, überwacht parallel den Bahn- und Versorgungsstrom und liest Daten eines Wireless-Receivers. Ein erkannter Bahn-Kurzschluss soll den Ausgang unmittelbar sperren und den Fehler bis zur bewussten Quittierung speichern. Eigene Anwendungsklassen werden in C++ geschrieben; CubeMX/HAL initialisiert die STM32-Peripherie.

## Hardware-Signale

| Signal | Pin | Aufgabe |
| --- | --- | --- |
| CARERRA_DATA | PA8 | Digitaler Ausgang zur Bahn-Endstufe; HIGH fordert den leitenden Zustand an, LOW sperrt. Schreibweise entspricht dem Projekt. |
| TRACK_SHUNT | PA1 / ADC_IN1 | Strommessung der Bahn. |
| SUPPLY_SHUNT | PA0 / ADC_IN0 | Strommessung der Versorgung. |
| WIRELESS_CLK | PA6 / Ausgang | Takt zum Wireless-Receiver. |
| WIRELESS_DATA | PA7 / Eingang | Daten vom Wireless-Receiver. |
| MOXA_TX / MOXA_RX | PA2 / PA3 | Serielle externe Verbindung. |

Die Stromumrechnung braucht die tatsächlichen Shunt-Widerstände, die Verstärkung des verbauten INA180 und die ADC-Referenz. Stromgrenzen und zulässige Abschaltzeit sind noch nicht festgelegt.

## Projektstruktur und Klassen

| Baustein | Zuständigkeit | Iststand |
| --- | --- | --- |
| source/libraries/core | CubeMX, HAL, Startup, CMake und generierte Peripherie. | Vorhanden. |
| RailPowerLowLevel | PA8 schreiben, Ausgang freigeben und bei Sperre LOW erzwingen. | Implementiert. |
| RailPower | OFF, ON, SHORT_CIRCUIT; besitzt RailPowerLowLevel. Freigabe, Sperre, Fehlerquittierung. | Implementiert. |
| CurrentSensor in CurrentSensors.h/.cpp | ADC-Rohwerte in mA umrechnen; Bahnprobe und Pegel zum Messzeitpunkt zusammenhalten. Keine Fehlerentscheidung. | ADC- und DMA-Verarbeitung implementiert. |
| RailShortCircuitDetector | Mehrere gültige HIGH-Stromproben auf Grenzwert prüfen. Keine GPIO-Ansteuerung. | Implementiert. `check()` meldet einen erkannten Kurzschluss als `HAL_ERROR`. |
| source/app/ControlUnit.cpp | Klassen verbinden, Init/Ereignisse und Wireless-Nachrichten koordinieren. | Ruft die Initialisierung und Auswertung auf und leitet den TIM2-Interrupt weiter. |

Das generierte main.c initialisiert GPIO, ADC, USART2 und TIM2, ruft danach ControlUnit_Init() und fortlaufend ControlUnit_Tick() auf.

`source/app/ControlUnit.h` deklariert diese C-Einstiegspunkte. Die C++-Klasse `ControlUnit` steht in `source/libraries/custom/ControlUnitController.h`.

`CurrentSensor::init()` und `ControlUnit::init()` liefern `HAL_StatusTypeDef`. `HAL_OK` bedeutet, dass alle Schritte der jeweiligen Initialisierung erfolgreich waren. Jeder gemeldete Start- oder Konfigurationsfehler wird als `HAL_ERROR` zurückgegeben.

## Ablauf

1. Die Signalerzeugung übergibt einen gewünschten LOW/HIGH-Pegel an RailPower. RailPowerLowLevel schreibt PA8 nur HIGH, solange die Ausgabe freigegeben ist; sonst LOW.
2. Der ADC liest TRACK_SHUNT an PA1. Jede Bahnprobe wird dem PA8-Zustand **zum Messzeitpunkt** zugeordnet, nicht dem Zustand zum späteren Zeitpunkt einer DMA- oder Callback-Auswertung. Nach der Schaltflanke muss das Messsignal ausreichend eingeschwungen sein.
3. CurrentSensor rechnet den Rohwert in mA um. PA0 wird getrennt für die Versorgung überwacht.
4. RailShortCircuitDetector bewertet gültige Bahnproben während HIGH: über Grenzwert erhöht den Zähler; eine gültige HIGH-Probe darunter setzt ihn zurück. Eine Signal-LOW-Phase ist kein gutartiger Messwert.
5. Bei der festgelegten Anzahl Überschreitungen ruft die Anwendung RailPower::onShortCircuit() auf. Das verriegelt die Ausgabe, schreibt PA8 LOW und speichert SHORT_CIRCUIT. HIGH-Anforderungen werden anschließend blockiert.
6. RailPower::resetFault() setzt den gespeicherten Fehler zurück auf OFF und schaltet nicht selbst ein. Wiederanlauf umfasst auch Detector-Reset und eine bewusste erneute Freigabe.
7. Wireless-Daten steuern später die Anwendung; Protokoll und Zuordnung zur Signalerzeugung müssen noch beschrieben werden.

## USART2-Empfang über den Moxa

Der Moxa verbindet Ethernet mit RS232. Der STM32 empfängt die Bytes über USART2 (115200 Baud, 8N1). Das Datenformat steht in [MOXA.md](MOXA.md).

1. CubeMX initialisiert `huart2`, aktiviert den USART2-Interrupt und erzeugt `USART2_IRQHandler()`. `ControlUnit_Init()` setzt die USART2-Priorität auf 1, damit TIM2 und ADC-DMA mit Priorität 0 Vorrang haben, und startet den Empfang für zwei Bytes.
2. `USART2_IRQHandler()` ruft den HAL auf. Nach dem zweiten Byte leitet `HAL_UART_RxCpltCallback()` in `source/app/ControlUnit.cpp` das Ereignis für `huart2` an `UartTransport` weiter. Der Transport speichert das Wort mit dem höherwertigen Byte zuerst und startet den nächsten Empfang.
3. Die spätere Befehlsauswertung ruft `tryRecieveWord()` in `ControlUnit_Tick()` auf. Prüfen, Kopieren und Zurücksetzen des Empfangsflags passieren zusammen bei kurz gesperrten Interrupts. Ein erfolgreich abgeholtes Wort wird nur einmal geliefert.

Der Transport hält maximal ein vollständiges Wort zur Abholung bereit. Trifft vorher ein weiteres ein, bleibt das ältere Wort erhalten. Die SPS muss laut Protokoll die Antwort abwarten, bevor sie den nächsten Befehl sendet. Nach einem UART-Empfangsfehler oder einem fehlgeschlagenen Neustart wird der Empfang abgebrochen. Weil das 2-Byte-Format keine Startmarkierung hat, müssen beide Seiten bei einem Fehler neu synchronisiert werden. Die Firmware bietet dafür derzeit einen Neustart der ControlUnit. Befehlsauswertung und Antworten kommen beim nächsten Schritt dazu.

## Timing und Schutz

Der ADC erfasst mit 12 Bit und 39,5 ADC-Takten Abtastzeit zuerst PA0 und danach PA1. TIM2 CH4 startet die Sequenz per Hardware. Der Compare-Interrupt liest PA8 ein, der DMA-Callback speichert den Pegel mit der Bahnprobe und `ControlUnit::tick()` wertet diese aus. Der TIM2-Interrupt wird über die App eingebunden. Für eine gültige Zuordnung muss PA8 über das Ablesen und die PA1-Abtastung hinweg stabil sein. Bei der Signalerzeugung außerdem Abstand zur Schaltflanke und die Interrupt-Latenz am Board prüfen.

RailPowerLowLevel::writeSignalLevel() schützt Flag-Prüfung und GPIO-Schreibvorgang mit gespeichertem PRIMASK. RailPower::enable(), disable() und resetFault() verwenden ebenfalls kurze kritische Abschnitte. onShortCircuit() verriegelt derzeit ohne eigenen kritischen Abschnitt; vor Interrupt-Anbindung Race Conditions und Reihenfolge prüfen.

## Tatsächlicher Stand

Die Strommessung und die Abschaltlogik sind im Code verbunden. Die Konfiguration in der App enthält noch Nullwerte, deshalb liefert die Initialisierung bis zur Eingabe der Hardwarewerte einen Fehler und die Bahn bleibt gesperrt. Abtastung, Stromskalierung und Abschaltzeit müssen am Board geprüft werden.

Die nächsten Schritte stehen in [TODO.md](TODO.md).
