# ControlUnit

## Aufbau

| Pfad | Inhalt |
| --- | --- |
| `source/libraries/core/` | STM32CubeMX-Projekt (`ControlUnit.ioc`), generierte `Core/`- und `Drivers/`-Dateien, Startup, Linkerskript und CMake |
| `source/libraries/custom/` | Eigene wiederverwendbare Bibliotheken |
| `source/app/` | Anwendungscode in C++ (`ControlUnit.cpp`) |
| `build/` | Build-Ausgaben |

CubeMX über `source/libraries/core/ControlUnit.ioc` öffnen. Änderungen an generierten Dateien nur innerhalb der `USER CODE`-Bereiche machen. `main.c` initialisiert Hardware und ruft danach `ControlUnit_Init()` und in der Hauptschleife `ControlUnit_Tick()` auf. Eigene Logik kommt nach `source/app/`, wiederverwendbare Komponenten nach `source/libraries/custom/`.

In VS Code den Projektstamm öffnen; CMake Tools verwendet `source/libraries/core` als Quellverzeichnis. Mit `cmake --preset Debug -S source/libraries/core` und `cmake --build build/Debug` aus dem Projektstamm bauen. Arm GNU Toolchain und Ninja müssen im PATH verfügbar sein.

## Git

Das Repository hat noch keinen Commit. `origin` ist bereits eingerichtet; das Remote enthält noch keinen Branch. Für den ersten Push:

```sh
git add .
git status --short
git commit -m "Initialer ControlUnit-Stand"
git branch -M main
git push -u origin main
```

Die verbindliche CubeMX-Datei ist `source/libraries/core/ControlUnit.ioc`. Die älteren Kopien `ControlUnit.ioc` und `cmake/` im Projektstamm sowie `build/` werden nicht versioniert. `Core/`, `Drivers/`, das Linkerskript, die CMake-Dateien und der eigene C++-Code gehören ins Repository.

Stand vor dem ersten Push: Der Debug-Build funktioniert. `RailShortCircuitDetector::check()` und `reset()` sind noch nicht implementiert; ADC-Timing und Kurzschlussabschaltung sind ohne Hardware nicht verifiziert.
