# FRITZ2MQTT

Ein eigenständiges ESP32-Gateway zwischen FRITZ!Box-Heizkörperthermostaten und MQTT. Die FRITZ!Box übernimmt DECT; der ESP32 liest ihre lokale AHA-Schnittstelle und übersetzt MQTT-Stellaufträge. Weder Home Assistant noch ein bestimmtes übergeordnetes Automationssystem sind zum Betrieb erforderlich.

**Entwicklungsstand:** Eigenständiger Gateway-Kern mit separatem OTA-Testkanal. Das [erste Testpaket](https://github.com/m-kaufmann/fritz2mqtt/releases/tag/v0.1.0-test.1) enthält Firmware, zugehörige Quellen, Lizenztexte und Prüfsummen. Noch keine allgemeine stabile Freigabe; offene Hardwareprüfungen sind in der Updatebeschreibung dokumentiert.

## Funktionen

- Bis zu acht Thermostate, eindeutige Zuordnung über AIN.
- IST- und gemeldete SOLL-Temperatur sowie Batterie in Prozent.
- Neue Sollwerte über frei konfigurierbare MQTT-Topics, 8–28 °C in 0,5-Grad-Schritten.
- Der reservierte MQTT-Wert `9.9` bedeutet „kein Stellauftrag“ und wird nicht weitergereicht.
- Lokale Webseite mit Bedienung, Einrichtung und Diagnose; keine Cloud-Abhängigkeit im laufenden Betrieb.
- Zwei WLAN-Profile und offener Einrichtungs-AP nach 60 Sekunden ohne verbundenes WLAN.
- WLAN-, FRITZ!Box-, MQTT- und Gerätezuordnungen im nichtflüchtigen Speicher.
- Zwei LEDs direkt an GPIO25 und GPIO26, jeweils mit eigenem 470-Ω-Vorwiderstand.

## Hardware

AZ-Delivery Dev Kit C / ESP32-WROOM-32; Buildprofil zunächst ESP32 mit 4 MB Flash. [Schaltplan](hardware/wiring.svg), [Gehäuse-STL](hardware/enclosure.stl), [Aufbau](hardware/README.md).

## Ersteinrichtung

Die [Schritt-für-Schritt-README zur Erstinstallation](docs/first-install.md) beschreibt USB-Erkennung, Boardprüfung, Factory-Datei, AP, zwei WLAN-Profile, FRITZ!Box, MQTT und Firmwareupdates anhand des erprobten Ablaufs.

## Aus Quellen installieren

Mit **ESPHome 2026.10.1**:

```sh
python -m pip install esphome==2026.10.1
esphome config firmware/esp32.yaml
esphome compile firmware/esp32.yaml
esphome upload firmware/esp32.yaml
```

Die Erstinstallation erfolgt über USB. Spätere ESPHome-Uploads können OTA erfolgen. Die generische Firmware enthält keine Standort-Zugangsdaten und keinen gemeinsamen API-Schlüssel. Im WLAN `FRITZ2MQTT-<Gerätekennung>` die Einrichtung unter `http://192.168.4.1/` öffnen. Webseite und AP sind in diesem Profil ohne eigene Anmeldung eingerichtet. FRITZ!Box-Adresse und Benutzer mit Smart-Home-Rechten, WLAN und MQTT werden dort eingetragen. Danach Thermostate zuordnen und deren Topics festlegen.

Die vorgeschlagenen Topics sind Beispiele; jedes Topic ist einzeln änderbar:

```text
fritz2mqtt/site/1/thermostat_t_ist       # Temperatur-Rückmeldung
fritz2mqtt/site/1/thermostat_t_soll      # Sollwert-Rückmeldung
fritz2mqtt/site/1/thermostat_battery     # Batterie, %
fritz2mqtt/site/1/thermostat/set         # Neuer Sollwert
```

Der MQTT-Host ist ein Hostname ohne `http://`. Rückmeldungen: QoS 1, retained. Befehle: QoS-0-Abonnement, kein Ausführen retained/duplizierter/ungültiger Nachrichten. Ein angenommener Auftrag wird einmalig geschrieben und zurückgelesen. Die Zeitschaltung der FRITZ!Box bleibt wirksam. Eine Sollwertbestätigung ist keine Messung der Ventilbewegung.

## Entwicklung und Updates

[Entwicklungs-/Releaseplan](docs/releases.md), [Lizenzhinweise](THIRD_PARTY_NOTICES.md), [Änderungsprotokoll](CHANGELOG.md).

Vor Veröffentlichung fertiger Firmware: vollständiger Build, Tests auf Hardware, Lizenzinventar der tatsächlich eingebundenen Abhängigkeiten, Wiederherstellung und Erhalt bestehender Konfiguration prüfen. Der Tab „System / Firmware“ implementiert die HTTPS-Updateprüfung und eine ausdrücklich bestätigte Installation; siehe [Updatebeschreibung](docs/updates.md). Der stabile Updatekanal ist noch nicht veröffentlicht; Testkanal und Hardwareprüfstand sind dort getrennt dokumentiert.

Hosttests auf macOS: `bash tools/test.sh` (Python 3, clang++, Expat und Node.js). Linux-Firmwarebuilds verwenden die tatsächlichen ESP-IDF-Bibliotheken; die CommonCrypto-Adapter unter `tests/mbedtls/` gehören ausschließlich zu den macOS-Hosttests.

## Lizenz

Projektlizenz: [GPLv3](LICENSE). Nutzung, Änderung und Weitergabe sind unter deren Bedingungen möglich, auch kommerziell. Fremde Bestandteile behalten ihre eigenen Lizenzbedingungen. Dieses Repository enthält keine persönlichen Gerätekonfigurationen oder Zugangsdaten.
