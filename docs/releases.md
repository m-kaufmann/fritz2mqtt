# Entwicklungs- und Releaseweg

1. **Dieses Änderungspaket:** eigenständiger Kern, neutrale Erstinstallation, Tests und CI-Build. Vorhandene Systeme bleiben auf ihrer bisherigen Firmware.
2. **Implementiert, noch in Abnahme:** HTTPS-Updateprüfung, explizite Installationsfreigabe, Fortschritt und Neustartkontrolle in der Gateway-Webseite über ESPHome `http_request`/`update`. Festes Manifest über GitHub Pages; Hosting im getrennten Testkanal eingerichtet; Hardwareabnahme siehe Updatebeschreibung.
3. **Erste Vorabversion:** passender Quellstand, vollständige Lizenztexte der eingebundenen Bibliotheken, Erstinstallations- und OTA-Datei, Manifest, Prüfsummen und Versionshinweise. Die CI prüft derzeit ausschließlich den Build und veröffentlicht keine Binärdateien.
4. **Stabile Version:** erst nach vollständigem Hardwaretest einschließlich Konfigurationserhalt, unterbrochenem Download, Wiederherstellung, erneuter FRITZ!Box-/MQTT-Verbindung und verworfenem Alt-/Doppelbefehl nach Neustart.

Versionsnummer, Hardwareprofil, Flash-/Partitionslayout und Konfigurationsschema müssen getrennt geführt werden. Das Schema `config_v1` bleibt im ersten Schritt unverändert. Für vorhandene Installationen kann `storage_namespace` gezielt auf den bisherigen Namensraum gesetzt werden; die generische Firmware liest ausschließlich ihren konfigurierten Namensraum. Neue Standardwerte überschreiben keine erfolgreich gelesene Konfiguration.

Zwei getrennte Kanäle: stabil und Testkanal (`testing`). Regelmäßiges Prüfen darf keine automatische Installation auslösen. HTTPS mit Zertifikatsprüfung ist erforderlich. Prüfsummen ersetzen keine Herausgebersignatur. Safe Mode ist eine Wiederherstellungshilfe; Rückfall auf alte Firmware und deren Konfigurationskompatibilität sind separat zu prüfen.

## Stable 0.1.18

Die Veröffentlichung vom 20.11.2026 wurde vom Betreiber auf Basis der lokalen
Simulationen und des erfolgreichen ESP32-Builds freigegeben. Die
Hardwareabnahme mit realen Befehlsgruppen steht noch aus.
Das Release enthält OTA- und Factory-Firmware, die zugeordneten Quellen,
Lizenztexte, Buildinformationen und Prüfsummen.
