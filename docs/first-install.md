# Erstinstallation und Einrichtung

Dieser Ablauf wurde mit einem separaten ESP32 NodeMCU (USB-C, CH340, ESP32-D0WD-V3, 4 MB Flash) erprobt. Ein bestehendes Gateway kann währenddessen unverändert weiterlaufen. FRITZ2MQTT benötigt im Betrieb weder Home Assistant noch ein bestimmtes Automationssystem.

## 1. Board und USB-Verbindung prüfen

Ein USB-Datenkabel verwenden. Der erprobte CH340 wird auf dem Mac als `/dev/cu.usbserial-…` angezeigt; der Portname kann auf jedem Rechner anders sein. Der CH340 ist der USB-Seriell-Wandler, das Firmwareziel bleibt der ESP32.

```sh
python -m pip install esphome==2026.10.1
python -m serial.tools.list_ports -v
python -m esptool --port <DEIN_USB_PORT> flash-id
```

Vor dem Schreiben Chiptyp, MAC-Adresse und Flashgröße notieren. Das hier angebotene Profil ist für ESP32 mit **4 MB Flash**. USB-C alleine sagt nichts über Chipfamilie oder Flashgröße aus. Bei mehreren angeschlossenen Boards das neue Gerät eindeutig zuordnen.

## 2. Firmware erstmals über USB installieren

Aus den Quellen des gewünschten Versionsstands:

```sh
esphome config firmware/esp32.yaml
esphome compile firmware/esp32.yaml
esphome upload firmware/esp32.yaml --device <DEIN_USB_PORT>
```

Für ein separates OTA-Testgerät stattdessen **`firmware/esp32-test.yaml`** verwenden. Dieses Profil fragt ausschließlich den Testkanal ab. Der normale Build verwendet den stabilen Kanal.

Wenn ein passendes fertiges Paket unter [GitHub Releases](https://github.com/m-kaufmann/fritz2mqtt/releases) angeboten wird: Für die erste USB-Installation ist **`firmware.factory.bin`** bestimmt, nicht `firmware.ota.bin`.

```sh
python -m esptool --port <DEIN_USB_PORT> write-flash 0x0 firmware.factory.bin
```

Das Flashwerkzeug muss die geschriebenen Daten anschließend erfolgreich verifizieren. Bei einem Verbindungsproblem gegebenenfalls BOOT gedrückt halten, den Verbindungsaufbau starten und BOOT nach Erkennung loslassen. Danach das Board normal neu starten.

## 3. Einrichtung über den Access Point

1. Mit dem WLAN **`FRITZ2MQTT-xxxxxx`** verbinden. Die Endung ist geräteabhängig. Der Einrichtungs-AP ist ohne Passwort.
2. Im Browser ausdrücklich **`http://192.168.4.1/`** öffnen; das geht beispielsweise vom Handy.
3. Unter **Einrichtung → WLAN und Einrichtungs-AP** zunächst nur das Standort-WLAN eintragen und speichern. Ein zweites WLAN-Profil ist optional.
4. Nach dem Verbindungswechsel wieder mit dem normalen Netzwerk verbinden. Das Gateway erhält dort eine Adresse vom Router und ist unter **`http://fritz2mqtt-xxxxxx.local/`** erreichbar, sofern mDNS im Netzwerk unterstützt wird. Alternativ die IP-Adresse im Router oder seriellen Startprotokoll nachsehen.

Die ersten Tests haben bestätigt: Der AP startet, die WLAN-Einstellung wird gespeichert und bleibt nach Neustart und OTA-Wartungsupload erhalten. Die vollständige Ausfallprobe beider gespeicherten WLANs ist gesondert zu prüfen. Ohne erreichbares Standort-WLAN ist der Fallback-AP nach 60 Sekunden vorgesehen.

## 4. FRITZ!Box und MQTT einrichten

FRITZ!Box-IP sowie einen FRITZ!Box-Benutzer mit Smart-Home-Rechten eintragen. Die Thermostate müssen bereits an dieser FRITZ!Box angemeldet sein. Das Gateway findet sie anhand ihrer AIN. Bis zu acht Geräte können zugeordnet werden.

Beim MQTT-Server **nur den Hostnamen** eintragen, ohne `http://`. Port und TLS müssen zur Broker-Konfiguration passen. Häufige Kombinationen sind **1883 ohne TLS** und **8883 mit TLS**, sie sind jedoch vom tatsächlichen Broker abhängig. Die Schrift `esp-tls` im ESP-IDF-Log allein beweist nicht, dass die TLS-Checkbox aktiv ist. Bei Zeitüberschreitungen die gespeicherte Kombination prüfen.

Die Brokerverbindung funktioniert auch ohne Topics. Für erste Netzwerktests können sämtliche Topics leer bleiben. Bei einem zweiten Testgerät keine produktiven Befehls-Topics abonnieren: Sonst würden mehrere Gateways denselben Befehl verarbeiten. Zunächst einen eigenen Topicbereich verwenden.

Für ein zugeordnetes Thermostat die Rückmeldungen IST, gemeldetes SOLL und Batterie sowie das Befehls-Topic einzeln festlegen. Vorgeschlagene Topics lassen sich ändern. Payloads sind Dezimalzahlen; `9.9` bedeutet „kein Stellauftrag“ und wird nicht an die FRITZ!Box weitergereicht.

Leere Passwortfelder behalten bei unveränderter Verbindung das gespeicherte Passwort. Die Webseite liest Passwörter nicht zurück. Für ein wirklich offenes WLAN die entsprechende Auswahl verwenden. Einstellungen werden auf dem ESP gespeichert; Firmware und Quellen enthalten keine Standort-Zugangsdaten.

## 5. Diagnose und Firmwareupdates

Unter **Diagnose** FRITZ!Box-Anmeldung, aktuelle Messwerte und MQTT-Verbindung prüfen. „Verbunden“ beim Broker bestätigt noch keine Topiczuordnung. Eine Sollwertbestätigung der FRITZ!Box bestätigt keine physische Ventilbewegung.

Unter **System / Firmware** werden installierte und angebotene Version sowie der Kanal angezeigt. **Nach Updates suchen** installiert nichts. Erst die Bestätigung der angebotenen Version und **Update installieren** starten den Download und anschließend einen Neustart. Währenddessen nicht mehrfach absenden. Nach dem Neustart muss dieselbe Gerätekennung die Zielversion melden. Details: [Firmwareupdates](updates.md).

Eine als Vorabversion veröffentlichte Testfirmware ist keine allgemeine stabile Freigabe. Ein fehlendes Updateangebot oder ein noch nicht veröffentlichter Kanal darf nicht durch eine frei eingegebene Download-URL umgangen werden.
