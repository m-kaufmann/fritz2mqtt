# Fremdkomponenten und Lizenzstand

Diese erste Quellfassung enthält eigene Gateway-Komponenten und Testadapter. Der Build lädt zusätzliche Fremdkomponenten; deren vollständiges Inventar und Lizenztexte müssen vor dem ersten Firmware-Release aus dem tatsächlich aufgelösten Build erzeugt und geprüft werden. Die folgenden Hinweise sind noch keine vollständige Binärdistributions-Lizenzsammlung.

| Bestandteil | Lizenzgrundlage / Rolle |
|---|---|
| ESPHome 2026.10.1 | Runtime/C++ GPLv3; Python und übrige Teile MIT. https://github.com/esphome/esphome/blob/2026.10.1/LICENSE |
| ESP-IDF | Überwiegend Apache-2.0 mit eigenen Drittkomponenten. Tatsächlich durch ESPHome aufgelöste Version im Release festhalten. https://github.com/espressif/esp-idf/blob/master/docs/en/COPYRIGHT.rst |
| espressif/expat 2.9.0 | Expat/MIT; XML-Parser. https://github.com/libexpat/libexpat/blob/R_2_9_0/expat/COPYING |
| Mbed TLS, MQTT, FreeRTOS und Web-/JSON-Abhängigkeiten | Bestandteil bzw. Abhängigkeiten des aufgelösten Builds; Versions- und Einzel-Lizenztexte noch zu erfassen. |

Kein Hersteller-Firmwarecode der FRITZ!Box wird mitgeliefert. Das Gateway nutzt deren AHA-HTTP-Schnittstelle. Herstellernamen bezeichnen kompatible Geräte.

Jeder künftige Firmwaredownload benötigt den entsprechenden vollständigen Quellstand, erforderliche Buildskripte und Konfiguration sowie die Lizenz-/Urheberhinweise der enthaltenen Bestandteile. Das Root-LICENSE überschreibt keine Fremdlizenzen.
