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
| RailPower | OFF, ON, SHORT_CIRCUIT, FAULT und COMMUNICATION_TIMEOUT. Verwendet RailPowerLowLevel. Freigabe, Sperre, Fehlerquittierung. | Implementiert. |
| CurrentSensor in CurrentSensors.h/.cpp | ADC-Rohwerte in mA umrechnen; Bahnprobe und Pegel zum Messzeitpunkt zusammenhalten. Keine Fehlerentscheidung. | ADC- und DMA-Verarbeitung implementiert. |
| SignalGenerator | PRG-, PG-, ACT- und CAR-Telegramme bilden und Manchester-Pegel mit Dauer liefern. | Ausgabestufe für sechs Fahrzeuge implementiert. |
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

## USART2-Kommunikation über den Moxa

Der Moxa verbindet Ethernet mit RS232. Der STM32 sendet und empfängt Bytes über USART2 (115200 Baud, 8N1). Das Datenformat steht in [MOXA.md](MOXA.md).

1. CubeMX initialisiert `huart2`, aktiviert den USART2-Interrupt und erzeugt `USART2_IRQHandler()`. `ControlUnit_Init()` setzt die USART2-Priorität auf 2, damit TIM2 und ADC-DMA Vorrang haben. Nach erfolgreichem Start der Bahnüberwachung und des UART-Empfangs beginnt die Hauptschleife. Die Bahn startet in `OFF`.
2. Nach dem zweiten Byte meldet `HAL_UART_RxCpltCallback()` den Empfang an `UartTransport`. Der Transport speichert das Wort und startet den nächsten Empfang. Das höherwertige Byte kommt zuerst.
3. `ControlUnit_Tick()` in `source/app/ControlUnit.cpp` prüft erst die Bahn und holt dann höchstens einen UART-Befehl ab. `ControlUnit::handleCommand()` in `source/libraries/custom/ControlUnit.cpp` verarbeitet ihn und liefert das Antwortwort. Hier kommen weitere Befehle dazu.
4. `UartTransport::trySendWord()` startet die Antwort ohne Wartezeit. Ist der UART noch belegt, bleibt die Antwort für den nächsten Durchlauf vorgemerkt. `HAL_UART_TxCpltCallback()` gibt den Sendepuffer wieder frei. Die HAL-Callbacks stehen in `source/app/ControlUnit.cpp`.

Der Transport hält ein vollständiges Wort zur Abholung bereit. Trifft vorher ein weiteres ein, bleibt das ältere Wort erhalten. Die SPS wartet laut Protokoll die Antwort ab, bevor sie den nächsten Befehl sendet. `INIT` (`00 00`) bestätigt die Kommunikation. Status, Bahnfreigabe und Fehlerquittierung folgen dem [MOXA-Protokoll](MOXA.md). Der Betriebs-Heartbeat der SPS wird alle 250 ms erwartet. Wenn er nach dem ersten Empfang für 1000 ms ausbleibt, sperrt der STM32 den Bahnausgang. Statusabfragen aktualisieren den Watchdog nicht.

Nach einem UART-Empfangsfehler oder einem fehlgeschlagenen Empfangsneustart wird der Empfang abgebrochen. Weil das 2-Byte-Format keine Startmarkierung hat, müssen beide Seiten bei einem Fehler neu synchronisiert werden. Die Firmware bietet dafür derzeit einen Neustart der ControlUnit.

## Carrera-Signal, Timing und Schutz

Der [OpenSlot-Zyklus](https://openslot.de/docs/datenprotokoll) besteht aus zehn Telegrammen im Abstand von 7,5 ms. Die Klasse SignalGenerator erzeugt den PRG-Wert für Normalbetrieb aus dem OpenSlot-Beispiel und die PG-Meldung mit gesperrtem Ghost- und Pacecar. ACT meldet die Fahrzeuge mit Fahrstufe über null. Die sechs CAR-Telegramme enthalten Fahrstufe und das aktive LOW-Spurwechselbit. Tankfunktion und Fernprogrammierung sind nicht vorgesehen.

HSI16 taktet TIM2 nach dem Vorteiler mit 1 MHz. Compare 1 gibt die Pegel über RailPower aus. Ein Manchester-Halbbit dauert 50 µs. Der Interrupt schaltet zuerst den vorbereiteten Pegel und berechnet dann den folgenden Pegel. Der nächste Compare wird relativ zum vorherigen geplant, auch beim Zählerüberlauf. Ist der Pegelwechsel erheblich verspätet, verriegelt die ControlUnit den Ausgang und speichert FAULT.

Compare 4 startet alle 400 µs eine ADC-Sequenz 20 µs nach Beginn eines möglichen Manchester-Halbbits. Der ADC läuft mit 8 MHz und erfasst zuerst PA0 und danach PA1 mit je 79,5 ADC-Takten Abtastzeit. Der Compare-Interrupt erfasst PA8 und der DMA-Callback mit niedrigerer Priorität speichert diesen Pegel mit der Bahnprobe. Einschwingzeit der Endstufe und des Strommesspfads sowie Interrupt-Latenz am Board messen.

RailPower hält PA8 bei OFF oder Fehler auf LOW. Beim Wiedereinschalten setzt SignalGenerator alle Fahrstufen und Spurwechselanforderungen zurück und startet bei PRG. Die Firmware sperrt kurze konkurrierende Zugriffe auf den SignalGenerator und auf die Zuordnung zwischen TIM2 und ADC-DMA.

## Tatsächlicher Stand

Signalerzeugung, SPS-Fahrbefehle, Strommessung und Abschaltlogik sind verbunden. Signalform, Abtastung, Stromskalierung und Abschaltzeit müssen am Board geprüft werden.

Die nächsten Schritte stehen in [TODO.md](TODO.md).
