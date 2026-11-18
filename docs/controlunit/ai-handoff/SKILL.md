---
name: continue-controlunit
description: Übernahme und Weiterentwicklung der STM32L071-ControlUnit in C++ für Carrera-Signal, ADC-Strommessung, Kurzschlusserkennung und Wireless-Receiver. Bei Arbeiten an diesem Projekt lesen.
---

# ControlUnit fortsetzen

Lies zuerst ../ARCHITECTURE.md und ../TODO.md; vergleiche dann alle Aussagen mit dem tatsächlichen Quellcode, der IOC-Datei und CMake. Die Dokumente können nach späteren Änderungen veralten. Antworte Nico auf Deutsch per du. Respektiere seinen jeweils aktuellen Wunsch nach Erklärung oder Code; generiere nicht ungefragt Code, wenn er explizit „ohne Code“ sagt.

## Architektur einhalten

- CubeMX/HAL unter source/libraries/core lassen; eigene Logik unter source/app und source/libraries/custom/Rails. Generierte Dateien nur in USER CODE-Bereichen erweitern.
- RailPowerLowLevel steuert und verriegelt PA8, RailPower besitzt die Zustände OFF/ON/SHORT_CIRCUIT, RailCurrentSensor rechnet ADC-Rohwerte um, RailShortCircuitDetector prüft den Grenzwert, die Anwendung verbindet die Komponenten.
- PA1 ist Bahnstrom, PA0 Versorgungsstrom. Einem Bahnstromwert den PA8-Zustand **zum Messzeitpunkt** zuordnen. LOW-Phasen nicht als entlastende HIGH-Messung zählen.
- Bei Fehler PA8 LOW verriegeln; Quittierung und erneute Freigabe getrennt halten. Nebenläufigkeit zwischen Signal- und Fehlerpfad beachten.
- Shunt, INA180-Verstärkung, ADC-Referenz, Schwellen und Abschaltzeit an Schaltplan und Anforderungen überprüfen; keine Zahlen erfinden. Keine Hardware-Verifikation behaupten.

## Stand bei der Übergabe

- RailPowerLowLevel und RailPower sind implementiert. RailPower::onShortCircuit() existiert, enthält aber anders als enable()/disable() keinen eigenen kritischen Abschnitt.
- RailShortCircuitDetector.cpp ist in der Custom-CMake-Datei gelistet; check() und reset() sind noch leer, overLimitCount_ ist nicht initialisiert.
- CurrentSensors.h definiert eine leere RailCurrentSensor-Klasse; CurrentSensors.cpp ist leer. Beide sind in CMake gelistet.
- ADC1 hat PA0 und PA1, 12 Bit, Software-Trigger, kein DMA; TIM2 ist initialisiert, aber nicht als ADC-Trigger verbunden.
- ControlUnit_Init() und ControlUnit_Tick() sind leer. Ein früherer erfolgreicher Build erfasste die beiden neuen CPP-Dateien noch nicht.

Arbeite entlang der TODO-Datei. Nach Änderungen an der Implementierung den aktuellen Build prüfen und die Dokumente aktualisieren. Ohne Hardware nur die tatsächlich überprüften Softwareeigenschaften melden.
