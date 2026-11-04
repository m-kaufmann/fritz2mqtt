# Firmwareupdates – Implementierung und Prüfstand

Der Tab **System / Firmware** prüft das fest konfigurierte Manifest seines Kanals und zeigt installierte/angebotene Version, Hardwareprofil und Ergebnis. Installation verlangt ein angekreuztes Bestätigungsfeld und den Klick auf „Update installieren“. Die API verlangt dieselbe konkrete Version sowie `confirm: true`, gültige Origin, CSRF-Token und JSON. Regelmäßige Updateprüfungen lösen keine Installation aus.

Quelle: `https://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-4mb-v1/stable/manifest.json`. Noch kein stabiler Kanal veröffentlicht. Ein nicht erreichbares Manifest wird als Fehler angezeigt und erlaubt keine Installation. GitHub Pages ist für den getrennten Testkanal eingerichtet; der stabile Kanal bietet weiterhin kein Manifest an.

## Download und Erhalt der Konfiguration

ESPHome 2026.10.1 übernimmt Manifestprüfung und HTTP-OTA. TLS-Zertifikatsprüfung bleibt aktiv, SNTP stellt die für HTTPS nötige Uhrzeit bereit. Das Gateway akzeptiert ausschließlich eine neuere numerische stabile Version `MAJOR.MINOR.PATCH` (bzw. die stabile Fassung einer laufenden `-dev`-Version), eine gültige MD5-Transportprüfsumme und exakt den passenden HTTPS-Dateipfad des Hardwareprofils. Metadaten müssen innerhalb der letzten zehn Minuten erfolgreich geprüft sein. Ein geändertes Angebot erfordert eine neue Bestätigung. MD5 ist keine Herausgebersignatur.

Vor und unmittelbar bei der Ausführung werden die Freigabebedingungen erneut geprüft. Check und Installation laufen im ESPHome-Hauptloop; HTTP liest einen geschützten Statuscache. Native Fortschrittsrückmeldungen aktualisieren ihn auch während des Flashens. Laufende Thermostataufträge blockieren den Installationsstart; während einer angenommenen Installation sind neue Stellaufträge und das Speichern der Einrichtung gesperrt. Die Updatefunktion schreibt keine NVS-Konfiguration; `config_v1` und der konfigurierte Namensraum bleiben unverändert. Der tatsächliche Erhalt und Wiederanlauf wird am separaten Testgerät geprüft (Nachweis unten).

Der Browser merkt sich Zielversion und Gerätekennung nur in seiner Sitzung. Nach Verbindungsunterbrechung wird ausschließlich der Status erneut gelesen, niemals der Installationsauftrag wiederholt. Nach Neustart gilt erst die am selben Gerät zurückgemeldete Zielversion als bestätigt. Bei unklarem Ausgang bleibt die Anzeige entsprechend offen.

## API

- `GET /gateway/api/firmware`: Statuscache; keine Zugangsdaten, Firmware-URLs nicht frei änderbar.
- `POST /gateway/api/firmware/check` mit `{}`: Prüfung anfordern, HTTP 202.
- `POST /gateway/api/firmware/install` mit `{"version":"0.1.0","confirm":true}`: genau die angebotene Version freigeben, HTTP 202.
- Ungültige Payload 400, fehlende Origin/CSRF 403, fehlende Freigabe bzw. laufender Vorgang 409.

Die offene lokale Weboberfläche bleibt wie bisher ohne Benutzeranmeldung. Das ist keine kryptografische Geräteeigentümer-Authentifizierung. Der native ESPHome-OTA-Wartungsweg bleibt erhalten.

## Noch vor einem stabilen Release

Vollständiges Lizenzinventar, freigegebenes Quell-/Firmwarepaket mit Manifest, HTTPS-Hosting sowie Hardwaretest von Erstinstallation, Konfigurationserhalt, echtem Update, unterbrochenem Download, falscher Prüfsumme und Wiederherstellung. Keine Änderungen an Bootloader oder Partitionstabelle über diesen Updateweg. Ein automatisches Bootloader-Rollback wird nicht behauptet. Das bisherige produktive Gateway bleibt unverändert; Tests erfolgen auf einem separaten ESP32 mit 4 MB Flash.

## Lokale Prüfung des Änderungspakets

02.11.2026: fünf C++-Testgruppen mit ASan/UBSan erfolgreich. Die neue Update-Policy prüft Versionsrückschritte, gleiche Versionen, falsche/unsichere Quelle, falsches Hardwareprofil, fehlende Prüfsumme, abgelaufene Prüfung, Fehler-/Beschäftigtzustände und Zeitgeberüberlauf. ESPHome 2026.10.1 akzeptiert das Profil mit Updatekomponenten und erzeugt den C++-Code. JavaScript-Syntaxprüfung erfolgreich. Der vollständige CI-Build und die browsergestützte Prüfung werden vor Abnahme zusätzlich dokumentiert.

Browser-Simulation: Installationsknopf vor Bestätigung gesperrt, danach freigegeben; während Installation Knopf/Checkbox/erneute Prüfung gesperrt; Fortschritt sichtbar; nach simuliertem Neustart am selben Gerät Zielversion bestätigt und erneute Installation gesperrt. Testwerkzeug: `python3 tools/preview_updates.py`, ausschließlich 127.0.0.1, keine Geräte-/Brokerverbindung.

Neuer ESP32-Testaufbau: USB-C/CH340, ESP32-D0WD-V3, 4 MB Flash. Lokaler vollständiger Build und erste USB-Installation mit vorheriger Chip-/MAC-/Flashgrößenprüfung sowie anschließender Hashverifikation erfolgreich. Start von ESPHome 2026.10.1 / Projekt 0.1.0-dev und Einrichtungs-AP mit Adresse 192.168.4.1 über serielles Log bestätigt. Netzwerkbetrieb und echter HTTPS-Pull-OTA-Test sind inzwischen nachgewiesen (siehe unten). Die hierfür verwendeten individuellen Kennungen und Logs bleiben außerhalb des öffentlichen Projekts.

Vollständiger aktueller CI-Lauf einschließlich Firmware und Hosttests erfolgreich: https://github.com/m-kaufmann/fritz2mqtt/actions/runs/41490461626 (Commit aeeb5d3). Reale API-Negativtests: fehlende/falsche Bestätigung 400, nicht angebotene Version 409, fremde Origin 403; keine Installation gestartet, Konfigurationsfingerabdruck unverändert. Separater nativer ESPHome-OTA-Wartungstest erfolgreich: anschließender Neustart, WLAN-Wiederverbindung, identischer Konfigurationsfingerabdruck und bytegleiche zuletzt gebaute Gateway-Webseite. Diese Prüfung ist vom anschließend durchgeführten HTTPS-Pull-OTA-Test über das veröffentlichte Manifest zu unterscheiden.

## Getrennter Testkanal und Paketierung

`firmware/esp32-test.yaml` übernimmt den neutralen Build, wählt aber ausschließlich `testing/manifest.json`. Die Weboberfläche kennzeichnet diesen Kanal als Testkanal ohne allgemeine Freigabe. Der stabile Kanal wird dadurch nicht verändert. Die Bilddateien bleiben versions- und hardwaregebunden; der Testkanal benötigt dieselbe bewusste Installationsbestätigung.

`tools/prepare_test_release.py` bündelt einen numerisch versionierten Testbuild mit dem exakten Git-Quellstand, generierten Quellen, ESPHome, aufgelösten Managed Components und dem ESP-IDF-SDK. Lizenz-/Copyrightdateien werden unverändert mitgeliefert; Buildinformationen werden ohne lokale Rechnerpfade normalisiert. Der Vorgang liest weder Geräte-NVS noch persönliche Konfigurationen. Ausgaben liegen außerhalb des Git-Repositories. Prüfung vor Commit: Hosttests, Paketgrenzen, Python-Syntax und Konfiguration beider Profile erfolgreich.

Die Quellenpaketierung behält relative SDK-Verweise innerhalb des jeweiligen Quellbaums bei und verweigert Verweise nach außerhalb. Rechnerbezogene Installer-Metadaten, Git-Verzeichnisse und Python-Cachedateien werden ausgeschlossen. Archiv und separate Lizenzsammlung werden vor dem Upload tatsächlich erzeugt und mit SHA-256 inventarisiert.

## Echter HTTPS-Pull-OTA-Test am 02.11.2026

Erfolgreich auf dem separaten ESP32/4-MB-Testgerät: `0.1.0-dev` → `0.1.0`, Kanal `testing`. Testpaket: [v0.1.0-test.1](https://github.com/m-kaufmann/fritz2mqtt/releases/tag/v0.1.0-test.1), Firmware-Quellstand `ccd4e61289ed942caa54bbaeaf336f886f06fb8d`.

GitHub Pages lieferte das Manifest und die exakt passende OTA-Datei über HTTPS; SHA-256 und MD5 der tatsächlich ausgelieferten Datei wurden vorab mit dem lokalen Build verglichen. Das Gateway fand das Angebot über „Nach Updates suchen“. Nach angekreuzter Bestätigung startete genau ein Klick auf „Update installieren“ den selbstständigen Download. Fortschritt und gesperrte Bedienelemente waren sichtbar. Anschließend meldete derselbe ESP die Version `0.1.0`; der Browser bestätigte „Version 0.1.0 läuft nach dem Neustart.“ Der Zielbuild wurde dabei nicht über den nativen Wartungsupload installiert.

Der Fingerabdruck der maskierten Konfigurations-API vor und nach dem Update war identisch, einschließlich Revision, WLAN-/Zugangskennzeichen und Thermostatzuordnung. Sämtliche MQTT-Topics blieben leer; keine Thermostat-Stellbefehle wurden gesendet. Individuelle Gerätekennungen, WLAN-Namen und Konfigurationsdaten bleiben außerhalb dieses öffentlichen Nachweises.

Das Paket enthält Factory-/OTA-Datei, Manifest, SHA256SUMS, Buildmetadaten, aufgelöste Abhängigkeiten, zugehörige Projekt-/ESPHome-/ESP-IDF-/Managed-Quellen und 55 ursprüngliche Lizenz-/Copyrightdateien. Der Testkanal bietet ausschließlich dieses Profil an; ein stabiles Manifest wurde nicht veröffentlicht. Downloadabbruch, absichtlich falsche Prüfsumme, Wiederherstellung und vollständiger WLAN-Ausfalltest bleiben vor stabiler Freigabe offen.

Wiederanlaufbefund: MQTT war nach dem Neustart wieder verbunden, ohne empfangene Stellbefehle. Die FRITZ!Box-Anmeldung gelang nach einem zunächst fehlgeschlagenen HTTP-Verbindungsversuch; die anschließende Geräteabfrage meldete jedoch weiterhin einen HTTP-Fehler. Damit sind Download, Installation, Versionsbestätigung und Konfigurationserhalt nachgewiesen, ein durchgehend erfolgreicher FRITZ!Box-Wiederanlauf nach diesem Test noch nicht. Diese bereits im Netzwerktest beobachtete Fehlerklasse bleibt separat zu untersuchen; keine stabile Freigabe daraus ableiten.

Weiterführende [HTTP-Analyse und Korrektur des WLAN-Startablaufs](http-diagnostics.md): Der nach dem OTA beobachtete frühe HTTP-0-Fehler ist reproduziert und im Entwicklungsbuild behoben; reale Header-Timeouts und normale Socket-Endemeldungen werden getrennt erfasst.

Weitere [Netzwerkdiagnose mit nativen Ereigniszählern und Resetgrund](network-diagnostics.md) trennt beobachtete Erreichbarkeitsverluste von tatsächlichen Neustarts.
