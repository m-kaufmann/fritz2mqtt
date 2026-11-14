# HTTP-Diagnose und Startreihenfolge

## Nachgewiesene Fehlerklassen

Bei der Untersuchung am separaten ESP32-Testgerät wurden unterschiedliche Vorgänge getrennt:

- Eine frühe Challenge-Anfrage begann bereits 2,95 Sekunden nach dem Start ohne verbundene WLAN-Station. Der TCP-Aufbau scheiterte nach 5 ms mit `28674` (`ESP_ERR_HTTP_CONNECT`), ohne HTTP-Status oder Antwortdaten. Die anschließende AHA-Fehlerwartezeit verzögerte die Erholung unnötig.
- Ein weiterer Versuch scheiterte bei bestehender WLAN-Verbindung und etwa −50 dBm beim Lesen der Anmelde-Header nach 4018 ms mit `-28679` (`-ESP_ERR_HTTP_EAGAIN`). Das ist eine tatsächlich abgelaufene Netzwerkwartezeit, kein Nachweis für falsche Zugangsdaten. Die Ursache der verspäteten Antwort (Router oder Übertragungsweg) lässt sich daraus allein nicht bestimmen.
- Native Meldungen `esp_transport_read returned:-1 and errno:128` wurden auch während erfolgreich bestätigter Stellaufträge beobachtet. Die verwendete FRITZ!Box liefert XML ohne Content-Length/Chunking mit `Connection: close`. Vollständige Antwort und gültiges XML entscheiden über Erfolg; eine einzelne native Socketmeldung reicht dafür nicht.

Vergleichsmessungen vom Rechner: 15/15 ICMP-Antworten ohne Verlust; TCP-Verbindungsaufbau 1–4 ms, HTTP-Challenge-Header meist 0,7–1,8 Sekunden. Diese Momentaufnahme schließt sporadische WLAN-/Routerausfälle nicht aus. Ein vom Anwender beobachteter zeitnaher Ausfall eines zweiten Gateways ist ein Hinweis auf gemeinsame Infrastruktur, jedoch kein Beweis. Dieses zweite Gateway wurde nicht angesprochen oder verändert.

## Korrektur im Entwicklungsbuild

Der AHA-Worker beginnt Lesezyklen erst, wenn der Hauptloop eine vollständige WLAN-Verbindung einschließlich IP-Konfiguration bestätigt. Ein Verbindungsaufbau stößt die Abfrage direkt an. Ohne WLAN werden keine AHA-Loginversuche und keine dadurch verursachten Fehlerwartezeiten erzeugt. Bereits vorhandene Sperrzeiten einer echten fehlgeschlagenen Anmeldung werden nicht aufgehoben. Stellaufträge ohne bestätigte Netzwerkbereitschaft werden abgelehnt und niemals automatisch nachgesendet.

Die Netzwerkwartezeit beträgt acht statt vier Sekunden. Vor Schreiben, Headerabruf und jedem Body-Lesen wird sie zusätzlich auf das verbleibende gemeinsame 20-Sekunden-Budget begrenzt. Der SDK-Timeout wird damit an den Phasengrenzen ebenfalls durch das verbleibende Budget begrenzt. Die 15-Sekunden-Gültigkeit eines noch nicht ausgeführten Stellauftrags und der einmalige Schreibversuch bleiben erhalten.

## Diagnose ohne Zugangsdaten

`GET /gateway/api/status` enthält zusätzlich `http.requests`, `http.failures`, `http.last` und `http.last_failure`. Gespeichert werden nur feste Operationsnamen, Phase, Zeitpunkt seit Start, Dauer, Status-/Fehlercode, Byteanzahl, letzter Read-Rückgabewert, WLAN-Assoziation/RSSI und freier Heap. Keine URLs, Header, Antwortinhalte, Benutzernamen, Passwörter oder Session-IDs. Der letzte Fehler bleibt bis zum Neustart auch nach späterem Erfolg abrufbar. Zähler gelten ausschließlich für den aktuellen Start und betreffen HTTP-Transport/-Status; XML- oder Gerätefehler erscheinen weiterhin in der bestehenden Diagnose.

Warnlogs enthalten echte fehlgeschlagene HTTP-Transaktionen, Debuglogs auch erfolgreiche. Diese Informationen erlauben bei späteren Bugmeldungen die Trennung von Startreihenfolge, TCP-Aufbau, Headerwartezeit, unvollständigen Daten und Authentifizierung. Individuelle Netz-/Gerätekennungen und Rohprotokolle gehören nicht in öffentliche Issues.

Quellen: [Espressif HTTP-Client-API](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-reference/protocols/esp_http_client.html) und der tatsächlich gebaute ESP-IDF-5.5.6-Quellstand im Quellenpaket. Der native Wartungsupload des Diagnosebuilds ist vom zuvor erfolgreich geprüften HTTPS-Pull-OTA zu unterscheiden.

## Hardwareprüfung am 02.11.2026

Vollständiger ESP32/4-MB-Build mit ESPHome 2026.10.1 und Hosttests (ASan/UBSan, Protokoll, Konfiguration, MQTT, Request-Body, Update-Policy, Paketgrenzen und JavaScript) erfolgreich. Diagnosebuild `0.1.1-dev` über den nativen Wartungsweg ausschließlich am separaten Testgerät installiert; noch kein neues Releasepaket oder stabiles Updateangebot.

Erster korrigierter Neustart: frische Thermostatwerte und MQTT nach 19 Sekunden, drei erfolgreiche HTTP-Transaktionen ohne Fehler. Anschließend bestätigte die FRITZ!Box die Sollwertfolge 20 → 21 → 20 °C. Bis zur anschließenden Wiederholungsprüfung elf erfolgreiche HTTP-Transaktionen und kein Transport-/Statusfehler. Die gespeicherte Konfiguration blieb unverändert.

Zweiter korrigierter Neustart desselben Builds: erneut frische Werte und MQTT nach 19 Sekunden, drei HTTP-Transaktionen ohne Fehler; gemeldeter Sollwert weiterhin 20 °C. Der Konfigurationsfingerabdruck wurde erneut verglichen. Zwei erfolgreiche Wiederanläufe sind ein gezielter Regressionstest, keine Langzeitmessung der gemeinsamen Netzwerkinfrastruktur.
