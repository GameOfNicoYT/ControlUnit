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
| RailCurrentSensor in CurrentSensors.h/.cpp | ADC-Rohwerte in mA umrechnen; Bahnprobe und Pegel zum Messzeitpunkt zusammenhalten. Keine Fehlerentscheidung. | Leere Klasse und leere CPP. |
| RailShortCircuitDetector | Mehrere gültige HIGH-Stromproben auf Grenzwert prüfen. Keine GPIO-Ansteuerung. | Header vorhanden; check() und reset() leer; Zähler nicht initialisiert. |
| source/app/ControlUnit.cpp | Klassen verbinden, Init/Ereignisse und Wireless-Nachrichten koordinieren. | ControlUnit_Init() und ControlUnit_Tick() leer. |

Das generierte main.c initialisiert GPIO, ADC, USART2 und TIM2, ruft danach ControlUnit_Init() und fortlaufend ControlUnit_Tick() auf.

## Ablauf

1. Die Signalerzeugung übergibt einen gewünschten LOW/HIGH-Pegel an RailPower. RailPowerLowLevel schreibt PA8 nur HIGH, solange die Ausgabe freigegeben ist; sonst LOW.
2. Der ADC liest TRACK_SHUNT an PA1. Jede Bahnprobe wird dem PA8-Zustand **zum Messzeitpunkt** zugeordnet, nicht dem Zustand zum späteren Zeitpunkt einer DMA- oder Callback-Auswertung. Nach der Schaltflanke muss das Messsignal ausreichend eingeschwungen sein.
3. RailCurrentSensor rechnet den Rohwert in mA um. PA0 wird getrennt für die Versorgung überwacht.
4. RailShortCircuitDetector bewertet gültige Bahnproben während HIGH: über Grenzwert erhöht den Zähler; eine gültige HIGH-Probe darunter setzt ihn zurück. Eine Signal-LOW-Phase ist kein gutartiger Messwert.
5. Bei der festgelegten Anzahl Überschreitungen ruft die Anwendung RailPower::onShortCircuit() auf. Das verriegelt die Ausgabe, schreibt PA8 LOW und speichert SHORT_CIRCUIT. HIGH-Anforderungen werden anschließend blockiert.
6. RailPower::resetFault() setzt den gespeicherten Fehler zurück auf OFF und schaltet nicht selbst ein. Wiederanlauf umfasst auch Detector-Reset und eine bewusste erneute Freigabe.
7. Wireless-Daten steuern später die Anwendung; Protokoll und Zuordnung zur Signalerzeugung müssen noch beschrieben werden.

## Timing und Schutz

Der ADC ist zurzeit auf 12 Bit, beide Kanäle, Software-Trigger, 1,5 ADC-Zyklen Sampling Time und ohne DMA eingerichtet. TIM2 ist initialisiert, löst aber noch keine ADC-Konversion aus. Das ist noch keine laufende Überwachung. Für eine zeitgerechte Reaktion müssen Messzeitpunkt, Einschwingzeit, Abtastfrequenz, Kanalzuordnung und Übertragung definiert werden. Die Signalerzeugung darf nicht auf eine blockierende ADC-Abfrage warten.

RailPowerLowLevel::writeSignalLevel() schützt Flag-Prüfung und GPIO-Schreibvorgang mit gespeichertem PRIMASK. RailPower::enable(), disable() und resetFault() verwenden ebenfalls kurze kritische Abschnitte. onShortCircuit() verriegelt derzeit ohne eigenen kritischen Abschnitt; vor Interrupt-Anbindung Race Conditions und Reihenfolge prüfen.

## Tatsächlicher Stand

RailPowerLowLevel und RailPower haben Implementierungen. RailShortCircuitDetector.cpp sowie CurrentSensors.cpp stehen zwar in source/libraries/custom/CMakeLists.txt, sind aber noch unvollständig. Der früher gezeigte erfolgreiche Build betraf einen älteren Dateistand. Ohne Hardware sind Stromskalierung, Abschaltzeit und Schutzwirkung unbestätigt.

Die nächsten Schritte stehen in [TODO.md](TODO.md).
