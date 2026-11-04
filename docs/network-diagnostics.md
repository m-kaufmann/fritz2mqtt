# Netzwerkdiagnose bei zeitweiser Nichterreichbarkeit

Eine nicht erreichbare Webseite allein belegt keinen Neustart. Die Status-API ergänzt daher die HTTP-Transaktionen um native WLAN-/IP-Ereignisse und den Resetgrund. Alle Angaben gelten für den aktuellen Start; sie werden nicht regelmäßig in den Flash geschrieben.

## Zusätzliche Statuswerte

`GET /gateway/api/status` enthält unter `network`:

- `reset_reason`: unveränderter ESP-IDF-Resetgrund; beispielsweise 3 nach einem beabsichtigten Software-/OTA-Neustart.
- `event_listeners_ready`: beide nativen Listener erfolgreich registriert. Erst dann sind Ereigniszähler aussagekräftig.
- `wifi_connects`, `wifi_disconnects`, `last_disconnect_ms`, `last_disconnect_reason`: native Stationsereignisse. Eine vom eigenen Startablauf ausgelöste Abmeldung ist kein Beleg für einen späteren Netzausfall.
- `ip_acquired`, `ip_lost`: IP-Ereignisse der Station.
- `ap_starts`, `ap_stops`: Ereignisse des Einrichtungs-AP.
- `main_loop_age_ms`, `max_main_loop_gap_ms`: Alter des letzten Hauptloop-Durchlaufs und größte gemessene Pause nach dem ersten Durchlauf. Ein großer Wert kann unterschiedliche blockierende Vorgänge anzeigen; er benennt allein keine Ursache.

Die Listener registrieren sich nach der Komponenteninitialisierung. Ihre Callbacks ändern ausschließlich atomare Zahlenwerte, nehmen keine Web-/Konfigurationssperre und protokollieren keine Ereignispayloads. Damit enthält die Diagnose weder Passwörter noch Session-IDs oder Antwortinhalte.

`http.last` und `http.last_failure` ergänzen die bestehende HTTP-Diagnose um `task_errno`, `transport_error`, `transport_detail`, `channel` und `ap_bssid`. Fehlernummern werden unmittelbar an den HTTP-Phasen erfasst. Die native Transportbibliothek verwendet auch für unverschlüsseltes HTTP einen ESP-TLS-Unterbau; diese Feldnamen beweisen keine verschlüsselte Verbindung. WLAN-Signal, Kanal und BSSID stammen vom Abschluss der jeweiligen HTTP-Transaktion und sind keine kontinuierliche Funkmessung. Netzwerkkennungen vor einer öffentlichen Bugmeldung entfernen.

## Vorgehen

Zuerst ohne Neustart Firmware, Laufzeit, Konfigurationsfingerabdruck und Zähler erfassen. Erreichbarkeit des ESP per TCP/HTTP und ICMP mit einer unabhängigen Gegenstelle vergleichen. Bei mehreren lokalen Netzwerkschnittstellen die Quellwege ausdrücklich trennen. Einen eigenen Test-Upload als beabsichtigten Neustart markieren; der darf nicht als spontaner Absturz gezählt werden.

WLAN-Roaming und Modem-Stromsparmodus sind mögliche Vergleichsfaktoren, keine aus einem Timeout abgeleiteten Ursachen. `post_connect_roaming: false` und `power_save_mode: none` können einzeln verglichen werden. Die bisherigen Vergleiche zeigten schnellere typische Antworten ohne Stromsparmodus, aber weiterhin Timeouts. Daraus folgt keine allgemein bestätigte Fehlerbehebung. Quellen: [ESPHome-WLAN-Dokumentation](https://esphome.io/components/wifi/) und die tatsächlich verwendeten ESP-IDF-Header.

Prüfstand 04.11.2026: Hosttests mit ASan/UBSan, Paketgrenzen und JavaScript erfolgreich; vollständiger ESP32-Build mit ESPHome 2026.10.1 erfolgreich. Am separaten Testgerät Listenerbereitschaft, erwarteter Software-Resetgrund und fortlaufende Ereignis-/Hauptloopwerte nach Wartungs-OTA bestätigt. Keine Änderung des gespeicherten Konfigurationsschemas oder der einmaligen Thermostat-Schreiblogik. Kein neues öffentliches Firmwarepaket freigegeben.
