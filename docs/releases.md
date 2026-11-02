# Entwicklungs- und Releaseweg

1. **Dieses Änderungspaket:** eigenständiger Kern, neutrale Erstinstallation, Tests und CI-Build. Vorhandene Systeme bleiben auf ihrer bisherigen Firmware.
2. **Nächstes Paket:** HTTP(S)-Updateprüfung, explizite Installationsfreigabe und Fortschritt in der Gateway-Webseite. ESPHome `http_request`/`update` nutzen; Manifest über GitHub Pages, keine beliebig eingebbare Firmware-URL.
3. **Erste Vorabversion:** passender Quellstand, vollständige Lizenztexte der eingebundenen Bibliotheken, Erstinstallations- und OTA-Datei, Manifest, Prüfsummen und Versionshinweise. Die CI prüft derzeit ausschließlich den Build und veröffentlicht keine Binärdateien.
4. **Stabile Version:** erst nach vollständigem Hardwaretest einschließlich Konfigurationserhalt, unterbrochenem Download, Wiederherstellung, erneuter FRITZ!Box-/MQTT-Verbindung und verworfenem Alt-/Doppelbefehl nach Neustart.

Versionsnummer, Hardwareprofil, Flash-/Partitionslayout und Konfigurationsschema müssen getrennt geführt werden. Das Schema `config_v1` bleibt im ersten Schritt unverändert. Für vorhandene Installationen kann `storage_namespace` gezielt auf den bisherigen Namensraum gesetzt werden; die generische Firmware liest ausschließlich ihren konfigurierten Namensraum. Neue Standardwerte überschreiben keine erfolgreich gelesene Konfiguration.

Zwei spätere Kanäle: stabil und Vorabversion. Regelmäßiges Prüfen darf keine automatische Installation auslösen. HTTPS mit Zertifikatsprüfung ist erforderlich. Prüfsummen ersetzen keine Herausgebersignatur. Safe Mode ist eine Wiederherstellungshilfe; Rückfall auf alte Firmware und deren Konfigurationskompatibilität sind separat zu prüfen.
