# ControlUnit – TODO

Stand: 23. November 2026. Hardwaremessungen stehen noch aus.

## Nächster Schritt: Kurzschlussüberwachung

- [x] Detektor und Zähler implementieren. Nur HIGH-Proben zählen. Bei erkanntem Kurzschluss `HAL_ERROR` zurückgeben.
- [x] Stromsensor mit ADC-Kalibrierung, DMA und Umrechnung in mA implementieren.
- [ ] Shunt-Widerstände, INA180-Verstärkung, ADC-Referenz, Bahnstromgrenze und erlaubte Abschaltzeit am realen Schaltplan und den Anforderungen festlegen.
- [x] ADC-Messpfad für PA0 und PA1 mit TIM2 CH4 und DMA einbinden.
- [x] PA8 im TIM2-CH4-Interrupt lesen und den Pegel mit der Bahnprobe an die Auswertung übergeben.
- [ ] Bei der Carrera-Signalerzeugung sicherstellen und messen, dass PA8 vom Trigger bis zur PA1-Abtastung stabil ist und genügend Abstand zur Schaltflanke besteht.
- [ ] Ungültige, ausbleibende oder veraltete Messungen behandeln; nicht als gültige Werte unter dem Grenzwert ausgeben.
- [x] Stromsensor, Detektor und `RailPower::onShortCircuit()` über `ControlUnit::tick()` verbinden.
- [ ] Vor der Interrupt-Anbindung der Signalausgabe den gleichzeitigen Zugriff auf `RailPower` prüfen.
- [ ] Rücksetzung koordinieren: Detector-Zähler löschen, Fault quittieren, Ausgang zunächst OFF lassen, anschließend bewusst freigeben. resetFault() prüft den physischen Kurzschluss derzeit nicht.

## Danach

- [ ] Zeitkritische Carrera-Signalerzeugung ohne blockierende ADC-Abfrage im Flankenpfad implementieren.
- [ ] Versorgungsshunt PA0 unabhängig vom Bahn-Kurzschluss überwachen.
- [ ] Wireless-Receiver auf PA6 WIRELESS_CLK (Ausgang) / PA7 WIRELESS_DATA (Eingang) einbinden und Protokoll auswerten.
- [x] Initialisierung und Auswertung in der App aufrufen.
- [x] Debug-Firmware mit der Bahnproben-Auswertung und dem TIM2-Interrupt bauen.
- [ ] Nach Verfügbarkeit der Hardware Stromskalierung, Timing, Grenzwerte und reale Abschaltzeit überprüfen.

## USART2 und MOXA-Protokoll

- [x] USART2-Empfang per Interrupt starten und vollständige Wörter für die Hauptschleife bereitstellen.
- [ ] Wörter in `ControlUnit_Tick()` auswerten und Antworten senden.
- [ ] Empfang und Neustart der Kommunikation am Board mit dem Moxa prüfen.
