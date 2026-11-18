# ControlUnit – TODO

Stand: 17. November 2026. Diese Liste basiert auf den heute gelesenen Projektdateien. Hardwaremessungen stehen noch aus.

## Nächster Schritt: Kurzschlussüberwachung

- [ ] RailShortCircuitDetector.cpp vervollständigen: overLimitCount_ im Konstruktor initialisieren; check() soll einen definierten bool zurückgeben und nur gültige HIGH-Proben oberhalb des Grenzwerts zählen; reset() löscht den Zähler. LOW-Abschnitte sind keine Entwarnung.
- [ ] CurrentSensors.h/.cpp vervollständigen. Die Dateien existieren und stehen bereits in der Custom-CMake-Liste; die Klasse RailCurrentSensor und die CPP-Datei sind noch leer.
- [ ] Shunt-Widerstände, INA180-Verstärkung, ADC-Referenz, Bahnstromgrenze und erlaubte Abschaltzeit am realen Schaltplan und den Anforderungen festlegen.
- [ ] ADC-Messpfad entwerfen: PA0 ist Versorgung (ADC_IN0), PA1 die Bahn (ADC_IN1). Der Iststand: 12 Bit, Software-Trigger, 1,5 ADC-Zyklen Sampling Time, beide Kanäle konfiguriert, kein DMA. Geeignetes Timing, Trigger und Messwertübernahme festlegen.
- [ ] Bahnprobe mit dem tatsächlichen PA8-Pegel bei der Konversion verknüpfen; bei einem Puffer nicht erst bei späterer Auswertung PA8 lesen. Abstand zu Schaltflanken vorsehen.
- [ ] Ungültige, ausbleibende oder veraltete Messungen behandeln; nicht als gültige Werte unter dem Grenzwert ausgeben.
- [ ] Stromsensor, Detector und RailPower::onShortCircuit() verbinden. Die Methode ist vorhanden; den gleichzeitigen Zugriff mit enable() und Signal-Interrupts prüfen.
- [ ] Rücksetzung koordinieren: Detector-Zähler löschen, Fault quittieren, Ausgang zunächst OFF lassen, anschließend bewusst freigeben. resetFault() prüft den physischen Kurzschluss derzeit nicht.

## Danach

- [ ] Zeitkritische Carrera-Signalerzeugung ohne blockierende ADC-Abfrage im Flankenpfad implementieren.
- [ ] Versorgungsshunt PA0 unabhängig vom Bahn-Kurzschluss überwachen.
- [ ] Wireless-Receiver auf PA6 WIRELESS_CLK (Ausgang) / PA7 WIRELESS_DATA (Eingang) einbinden und Protokoll auswerten.
- [ ] Anwendung in source/app/ControlUnit.cpp verdrahten: ControlUnit_Init() und ControlUnit_Tick() sind leer.
- [ ] Aktuellen Stand bauen. Der vorher gezeigte erfolgreiche Build enthielt RailShortCircuitDetector.cpp und CurrentSensors.cpp noch nicht; CMake enthält inzwischen beide.
- [ ] Nach Verfügbarkeit der Hardware Stromskalierung, Timing, Grenzwerte und reale Abschaltzeit überprüfen.
