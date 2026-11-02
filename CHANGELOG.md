# Änderungen

## 0.1.1-dev – HTTP-Diagnose am Testgerät

- AHA-Abfragen erst bei bestehender WLAN-/IP-Verbindung; unmittelbarer Anstoß nach Verbindung statt unnötiger Start-Fehlerwartezeit.
- Netzwerkwartezeit acht Sekunden, zusätzlich vor jeder HTTP-Phase auf das verbleibende 20-Sekunden-Budget begrenzt.
- Geheimnisfreier HTTP-Statuscache mit letzter Transaktion und letztem Fehler; getrennte Fehlercodes für Netzwerk-/Lesephase.
- Bestehende Grenzen für Sollwerte, Gültigkeitsdauer und einmaligen Schreibversuch unverändert.
- Nur als Entwicklungsbuild am separaten Testgerät; bestehendes Prerelease-Paket 0.1.0 unverändert.

## 0.1.0-dev – noch nicht freigegeben

- Eigenständiger Gateway-Kern mit neutralen Werkseinstellungen und frei wählbaren MQTT-Topics.
- Konfigurierbarer NVS-Namensraum für den Erhalt bestehender Installationen.
- Bisheriger AHA-, MQTT-, WLAN- und HTTP-Body-Code mit Hosttests übernommen.
- Quellbuildprofil für ESP32-WROOM-32, zwei Status-LEDs und Gehäusedokumentation.
- System/Firmware-Tab: explizite Updateprüfung, Bestätigung und Fortschritt mit Neustartkontrolle.
- HTTPS-Quelle und Hardwareprofil fest gebunden; alte Versionen, veraltete Metadaten und ungültige Prüfsummen gesperrt.
- Noch kein stabiler Firmwaredownload; Hosting und echte OTA-Abnahme stehen aus.
