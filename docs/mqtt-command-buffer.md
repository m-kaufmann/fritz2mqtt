# MQTT Sollwertgruppen ab 0.1.18 dev

Entwicklungsstand vom 20.11.2026 auf Basis der Quellen von Stable 0.1.17
(Commit `22acb9c424989350232248f3fc1d725208eb623d`).
Die lokale Korrektur betrifft den Empfang und die serielle Verarbeitung von
Stellaufträgen durch denselben HTTP-Worker.

## Ursache

Die globale Sperre `command_busy_` verhinderte die Annahme weiterer Aufträge.
Gleichzeitig entnahm die MQTT-Schleife diese Nachrichten bereits aus dem
Empfangspuffer. Bei einer Gruppe konnten deshalb Aufträge für alle anderen
Thermostate als abgelehnt verloren gehen. Ein binäres Aufwecksignal allein
gewährleistete außerdem keine Abarbeitung mehrerer wartender Aufträge.

## Annahme und Reihenfolge

Der Empfangspuffer fasst 32 MQTT-Nachrichten. Dahinter liegen maximal acht
wartende Stellaufträge, entsprechend der unterstützten Gerätezahl. Ein
laufender Auftrag gehört separat dem HTTP-Worker. Die Sperre zeigt an, dass
Aufträge laufen oder warten. Sie verhindert Firmwareinstallationen und
Konfigurationsänderungen während der Abarbeitung.

Pro Gerät wartet höchstens ein Sollwert. Ein neu eintreffender gültiger
Sollwert ersetzt den bisherigen wartenden Sollwert und behält dessen
FIFO-Platz. Hat der Worker einen älteren Auftrag desselben Geräts bereits
entnommen, prüft er vor dem HTTP-Schreibversuch auf einen inzwischen
eingegangenen neueren Sollwert. Der ältere Auftrag wird dann übersprungen.
Ein bereits begonnener Schreibversuch wird abgeschlossen, bevor ein neuerer
Auftrag desselben Geräts ausgeführt werden kann.

Der Neutralwert `9.9` erhöht den Ignoriert-Zähler. Er erzeugt keinen
Stellauftrag und entfernt oder ersetzt keinen wartenden Sollwert.
Retained-, DUP-, ungültige und veraltete Empfangsnachrichten bleiben gesperrt.

## Worker und Altersgrenze

Der Worker arbeitet die Aufträge einzeln ab und weckt sich bei weiterem
Arbeitsvorrat selbst. Zusätzlich prüft er bei wartenden Aufträgen spätestens
nach einer Sekunde erneut. Dadurch gehen zusammengefallene binäre
Aufwecksignale nicht zulasten der Warteschlange. Der Empfang einer
Abschlussmeldung in der Hauptschleife hebt die Sperre für andere noch
laufende oder wartende Aufträge nicht auf.

Ein angenommener Auftrag gilt höchstens 180 Sekunden ab seinem ursprünglichen
MQTT-Empfang. Die Grenze gilt auch bei WLAN-Ausfall oder FRITZ!Box-Wartezeiten.
Ein neuer gültiger Sollwert hat einen neuen Empfangszeitpunkt. Die bereits
bestehende Zehn-Sekunden-Grenze im rohen Empfangspuffer bleibt bestehen.

Vor dem Schreiben prüft der Worker Alter, Gerätefreigabe, Typ, AIN,
Konfigurationsrevision, Wertebereich, Session und aktuelle Geräteantwort.
Verbindungs-, Session- und Abfrageprobleme dürfen vor dem ersten
Schreibversuch innerhalb der Altersgrenze erneut geprüft werden.
Die vorhandenen HTTP-Wartezeiten gelten dabei weiter. Ein zurückgestellter
Auftrag kommt ans Ende der Warteschlange, sofern bereits kein neuerer
Sollwert dieses Geräts wartet.

Nach Beginn eines HTTP-Schreibversuchs erfolgt kein automatischer zweiter
Schreibversuch für denselben Auftrag. Das gilt auch bei unklarer
Sendeantwort oder gescheitertem Zurücklesen. Ein älterer Auftrag darf einen
inzwischen angenommenen neueren wartenden Auftrag nicht wieder einsetzen.
Die Warteschlange ist flüchtig und wird nach einem Neustart nicht wiederhergestellt.

## Diagnose

`GET /gateway/api/status` ergänzt die bestehenden Daten um
`command_pending` und `command_max_age_ms`. Unter
`devices[].command` stehen pro Gerät

- `pending` und `running` für den aktuellen Arbeitsvorrat,
- `state`, `message`, `id` und `at_ms` für das letzte Ereignis,
- `accepted`, `replaced`, `ignored`, `rejected`, `overflow`,
  `expired`, `deferred` und `superseded` als Zähler,
- `attempted` für HTTP-Schreibversuche und `finished` für beendete
  Worker-Aufträge,
- `last_attempted_target` für den zuletzt tatsächlich versuchten Sollwert.

Ein abgelaufener oder ersetzter Auftrag kann ohne Schreibversuch enden.
Unbekannte oder unlesbare Topics können keinem Gerät zugeordnet werden und
zählen ausschließlich im globalen MQTT-Ablehnungszähler. Die Diagnose behält
höchstens 16 Gerätekennungen im RAM. Nach einem Neustart beginnen die Zähler
neu.

## Lokale Verifikation

`bash tools/test.sh` führt die bisherigen Hosttests und zwei neue Prüfungen
mit AddressSanitizer und UndefinedBehaviorSanitizer aus.
`tests/command_pipeline_test.py` kompiliert die tatsächlichen Funktionen für
MQTT-Empfang, Annahme, Worker-Ablauf, Schreiben und Ergebnisverarbeitung aus
den aktuellen Quelldateien gegen simulierte Plattform- und HTTP-Funktionen.

| Fall | Geprüftes Ergebnis |
| --- | --- |
| Original-Annahmefunktion von 0.1.17 | Drei Nachrichten ergeben einen angenommenen und zwei abgelehnte Aufträge |
| Drei und acht unterschiedliche Geräte | Alle gültigen Aufträge werden seriell geschrieben |
| Zwölf Gruppen mit wechselndem Erstgerät | Jedes Gerät erhält zwölf Schreibversuche |
| Weitere Acht-Geräte-Gruppen während der Abarbeitung | Alle Geräte erreichen den jeweils letzten Sollwert |
| Mehrere wartende Sollwerte desselben Geräts | Der zuletzt empfangene gültige Wert gewinnt bei gleichem FIFO-Platz |
| Neuer Sollwert während der Vorprüfung | Der alte noch unversuchte Auftrag wird übersprungen |
| Neuer Sollwert während des Schreibversuchs | Der neuere Auftrag folgt anschließend |
| Neutralwerte und ungültige Nachrichten | Ein gültiger wartender Sollwert bleibt erhalten |
| Langsame HTTP-Antworten | Acht Aufträge werden innerhalb des Zeitbudgets abgearbeitet |
| Vorübergehende Abfrage- und Session-Probleme | Erneute Vorprüfung, anschließend vollständige Abarbeitung |
| Unklare Schreibantwort und fehlendes Zurücklesen | Genau ein Schreibversuch je Auftrag |
| Dauerhafter WLAN- oder Session-Ausfall | Ablauf aller wartenden Aufträge ohne spätere Schreibversuche |
| Ablauf während der Vorprüfung | Kein HTTP-Schreibversuch |
| Konfigurationswechsel vor Entnahme oder während der Vorprüfung | Keine Ausführung mit alter Revision |
| Gerätefehler, fehlende Erreichbarkeit | Ablehnung pro Gerät, weitere Geräte werden abgearbeitet |
| Empfangspufferüberlauf und fragmentierte Nachricht | Gerätebezogene Ablehnungs- und Überlaufdiagnose |
| Uptime-Überlauf | Altersprüfung funktioniert über den 32-Bit-Zählerumlauf hinweg |
| Steckdose und Thermostat gemeinsam | Typgerechte Verarbeitung und Neutralwertschutz |
| Alte Abschlussmeldung während eines neuen Auftrags | Sperre für den aktiven Auftrag bleibt gesetzt |

Die Reproduktion der ursprünglichen Annahmefunktion erfolgt lokal mit

```sh
python3 tests/command_pipeline_test.py --baseline 22acb9c
```

Der ESP32-Build verwendet ESPHome 2026.10.1 und `firmware/esp32.yaml`
mit Projektversion `0.1.18-dev`. Die lokale Vorbereitung umfasst
Factory- und OTA-Datei, Quellen-Patch, Buildinformationen und Prüfsummen.
Hardwareabnahme und Veröffentlichung sind nachgelagerte Schritte.
