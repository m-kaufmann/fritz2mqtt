# FRITZ2MQTT

Ein eigenständiges ESP32-Gateway zwischen FRITZ!Box-Smart-Home-Geräten und MQTT. Die FRITZ!Box übernimmt DECT; der ESP32 liest ihre lokale AHA-Schnittstelle und übersetzt MQTT-Stellaufträge. Weder Home Assistant noch ein bestimmtes übergeordnetes Automationssystem sind zum Betrieb erforderlich.

**Stabile Firmware für ESP32 mit 4 MB Flash:** [Download 0.1.16](https://github.com/m-kaufmann/fritz2mqtt/releases/tag/v0.1.16) mit Factory- und OTA-Datei, Quellen, Lizenzen und Prüfsummen. Die Neuerungen stehen im [Änderungsprotokoll](CHANGELOG.md).

## Funktionen

- Bis zu acht Thermostate und Schaltsteckdosen insgesamt, eindeutige Zuordnung über AIN.
- IST- und gemeldete SOLL-Temperatur sowie Batterie in Prozent.
- Neue Sollwerte über frei konfigurierbare MQTT-Topics, 8–28 °C in 0,5-Grad-Schritten.
- Der reservierte MQTT-Wert `9.9` bedeutet „kein Stellauftrag“ und wird nicht weitergereicht.
- Lokale Webseite mit Bedienung, Einrichtung und Diagnose; keine Cloud-Abhängigkeit im laufenden Betrieb.
- Zwei dauerhaft gespeicherte WLAN-Profile und offener Einrichtungs-AP nach 60 Sekunden ohne verbundenes WLAN. WLAN-Netze lassen sich im AP und im laufenden Betrieb suchen.
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

Die Erstinstallation erfolgt über USB. Spätere ESPHome-Uploads können OTA erfolgen. Die generische Firmware enthält keine Standort-Zugangsdaten und keinen gemeinsamen API-Schlüssel. Im WLAN `FRITZ2MQTT-<Gerätekennung>` die Einrichtung unter `http://192.168.4.1/` öffnen. Webseite und AP sind in diesem Profil ohne eigene Anmeldung eingerichtet. FRITZ!Box-Adresse und Benutzer mit Smart-Home-Rechten, WLAN und MQTT werden dort eingetragen. Danach Geräte zuordnen und deren Topics festlegen.

Der Topic-Präfix ist unter „Geräte zuordnen“ einstellbar. Standard ist `fritz2mqtt`. Die Vorschläge lassen sich je Topic ändern:

```text
fritz2mqtt/site/1/thermostat_t_ist       # Temperatur-Rückmeldung
fritz2mqtt/site/1/thermostat_t_soll      # Sollwert-Rückmeldung
fritz2mqtt/site/1/thermostat_battery     # Batterie, %
fritz2mqtt/site/1/thermostat/set         # Neuer Sollwert
```

### FRITZ!Smart Energy 200

Die Steckdose erscheint nach dem Koppeln mit der FRITZ!Box unter „Einrichtung“ → „Geräte zuordnen“. Dort lassen sich die Topics pro Steckdose vorbelegen und einzeln bearbeiten. In „Bedienung“ erscheinen der gemeldete Zustand, der Ein-/Ausschalter und die aktuellen Messwerte.

| Topic-Endung (Vorschlag) | Richtung | Nutzlast |
| --- | --- | --- |
| `/set` | Eingang | `0` für Aus, `1` für Ein |
| `_state` | Ausgang | Gemeldeter Schaltzustand als `0` oder `1` |
| `_power` | Ausgang | Momentanleistung in W |
| `_energy` | Ausgang | Energiezähler in kWh |
| `_voltage` | Ausgang | Netzspannung in V |
| `_temperature` | Ausgang | Temperatur am Messort in °C |

Messwert-Topics können einzeln leer bleiben. Änderungen über die Taste oder FRITZ!Box werden beim nächsten erfolgreichen Abruf als Zustand gemeldet.

Der MQTT-Host ist ein Hostname ohne `http://`. Rückmeldungen: QoS 1, retained. Befehle: QoS-0-Abonnement, kein Ausführen retained/duplizierter/ungültiger Nachrichten. Ein angenommener Auftrag wird einmalig geschrieben und zurückgelesen. Die Zeitschaltung der FRITZ!Box bleibt wirksam. Das Gateway bestätigt den von der FRITZ!Box zurückgelesenen Sollwert.

## Entwicklung und Updates

[Entwicklungs-/Releaseplan](docs/releases.md), [Lizenzhinweise](THIRD_PARTY_NOTICES.md), [Änderungsprotokoll](CHANGELOG.md).

Vor Veröffentlichung fertiger Firmware: vollständiger Build, Tests auf Hardware, Lizenzinventar der tatsächlich eingebundenen Abhängigkeiten, Wiederherstellung und Erhalt bestehender Konfiguration prüfen. Der Tab „System / Firmware“ implementiert die HTTPS-Updateprüfung und eine ausdrücklich bestätigte Installation; siehe [Updatebeschreibung](docs/updates.md). Bestehende Geräte im Stable-Kanal finden neue Versionen unter **System / Firmware → Nach Updates suchen**.

Hosttests auf macOS: `bash tools/test.sh` (Python 3, clang++, Expat und Node.js). Linux-Firmwarebuilds verwenden die tatsächlichen ESP-IDF-Bibliotheken; die CommonCrypto-Adapter unter `tests/mbedtls/` gehören ausschließlich zu den macOS-Hosttests.

## Lizenz

Projektlizenz: [GPLv3](LICENSE). Nutzung, Änderung und Weitergabe sind unter deren Bedingungen möglich, auch kommerziell. Fremde Bestandteile behalten ihre eigenen Lizenzbedingungen. Dieses Repository enthält keine persönlichen Gerätekonfigurationen oder Zugangsdaten.
