# Änderungen

## 0.1.17 – Stable (16.11.2026)

- Unterstützung für FRITZ!Smart Energy 200 über die FRITZ!Box. Steckdosen und Thermostate lassen sich gemeinsam in einem Gateway zuordnen.
- Steckdosenkachel mit eingerichtetem Anzeigenamen und großem Ein/Aus-Schalter, der den gemeldeten Schaltzustand anzeigt. Netzspannung und Energiezähler erscheinen mit einer Nachkommastelle.
- Eigene MQTT-Topics für Schaltbefehl, Schaltzustand, Momentanleistung, Energiezähler, Netzspannung und Temperatur.

## 0.1.16 – Stable (15.11.2026)

- „Topics vorbelegen“ ist im aufgeklappten Bereich „Datenpunkte und Topics“ sichtbar. Bereits geöffnete Bereiche bleiben beim Hinzufügen und Entfernen anderer Geräte offen.
- Während eines Stellauftrags ändert nur der zugehörige „Übernehmen“-Button seine Farbe. Weitere Stellaufträge sind bis zum Abschluss gesperrt.
- Der gespeicherte MQTT-Topic-Präfix steht beim späteren Konfigurieren weiterer Thermostate wieder zur Verfügung. Die vorgeschlagenen Topics lassen sich pro Thermostat bearbeiten.

## 0.1.14 – Stable (15.11.2026)

- WLAN-Einrichtung und Standortwechsel nutzen dieselbe Gateway-Oberfläche. Beide WLAN-Profile werden vor dem Neustart dauerhaft gespeichert.
- WLAN-Suche im Einrichtungs-AP und im laufenden Betrieb. Netzname, Signalstärke und Schutzstatus stehen in der Auswahlliste. Die SSID kann weiterhin von Hand eingegeben werden.
- Der MQTT-Topic-Präfix lässt sich dauerhaft einstellen und für neue Topic-Vorschläge verwenden.
- Gespeicherte Passwörter zeigen einen Platzhalter. Buttons sind gruppenweise ausgerichtet und der Speicherbutton wird bei Änderungen aktiv.
- Diagnose und Firmware-Übersicht zeigen Bezeichnungen und Werte zweispaltig. Statussymbole kennzeichnen Verbindung, Prüfung, Warnung und Fehler.
- Heizkörper-Icon für den iOS-Home-Bildschirm und den Browser-Tab. Das Favicon war im Test in Chrome sichtbar. Safari zeigte im Tab weiterhin sein Ersatzsymbol.
- Stabiles OTA-Angebot für `esp32-4mb-v1` mit Factory- und OTA-Datei, exakt zugeordneten Build-Quellen, Lizenztexten und SHA-256-Prüfsummen.

## 0.1.4-dev – erweiterte Netzwerkdiagnose

- Native WLAN-/IP-/AP-Ereigniszähler, Resetgrund und Hauptloop-Pausen in der Status-API.
- HTTP-Fehler um unmittelbares errno, Transportdetails und AP-Kontext ergänzt.
- Ereignis-Callbacks verwenden ausschließlich atomare Zähler.
- Intel-macOS-Runner für die CommonCrypto-Hosttests statt des kapazitätsbeschränkten ARM-Pools.
- Diagnoseentwicklung; Ursache wiederholter Nichterreichbarkeit weiterhin in Untersuchung. Kein neues Firmware-Release.

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
