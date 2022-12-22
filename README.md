# Sprachgesteuerte Lampe Team4

### Installation & Inbetriebnahme
- STM32 & Raspberry Pi an Strom anschließen
- beide Geräte aneinander anschließen

- von einem PC aus ssh auf den Raspberry Pi
  - install pycryptodome 
  - install docker
  - install rhasspy
  - install python

- Projekt von GitHub runterladen
- C Code über z.B. Cube IDE öffnen und bauen
- auf STM32 spielen z.B. über Debugger

- Python Code auf Raspberry Pi spielen
- starten des Programms im Terminal über "python Serial.py"

Das Projekt ist jetze einsatzbereit.

### Starten der Geräte
Bei Einschalten werden diverse Module auf Funktionalität überprüft. Die LEDs leuchten/blinken nacheinander auf. Als letztes blinkt die rote LED auf dem Raspberry PI, damit ist die Überprüfung abgeschlossen. 
Sobald die grüne LED auf dem Raspberry Pi leuchtet, kann mit der ersten Sprachaufnahme gestartet werden. 

### Licht ein/ausschalten
- Drücke auf den blauebn Knopf auf dem STM32 und sage "on" oder "stop" um das Licht ein- oder auszuschalten
- Magic passiert, warte ein paar Sekunden
- die blaube LED auf dem Raspberry Pi wird sich ein/ausschalten

### Erklärung der LEDs
LEDs STM32
- grün leuchten: Gerät ist einsatzbereit
- rot leuchten: Fehlerzustand (Safe State)
- orange blinken: Sprachaufnahme läuft

LEDs Raspberry Pi
- grün leuchten: Gerät ist einsatzbereit
- rot leuchten: Fehlerzustand
- orange blinken: Daten werden übertragen
- Blau leuchten: Licht eingeschaltet
