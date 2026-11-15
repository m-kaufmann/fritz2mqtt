#pragma once
namespace esphome::fritzbox_aha {
static const char GATEWAY_PAGE[] = R"F2M(
<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><link rel="icon" type="image/vnd.microsoft.icon" sizes="16x16 32x32" href="/favicon.ico"><link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png"><title>FRITZ2MQTT Gateway</title>
<style>
:root{font:15px/1.45 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;color:#244A59;background:#F4F8F8;color-scheme:light}*{box-sizing:border-box}body{margin:0}main{max-width:1200px;margin:auto;padding:20px}h1{font-size:23px;margin:0}h2{font-size:18px;margin:8px 0 16px}h3{font-size:16px;margin:0 0 12px}header{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap}small,.hint{color:#607A84}.tabs{display:flex;gap:4px;border-bottom:1px solid #B9D1D5;margin:18px 0 12px;overflow-x:auto}.tabs button{border:0;border-radius:0;background:transparent;border-bottom:3px solid transparent}.tabs button:not([aria-selected=true]){font-weight:400}.tabs button[aria-selected=true]{font-weight:700;color:#A3372B;background:#FCEDE8;border-color:#C94B3C}button,input,select{font:inherit;color:inherit}button{padding:7px 14px;min-height:38px;border:1px solid #607A84;background:#F4F8F8;border-radius:5px;font-weight:650;cursor:pointer}button:hover:not(:disabled){background:#EAF1F3}button:disabled{opacity:.45;cursor:default}button:focus-visible,input:focus-visible{outline:3px solid #C94B3C;outline-offset:2px}input,select{padding:7px 9px;min-height:38px;border:1px solid #B9D1D5;border-radius:4px;background:white;max-width:100%;width:100%}input[type=checkbox]{width:20px;min-height:20px}label{display:flex;flex-direction:column;gap:4px}.check{flex-direction:row;align-items:center;gap:8px}.status{font-size:15px;line-height:1.45;padding:10px 14px;margin:0 0 12px;border-left:4px solid #C94B3C;background:#FCEDE8}.status.ok{border-color:#17613B;background:#DDF2E5}.status.error{border-color:#B64040;background:#FCEBEC}.panel{background:white;padding:18px;margin:12px 0;border-top:3px solid #C94B3C;border-radius:5px}.devices{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,450px),1fr));gap:16px}.device{background:white;padding:18px;border-top:3px solid #C94B3C;border-radius:5px}.values{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin:14px 0}.value{color:#17613B;font-weight:650;font-size:23px}.unknown{color:#607A84}.input-row{display:flex;gap:6px;align-items:center}.input-row input{text-align:center;max-width:100px}.input-row button{min-width:38px}.apply{margin:12px 0;width:100%}.device-status{font-size:13px;min-height:20px;color:#607A84}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,240px),1fr));gap:12px}.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:16px}.scroll{overflow-x:auto;-webkit-overflow-scrolling:touch}table{border-collapse:collapse;width:100%;font-size:13px;text-align:left}th,td{padding:9px;border-bottom:1px solid #DCE7E9;white-space:nowrap}th{font-weight:650}.rowvalue{color:#17613B;font-weight:650}.mapping{border-top:1px solid #DCE7E9;margin:18px 0;padding-top:16px}.mapping .topics{margin-top:12px}details{margin:12px 0}summary{cursor:pointer;padding:8px 0}a{color:#A3372B}[hidden]{display:none!important}@media(pointer:coarse){button,input{min-height:44px}.input-row button{min-width:44px}.tabs button{min-height:44px}}@media(max-width:540px){main{padding:12px}.panel,.device{padding:14px}h1{font-size:20px}}
.grid{grid-template-columns:repeat(2,minmax(0,1fr));align-items:start}.grid>label{min-width:0}.field-label{display:block;min-height:22px;overflow-wrap:anywhere}.grid input,.grid select{height:38px}.grid .check{min-height:38px}.grid .check input{height:20px;flex:0 0 20px}.network{margin:12px 0 20px}.network h3{margin-bottom:8px}.discover-row{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:8px;align-items:center}.discover-row select{min-width:0;height:38px}.mapping>.check{margin-top:16px}.network .actions+label{margin-top:8px}.network select{min-height:38px}@media(pointer:coarse){.network select{min-height:44px}}.check input{order:-1}.check .field-label{min-height:0}.mapping .actions{margin-top:12px}button:focus-visible,select:focus-visible{outline:3px solid #C94B3C;outline-offset:2px}@media(pointer:coarse){.grid input,.grid select,.discover-row select{height:44px}.grid .check input{height:20px}}@media(max-width:640px){.grid{grid-template-columns:1fr}.discover-row{grid-template-columns:1fr}.discover-row button{justify-self:start}}
@media(pointer:coarse){.check,.grid .check{min-height:44px}}
.firmware-status{padding:10px 14px;border-left:4px solid #607A84;background:#F4F8F8;margin:14px 0;min-height:42px}.firmware-status.error{border-color:#B64040;background:#FBEDED}.firmware-status.warning{border-color:#B7791F;background:#FFF2D8}.firmware-status.ok{border-color:#17613B;background:#DDF2E5}progress{width:100%;height:24px}.firmware-version{font-weight:650;color:#17613B}.firmware-confirm{margin-top:18px}
#settings input[type=password]::placeholder{color:#607A84;opacity:1;letter-spacing:.12em}
#settings .check{min-height:38px;gap:10px;cursor:pointer}
#settings .check input{appearance:none;position:relative;width:44px;height:26px;min-height:26px;flex:0 0 44px;margin:0;padding:0;border:2px solid #607A84;border-radius:20px;background:#F4F8F8;cursor:pointer}
#settings .check input::before{content:"";position:absolute;inset:3px auto auto 3px;width:16px;height:16px;border-radius:50%;background:#607A84}
#settings .check input:checked{border-color:#17613B;background:#17613B}
#settings .check input:checked::before{left:auto;right:3px;background:white}
.action-group{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;width:max-content;max-width:100%}
.action-group button,.mapping-actions button{text-align:left}
.mapping-actions{display:grid;inline-size:min(100%, 13rem);margin-top:12px}
button.primary{background:#A3372B;border-color:#A3372B;color:white}
button.primary:hover:not(:disabled){background:#872D23}
button.primary:disabled{background:#FCEDE8;border-color:#B9D1D5;color:#607A84;opacity:1}
button.apply.waiting-for-other:disabled{background:#A3372B;border-color:#A3372B;color:white}
#settings>.actions{align-items:center;padding-inline:18px}
@media(max-width:540px){#settings>.actions{padding-inline:14px}.action-group{grid-template-columns:minmax(0,1fr)}}
.input-row button{inline-size:38px;padding-inline:0}
@media(pointer:coarse){.input-row button{inline-size:44px}}
@media(pointer:coarse){#settings .check{min-height:44px}}
@media(forced-colors:active){#settings .check input{appearance:auto;width:26px;flex-basis:26px}#settings .check input::before{display:none}}
#diagnostics dl,#firmware dl{margin:0}
.diagnosis-row{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1.7fr);gap:16px;padding:9px 0;border-bottom:1px solid #DCE7E9}
.diagnosis-row:last-child{border-bottom:0}
.diagnosis-row dt{font-weight:600}
.diagnosis-row dd{margin:0;overflow-wrap:anywhere}
.diagnosis-state{display:grid;grid-template-columns:20px minmax(0,1fr);align-items:start;gap:8px}
.diagnosis-status-text{font-weight:650;overflow-wrap:break-word}
.diagnosis-icon{display:inline-grid;place-items:center;width:20px;height:20px;border-radius:50%;background:#F4F8F8;color:#607A84}
.diagnosis-state.ok{color:#17613B}.diagnosis-state.ok .diagnosis-icon{color:inherit;background:#DDF2E5}
.diagnosis-state.warning{color:#8B5B0D}.diagnosis-state.warning .diagnosis-icon{color:inherit;background:#FFF2D8}
.diagnosis-state.error{color:#A3372B}.diagnosis-state.error .diagnosis-icon{color:inherit;background:#FCEBEC}
@media(max-width:540px){.diagnosis-row{grid-template-columns:minmax(0,1fr) minmax(0,1.3fr);gap:10px}}
</style>
<main><header><div><h1>FRITZ2MQTT Gateway</h1><small id="location">Eine FRITZ!Box · mehrere Thermostate</small></div><button id="poll">Werte aktualisieren</button></header>
<nav class="tabs" role="tablist" aria-label="Gateway-Bereiche"><button role="tab" id="tab-control" aria-controls="control" aria-selected="true">Bedienung</button><button role="tab" id="tab-settings" aria-controls="settings" aria-selected="false">Einrichtung</button><button role="tab" id="tab-diagnostics" aria-controls="diagnostics" aria-selected="false">Diagnose</button><button role="tab" id="tab-firmware" aria-controls="firmware" aria-selected="false">System / Firmware</button></nav>
<div id="message" class="status" role="status" aria-live="polite">Verbindung wird aufgebaut …</div>
<section id="control" role="tabpanel" aria-labelledby="tab-control"><div class="devices" id="devices"></div><p id="empty" class="hint" hidden>Noch kein Thermostat gefunden. FRITZ!Box-Zugang unter Einrichtung prüfen.</p><p class="hint">„Neues SOLL“ ist eine lokale Auswahl. Erst „Übernehmen“ sendet einen einmaligen Stellauftrag. Die Zeitschaltung der FRITZ!Box bleibt wirksam.</p></section>
<section id="settings" role="tabpanel" aria-labelledby="tab-settings" hidden><div class="panel"><h2>WLAN und Einrichtungs-AP</h2><p class="hint">Zwei WLAN-Kennungen sind speicherbar. Ist keines erreichbar, öffnet nach 60 Sekunden ein offener Einrichtungs-AP. Zugang über 192.168.4.1.</p><div id="wifi"></div><p class="hint">Vier Sternchen zeigen ein gespeichertes Passwort. Bei gleicher SSID bleibt es erhalten, bis du ein neues eingibst. Für ein WLAN ohne Passwort „Offenes WLAN“ wählen. Bei geänderten WLAN-Daten startet das Gateway nach dem Speichern neu.</p></div><div class="panel"><h2>FRITZ!Box und Standort</h2><div class="grid" id="connection"></div><p class="hint">Vier Sternchen zeigen ein gespeichertes Passwort. Gib nur dann ein Passwort ein, wenn du es ändern willst.</p><details><summary>MQTT-Verbindung</summary><p class="hint">Der Gateway veröffentlicht IST, gemeldetes SOLL und Batterie auf den eingetragenen Topics. Neue Sollwerte empfängt er ausschließlich über das jeweilige Befehls-Topic. Änderungen werden nach dem Speichern angewendet.</p><div class="grid" id="mqtt"></div></details></div><div class="panel"><h2>Thermostate zuordnen</h2><div id="topic-settings" class="grid"></div><p class="hint">Der Präfix ist der erste Teil der Topic-Vorschläge. Mit „Topics vorbelegen“ erzeugst du die vier Topics für das jeweilige Thermostat.</p><p class="hint">Bis zu acht Geräte lassen sich über ihre AIN zuordnen.</p><div class="discover-row"><select id="discovered" aria-label="Gefundenes Thermostat auswählen"></select><button id="add">Gerät hinzufügen</button></div><div id="mappings"></div></div><div class="actions"><div class="action-group"><button id="save" class="primary" disabled>Speichern und anwenden</button><button id="reload">Gespeicherten Stand laden</button></div><span id="dirty" class="hint"></span></div></section>
<section id="diagnostics" role="tabpanel" aria-labelledby="tab-diagnostics" hidden><div class="panel"><h2>Verbindungen</h2><dl id="connections"></dl></div><div class="panel"><h2>Diagnosedaten</h2><dl id="diagnosis-data"></dl><p><a href="/?esphome=1" target="_blank" rel="noopener">ESPHome-Protokoll öffnen</a></p></div><div class="panel"><h2>Gerätedetails</h2><div class="scroll"><table><thead><tr><th>Thermostat</th><th>AIN</th><th>Modell</th><th>Batterie (%)</th><th>Status</th></tr></thead><tbody id="details"></tbody></table></div></div></section>
<section id="firmware" role="tabpanel" aria-labelledby="tab-firmware" hidden><div class="panel"><h2>Firmware aktualisieren</h2><dl>
<div class="diagnosis-row"><dt>Installierte Version</dt><dd id="fw-current" class="firmware-version">—</dd></div>
<div class="diagnosis-row"><dt id="fw-offer-label">Verfügbare stabile Version</dt><dd id="fw-latest">—</dd></div>
<div class="diagnosis-row"><dt>Updatekanal</dt><dd id="fw-channel">—</dd></div>
<div class="diagnosis-row"><dt>Hardwareprofil</dt><dd id="fw-profile">esp32-4mb-v1</dd></div>
</dl><div id="fw-message" class="firmware-status" role="status" aria-live="polite">Updatefunktion wird geprüft …</div><progress id="fw-progress" max="100" value="0" aria-label="Firmwareinstallation" hidden></progress><p class="hint">Neue Versionen werden regelmäßig gesucht. Eine Installation erfolgt ausschließlich nach deiner Bestätigung und startet das Gateway neu. Die gespeicherte Einrichtung wird nicht zurückgesetzt.</p><p><a href="https://github.com/m-kaufmann/fritz2mqtt/releases" target="_blank" rel="noopener noreferrer">Versionshinweise auf GitHub</a></p><label class="check firmware-confirm"><input type="checkbox" id="fw-confirm" disabled><span id="fw-confirm-label">Angezeigte Version installieren und Gateway neu starten</span></label><div class="actions action-group"><button id="fw-check" disabled>Nach Updates suchen</button><button id="fw-install" disabled>Update installieren</button></div><p class="hint">Angeboten werden nur Versionen des fest eingestellten Kanals für dieses Hardwareprofil. Eine freie Firmware-URL und eine automatische Installation sind nicht vorgesehen.</p></div></section>
</main>
<script>
'use strict';
const $=id=>document.getElementById(id), cards=new Map(); let data=null,config=null,token='',dirty=false,noticeUntil=0,loading=false,saving=false,configBaseline='',pendingTarget=null;
function node(tag,text,cls){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(cls)n.className=cls;return n;}
function message(text,error=false,seconds=8){$('message').textContent=text;$('message').className='status '+(error?'error':'ok');noticeUntil=Date.now()+seconds*1000;}
function configSnapshot(){
  return JSON.stringify(config,(key,value)=>key==='password'&&value===''?undefined:value);
}
function mark(){
  dirty=configSnapshot()!==configBaseline;
  $('save').disabled=saving||!dirty;
  $('dirty').textContent=dirty?'Ungespeicherte Änderungen':'Gespeicherter Stand geladen';
}
function configSaved(){
  configBaseline=configSnapshot();
  mark();
}
async function api(path,body){const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),8000);try{const options=body===undefined?{cache:'no-store'}:{method:'POST',headers:{'Content-Type':'application/json','X-Gateway-Token':token},body:JSON.stringify(body)};options.signal=controller.signal;const res=await fetch('/gateway/api/'+path,options);let result;try{result=await res.json();}catch{throw Error('Ungültige Antwort vom Gateway');}if(!res.ok){const error=Error(result.error||'Anfrage fehlgeschlagen (HTTP '+res.status+')');error.status=res.status;throw error;}return result;}catch(e){if(e.name==='AbortError')throw Error('Zeitüberschreitung; Verbindung wird erneut geprüft');throw e;}finally{clearTimeout(timeout);}}
function value(v,unit=' °C'){return typeof v==='number'?v.toFixed(unit===' %'?0:1)+unit:'—';}
for(const tab of document.querySelectorAll('[role=tab]')){tab.onclick=()=>{for(const b of document.querySelectorAll('[role=tab]')){const active=b===tab;b.setAttribute('aria-selected',active);$(b.getAttribute('aria-controls')).hidden=!active;}if(tab.id==='tab-settings'&&!config)loadConfig();};tab.onkeydown=e=>{if(['ArrowLeft','ArrowRight'].includes(e.key)){const tabs=[...document.querySelectorAll('[role=tab]')],n=(tabs.indexOf(tab)+(e.key==='ArrowRight'?1:tabs.length-1))%tabs.length;tabs[n].focus();tabs[n].click();}};}
function createCard(d){const root=node('article',undefined,'device'),title=node('h2'),status=node('small');root.append(title,status);const values=node('div',undefined,'values'),actual=node('div'),target=node('div');const a=node('div',undefined,'value'),t=node('div',undefined,'value');actual.append(node('div','IST'),a);target.append(node('div','Derzeitiges SOLL'),t);values.append(actual,target);root.append(values);const label=node('label','Neues SOLL (°C)'),row=node('div',undefined,'input-row'),minus=node('button','−'),input=node('input'),plus=node('button','+');input.type='number';input.min=8;input.max=28;input.step=1;input.setAttribute('aria-label','Neues SOLL für '+d.name);input.value=typeof d.target==='number'?d.target:'';minus.setAttribute('aria-label','Ein Grad weniger');plus.setAttribute('aria-label','Ein Grad mehr');input.oninput=()=>{input.dataset.edited='1';};minus.onclick=()=>{input.dataset.edited='1';input.value=Math.max(8,Math.ceil(Number(input.value)||20)-1);};plus.onclick=()=>{input.dataset.edited='1';input.value=Math.min(28,Math.floor(Number(input.value)||20)+1);};row.append(minus,input,plus);label.append(row);root.append(label);const apply=node('button','Übernehmen','apply primary'),result=node('div','', 'device-status');result.setAttribute('role','status');result.setAttribute('aria-live','polite');root.append(apply,result);apply.onclick=async()=>{if(pendingTarget!==null||data?.busy)return;const target=Number(input.value);if(input.value===''||!input.checkValidity()||!Number.isFinite(target)){message('Bitte eine Temperatur von 8 bis 28 °C in ganzen Grad wählen.',true);return;}pendingTarget=d.ain;renderTargetButtons();try{const r=await api('target',{ain:d.ain,target});result.textContent=r.ignored?r.reason:'Auftrag angenommen; Rückmeldung wird geprüft';message(r.ignored?r.reason:'Auftrag angenommen; Rückmeldung wird geprüft.');}catch(e){message(e.message+' Der Auftrag wird nicht automatisch wiederholt; bitte die Rückmeldung abwarten.',true);}finally{pendingTarget=null;await refresh();}};$('devices').append(root);return{root,title,status,a,t,input,apply,result,minus,plus};}
function render(){if(!data)return;$('location').textContent=data.location+' · eine FRITZ!Box · '+data.devices.length+' Thermostat(e)';if(Date.now()>noticeUntil){$('message').textContent=data.fresh&&data.authenticated?'FRITZ!Box verbunden · Messwerte aktuell':data.diagnostic||'Warte auf FRITZ!Box';$('message').className='status '+(data.fresh&&data.authenticated?'ok':'');}const live=new Set();for(const d of data.devices){live.add(d.ain);let c=cards.get(d.ain);if(!c){c=createCard(d);cards.set(d.ain,c);}if(c.input.value===''&&!c.input.dataset.edited&&typeof d.target==='number')c.input.value=d.target;c.title.textContent=d.name;c.status.textContent=!d.configured?'Gefunden · bitte unter Einrichtung zuordnen':!d.enabled?'Bedienung deaktiviert':!d.available?'Keine aktuellen Gerätedaten':d.error&&d.error!=='0'?'HKR-Fehler '+d.error:'Erreichbar';c.a.textContent=value(d.actual);c.a.className=typeof d.actual==='number'?'value':'value unknown';c.t.className=typeof d.target==='number'?'value':'value unknown';c.t.textContent=typeof d.target==='number'?value(d.target):d.mode==='Aus'||d.mode==='Ein'?d.mode:'—';if(data.command_ain===d.ain)c.result.textContent=data.command_message;}for(const [ain,c]of cards)if(!live.has(ain)){c.root.remove();cards.delete(ain);}$('empty').hidden=!!data.devices.length;renderTargetButtons();renderDiagnostics(data);$('details').replaceChildren();for(const d of data.devices){const tr=node('tr');for(const v of[d.name,d.ain,d.model,value(d.battery,' %'),d.available?(d.battery_low==='1'?'Batterie wechseln':'Erreichbar'):'Keine aktuellen Daten'])tr.append(node('td',v));$('details').append(tr);}renderDiscovery();}
function renderTargetButtons(){
  const busy=pendingTarget!==null||!!data.busy;
  const selected=pendingTarget??data.command_ain;
  for(const d of data.devices){
    const button=cards.get(d.ain).apply;
    const ready=d.enabled&&d.ready;
    button.disabled=busy||!ready;
    button.classList.toggle('waiting-for-other',ready&&busy&&d.ain!==selected);
  }
}
function statusBadge(text,state,detail){
  const status=node('span',undefined,'diagnosis-state '+state);
  const symbols={ok:'✓',idle:'○',pending:'◷',warning:'!',error:'✕'};
  const icon=node('span',symbols[state],'diagnosis-icon');
  icon.setAttribute('aria-hidden','true');
  const body=node('span');
  body.append(node('span',text,'diagnosis-status-text'));
  if(detail)body.append(node('span',' · '+detail,'hint'));
  status.append(icon,body);
  return status;
}
function diagnosisRow(list,label,text,state,detail){
  const row=node('div',undefined,'diagnosis-row'), value=node('dd');
  if(state)value.append(statusBadge(text,state,detail));
  else value.textContent=text;
  row.append(node('dt',label),value);list.append(row);
}
function renderDiagnostics(status){
  const connections=$('connections'), metrics=$('diagnosis-data');
  connections.replaceChildren();metrics.replaceChildren();
  if(!status){
    diagnosisRow(connections,'Gateway','Nicht erreichbar','error');
    diagnosisRow(metrics,'Diagnose','Keine aktuellen Daten');
    return;
  }
  const diagnostic=status.diagnostic||'Warte auf erste Abfrage';
  const mqtt=status.mqtt==='Verbunden'&&!status.mqtt_connected?'Nicht verbunden':status.mqtt||'Nicht verbunden';
  const wifiState=status.wifi_connected?'ok':status.network?.wifi_disconnects?'warning':status.wifi_configured?'pending':'idle';
  diagnosisRow(connections,'WLAN',
    {ok:'Verbunden',warning:'Verbindung unterbrochen',pending:'Verbindung wird aufgebaut',idle:'Nicht verbunden'}[wifiState],
    wifiState,status.wifi_connected?status.wifi_ssid:'');
  const fritzState=status.wifi_connected&&status.connected?'ok':
    status.wifi_connected&&/fehlgeschlagen|ungueltig/i.test(diagnostic)?'warning':
    status.wifi_connected&&/laeuft|Warte/.test(diagnostic)?'pending':'idle';
  diagnosisRow(connections,'FRITZ!Box',
    {ok:'Erreichbar',warning:'Abfrage fehlgeschlagen',pending:'Verbindung wird geprüft',idle:'Nicht erreichbar'}[fritzState],fritzState);
  const authState=fritzState==='ok'&&status.authenticated?'ok':
    /Anmeldung abgelehnt|Ungueltige Login/i.test(diagnostic)?'error':
    /Sperrzeit|Session ungueltig/i.test(diagnostic)?'warning':
    /Anmeldung laeuft/i.test(diagnostic)?'pending':'idle';
  diagnosisRow(connections,'Anmeldung',
    {ok:'Erfolgreich',error:'Fehler',warning:'Erneute Anmeldung folgt',pending:'Anmeldung läuft',idle:'Nicht angemeldet'}[authState],
    authState,authState==='ok'?status.method:'');
  const mqttState=/Fehler|lehnt Anmeldung|fehlgeschlagen|konnte nicht/i.test(mqtt)?'error':
    status.wifi_connected&&status.mqtt_connected?'ok':
    /unterbrochen/i.test(mqtt)?'warning':/aufgebaut|Warte/.test(mqtt)?'pending':'idle';
  diagnosisRow(connections,'MQTT',mqtt,mqttState);
  diagnosisRow(connections,'Einrichtungs-AP',status.ap_active?'Aktiv':'Bereit bei WLAN-Ausfall',status.ap_active?'ok':'idle',
    [status.ap_ssid,status.ap_address,'ohne Passwort'].filter(Boolean).join(' · '));
  diagnosisRow(metrics,'FRITZ!Box-Diagnose',diagnostic);
  diagnosisRow(metrics,'Gefundene Geräte',status.device_count>=0?String(status.device_count):'—');
  diagnosisRow(metrics,'Broker-Bestätigungen',[status.mqtt_subacks??0,(status.mqtt_subacks===1?'Befehls-Topic':'Befehls-Topics'),'·',
    status.mqtt_pubacks??0,(status.mqtt_pubacks===1?'Messwert':'Messwerte')].join(' '));
  diagnosisRow(metrics,'MQTT-Empfang',[
    (status.mqtt_rx??0)+' empfangen',(status.mqtt_accepted??0)+' angenommen',
    (status.mqtt_ignored??0)+' ignoriert',(status.mqtt_rejected??0)+' abgelehnt'
  ].join(' · '));
  diagnosisRow(metrics,'Letzter MQTT-Eingang',status.mqtt_last_input||'—');
  diagnosisRow(metrics,'Betriebszeit',Math.floor(status.uptime/60)+' min');
  diagnosisRow(metrics,'Einrichtungsstand','Version '+status.revision);
}
function field(parent,labelText,obj,key,type='text',max=160){const label=node('label'),input=node('input');label.append(node('span',labelText,'field-label'));input.type=type;input.dataset.key=key;input.maxLength=max;if(type==='checkbox'){label.className='check';input.checked=!!obj[key];}else input.value=obj[key]??'';input.oninput=()=>{obj[key]=type==='checkbox'?input.checked:type==='number'?Number(input.value):input.value;mark();};label.append(input);parent.append(label);return input;}
function passwordField(parent,label,obj,saved,max=256){
  const input=field(parent,label,obj,'password','password',max);
  input.autocomplete='new-password';
  input.placeholder=saved?'****':'';
  return input;
}
async function findWifiNetworks(){
  if(location.hostname==='192.168.4.1'&&!data?.wifi_connected){
    const response=await fetch('/config.json',{cache:'no-store',signal:AbortSignal.timeout(5000)});
    if(!response.ok)throw Error('WLAN-Liste konnte nicht geladen werden.');
    return (await response.json()).aps;
  }
  await api('wifi/scan',{});
  const deadline=Date.now()+15000;
  do{
    await new Promise(resolve=>setTimeout(resolve,500));
    const result=await api('wifi/scan');
    if(result.error)throw Error(result.error);
    if(!result.busy)return result.aps;
  }while(Date.now()<deadline);
  throw Error('WLAN-Suche dauert zu lange. Bitte erneut versuchen.');
}
function wifiForm(){
  $('wifi').replaceChildren();
  config.wifi.forEach((w,i)=>{
    const section=node('div',undefined,'network'), grid=node('div',undefined,'grid');
    section.append(node('h3','WLAN '+(i+1)),grid);
    const originalSsid=w.ssid, originalOpen=w.open;
    const ssid=field(grid,'WLAN-Name (SSID)',w,'ssid','text',32);
    ssid.autocapitalize='none';ssid.spellcheck=false;
    const password=passwordField(grid,'WLAN-Passwort'+(w.password_set?' (gespeichert)':''),w,w.password_set,63);
    const open=field(section,'Offenes WLAN (ohne Passwort)',w,'open','checkbox');
    password.disabled=!!w.open;
    const clearPassword=()=>{password.value='';w.password='';};
    const updatePasswordHint=()=>{
      const saved=w.password_set&&w.ssid===originalSsid&&!open.checked;
      password.placeholder=saved?'****':'';
      password.previousElementSibling.textContent='WLAN-Passwort'+(saved?' (gespeichert)':'');
    };
    open.addEventListener('input',()=>{
      password.disabled=open.checked;
      if(open.checked)clearPassword();
      updatePasswordHint();
      mark();
    });
    ssid.addEventListener('input',()=>{
      clearPassword();
      open.checked=w.ssid===originalSsid&&originalOpen;w.open=open.checked;password.disabled=open.checked;
      updatePasswordHint();
      mark();
    });
    const actions=node('div',undefined,'actions'), scan=node('button','WLANs suchen');
    const label=node('label','Netzwerk für WLAN '+(i+1)+' auswählen'), select=node('select');
    label.hidden=true;label.append(select);
    scan.onclick=async()=>{
      scan.disabled=true;scan.textContent='WLAN-Suche läuft …';label.hidden=true;
      try{
        const aps=await findWifiNetworks();
        select.replaceChildren(node('option','WLAN auswählen'));
        select.options[0].value='';
        for(const ap of aps.filter(ap=>ap.ssid)){
          const option=node('option',ap.ssid+' · '+ap.rssi+' dBm'+(ap.lock?' · geschützt':' · offen'));
          option.value=ap.ssid;option.dataset.open=ap.lock?'0':'1';select.append(option);
        }
        label.hidden=select.options.length===1;
        if(label.hidden)message('Keine WLANs gefunden. Erneut suchen oder SSID eingeben.');
        else select.focus();
      }catch(e){
        message(e.message+' Die SSID kann direkt eingegeben werden.',true);
      }finally{
        scan.disabled=false;scan.textContent='WLANs suchen';
      }
    };
    select.onchange=()=>{
      if(!select.value)return;
      if(ssid.value!==select.value){ssid.value=select.value;ssid.dispatchEvent(new Event('input'));}
      open.checked=select.selectedOptions[0].dataset.open==='1';open.dispatchEvent(new Event('input'));
      if(!open.checked)password.focus();
    };
    actions.append(scan);section.append(actions,label);
    $('wifi').append(section);
  });
}
function connectionForm(){if(!config)return;wifiForm();$('topic-settings').replaceChildren();
  const prefix=field($('topic-settings'),'MQTT-Topic-Präfix',config.mqtt,'topic_prefix','text',64);
  prefix.autocapitalize='none';prefix.spellcheck=false;
  $('connection').replaceChildren();field($('connection'),'Location',config,'location','text',64);field($('connection'),'FRITZ!Box-IP',config,'host','text',64);field($('connection'),'FRITZ!Box-Benutzer',config,'username','text',128);passwordField($('connection'),'FRITZ!Box-Passwort'+(config.fritz_password_set?' (vorhanden)':''),config,config.fritz_password_set);$('mqtt').replaceChildren();field($('mqtt'),'Broker-Adresse',config.mqtt,'host','text',128);field($('mqtt'),'Port',config.mqtt,'port','number');field($('mqtt'),'MQTT-Benutzer',config.mqtt,'username','text',128);passwordField($('mqtt'),'MQTT-Passwort'+(config.mqtt.password_set?' (vorhanden)':''),config.mqtt,config.mqtt.password_set);field($('mqtt'),'TLS verwenden',config.mqtt,'tls','checkbox');renderMappings();}
function renderMappings(){
const mappings=$('mappings');
const expanded=new Set([...mappings.querySelectorAll('details[open]')].map(details=>details.dataset.ain));
mappings.replaceChildren();config.devices.forEach((d,i)=>{const wrap=node('div',undefined,'mapping');wrap.append(node('h3',d.name+' · AIN '+d.ain));const grid=node('div',undefined,'grid');field(grid,'Anzeigename',d,'name','text',96);field(grid,'Unit',d,'unit','text',5);field(grid,'Datenpunktbasis',d,'base','text',64).placeholder='z. B. thermostat_living';wrap.append(grid);field(wrap,'Bedienung freigeben',d,'enabled','checkbox');const det=node('details');det.dataset.ain=d.ain;det.open=expanded.has(d.ain);det.append(node('summary','Datenpunkte und Topics'));det.append(node('p','Neuer Sollwert: _t_neu (actor). Rückmeldungen: _t_ist, _t_soll und _battery (sensor, %). 9,9 bedeutet „kein Stellauftrag“ und wird nicht weitergereicht.','hint'));const topics=node('div',undefined,'grid topics');field(topics,'IST-Topic',d,'actual_topic');field(topics,'SOLL-Rückmeldungs-Topic',d,'target_topic');field(topics,'Neuer-SOLL-Topic',d,'command_topic');field(topics,'Batterie-Topic (%)',d,'battery_topic');det.append(topics);const suggest=node('button','Topics vorbelegen');suggest.onclick=()=>{if(!config.location||!d.unit||!d.base){message('Zuerst Location, Unit und Datenpunktbasis ausfüllen.',true);return;}const namespace=config.mqtt.topic_prefix;if(!/^[A-Za-z0-9_-]{1,64}$/.test(namespace)){message('MQTT-Topic-Präfix: bitte nur Buchstaben, Ziffern, _ und - verwenden.',true);return;}const prefix=namespace+'/'+config.location+'/'+d.unit+'/';if((prefix+d.base+'_battery').length>160){message('Topic-Vorschlag zu lang. Bitte Präfix, Location oder Datenpunktbasis kürzen.',true);return;}d.actual_topic=prefix+d.base+'_t_ist';d.target_topic=prefix+d.base+'_t_soll';d.command_topic=prefix+d.base+'/set';d.battery_topic=prefix+d.base+'_battery';for(const input of topics.querySelectorAll('input'))input.value=d[input.dataset.key];mark();};const remove=node('button','Zuordnung entfernen');remove.onclick=()=>{config.devices.splice(i,1);mark();renderMappings();renderDiscovery();};const topicActions=node('div',undefined,'mapping-actions');topicActions.append(suggest);det.append(topicActions);const mappingActions=node('div',undefined,'mapping-actions');mappingActions.append(remove);wrap.append(det,mappingActions);mappings.append(wrap);});}
function renderDiscovery(){const select=$('discovered'),selected=select.value;select.replaceChildren();for(const d of data?.devices||[])if(!config?.devices.some(c=>c.ain===d.ain)){const option=node('option',d.name+' · '+d.ain);option.value=d.ain;select.append(option);}if([...select.options].some(o=>o.value===selected))select.value=selected;const empty=!select.options.length;if(empty){select.append(node('option','Keine weiteren Thermostate gefunden'));}select.disabled=empty;$('add').disabled=!config||empty||config.devices.length>=8;}
$('add').onclick=()=>{const d=data.devices.find(d=>d.ain===$('discovered').value);if(!d)return;config.devices.push({ain:d.ain,name:d.name,unit:'',base:'',enabled:false,actual_topic:'',target_topic:'',command_topic:'',battery_topic:''});mark();renderMappings();renderDiscovery();};
async function loadConfig(){try{config=await api('config');configSaved();connectionForm();renderDiscovery();}catch(e){message(e.message,true);}}
$('reload').onclick=()=>{if(dirty&&!confirm('Ungespeicherte Änderungen verwerfen?'))return;loadConfig();};
$('save').onclick=async()=>{
  if(!config||!dirty||saving)return;
  saving=true;$('save').disabled=true;$('settings').inert=true;
  try{
    const r=await api('config',{...config,wifi_configured:!!config.wifi?.some(w=>w.ssid)});
    configSaved();
    if(r.wifi_restarting){
      $('dirty').textContent='Gespeichert';
      message('Einrichtung gespeichert. Gateway startet neu. Danach mit dem neuen WLAN oder dem Einrichtungs-AP verbinden.',false,25);
      setTimeout(()=>{if(!dirty)loadConfig();},10000);
    }else{
      await loadConfig();
      message('Einrichtung dauerhaft gespeichert; wird angewendet.');
    }
    await refresh();
  }catch(e){
    message(e.message,true);
  }finally{
    saving=false;$('settings').inert=false;$('save').disabled=!dirty;
  }
};
$('poll').onclick=async()=>{try{await api('poll',{});message('Neue Geräteabfrage angefordert.');}catch(e){message(e.message,true);}};
async function refresh(){if(loading)return;loading=true;try{data=await api('status');token=data.csrf;render();}catch(e){renderDiagnostics(null);$('details').replaceChildren();for(const c of cards.values()){c.apply.disabled=true;c.apply.classList.remove('waiting-for-other');c.a.textContent='—';c.t.textContent='—';c.status.textContent='Verbindung unterbrochen · wird erneut versucht';}message('Gateway nicht erreichbar: '+e.message,true,3);}finally{loading=false;}}

let fwData=null,fwLoading=false,fwBusy=false,fwOffer='',fwOutcome='',fwOutcomeKind='warning';
let fwExpected=null;
try{fwExpected=JSON.parse(sessionStorage.getItem('fritz2mqtt-update')||'null');}catch{}
function fwMessage(text,kind='idle'){
 const message=$('fw-message');
 if(message.dataset.kind===kind&&message.dataset.text===text)return;
 message.dataset.kind=kind;message.dataset.text=text;
 message.className='firmware-status '+kind;
 message.replaceChildren(statusBadge(text,kind));
}
function clearExpected(){fwExpected=null;try{sessionStorage.removeItem('fritz2mqtt-update');}catch{}}
function renderFirmware(){
 if(!fwData)return;
 const f=fwData,active=!!f.checking||!!f.installing||fwBusy;
 $('fw-channel').textContent=f.channel==='testing'?'Testkanal · keine allgemeine Freigabe':'Stabil';$('fw-offer-label').textContent=f.channel==='testing'?'Verfügbare Testversion':'Verfügbare stabile Version';
 $('fw-current').textContent=f.current||'—';$('fw-latest').textContent=f.latest||'Noch kein freigegebenes Angebot';$('fw-profile').textContent=f.profile||'—';
 if(fwOffer!==f.latest){fwOffer=f.latest;$('fw-confirm').checked=false;}
 if(fwExpected&&fwExpected.device===f.device&&fwExpected.version===f.current){fwOutcome='Version '+f.current+' läuft nach dem Neustart.';fwOutcomeKind='ok';clearExpected();}
 if(fwExpected&&Date.now()-fwExpected.started>120000){fwOutcome='Update-Ergebnis noch offen. Installierte Version prüfen; kein automatischer Wiederholversuch.';fwOutcomeKind='warning';clearExpected();}
 const awaiting=!!fwExpected;
 $('fw-check').disabled=!f.enabled||active||awaiting;
 $('fw-confirm').disabled=!f.can_install||active||awaiting;
 $('fw-confirm-label').textContent=(f.latest?'Version '+f.latest:'Angezeigte Version')+' installieren und Gateway neu starten';
 $('fw-install').disabled=!f.can_install||!$('fw-confirm').checked||active||awaiting||dirty;
 $('fw-progress').hidden=!f.installing;
 $('fw-progress').value=Number.isFinite(f.progress)?Math.max(0,Math.min(100,f.progress)):0;
 if(fwOutcome)fwMessage(fwOutcome,fwOutcomeKind);
 else if(!f.enabled)fwMessage('Updatefunktion in diesem Build nicht eingerichtet.');
 else if(f.installing)fwMessage('Firmware wird installiert · '+Math.round($('fw-progress').value)+' % · Neustart folgt','pending');
 else if(awaiting)fwMessage('Installation angefordert; Fortschritt und Neustart werden geprüft.','pending');
 else if(f.checking||fwBusy)fwMessage('Updateprüfung läuft …','pending');
 else if(f.failed)fwMessage(f.message||'Updateprüfung fehlgeschlagen.','error');
 else if(dirty)fwMessage('Vor der Installation die ungespeicherten Einstellungen speichern oder neu laden.','warning');
 else if(f.can_install)fwMessage('Neue Version verfügbar. Zum Installieren unten bestätigen.','warning');
 else if(f.latest&&f.latest===f.current)fwMessage('Firmware ist aktuell.','ok');
 else fwMessage(f.latest&&f.latest!==f.current?f.reason:f.message||'Noch keine erfolgreiche Updateprüfung.');
}
async function refreshFirmware(){if(fwLoading)return;fwLoading=true;try{fwData=await api('firmware');renderFirmware();}catch(e){$('fw-check').disabled=true;$('fw-install').disabled=true;$('fw-confirm').disabled=true;if(fwExpected&&Date.now()-fwExpected.started>120000){clearExpected();fwOutcome='Gateway nach dem Update noch nicht erreichbar. Ergebnis offen; installierte Version später prüfen.';fwOutcomeKind='warning';}fwMessage(fwOutcome||(fwExpected?'Gateway wird neu verbunden. Das Update wird nicht erneut gesendet.':'Firmwarestatus nicht erreichbar: '+e.message),fwOutcome?fwOutcomeKind:fwExpected?'pending':'error');}finally{fwLoading=false;}}
$('fw-confirm').onchange=renderFirmware;
$('fw-check').onclick=async()=>{if(fwBusy)return;fwBusy=true;fwOutcome='';renderFirmware();try{await api('firmware/check',{});}catch(e){fwOutcome=e.message;fwOutcomeKind='error';}finally{fwBusy=false;await refreshFirmware();}};
$('fw-install').onclick=async()=>{
 if(fwBusy||fwExpected||!fwData?.can_install||!$('fw-confirm').checked||dirty)return;
 fwBusy=true;fwOutcome='';fwExpected={version:fwData.latest,device:fwData.device,started:Date.now()};
 try{sessionStorage.setItem('fritz2mqtt-update',JSON.stringify(fwExpected));}catch{}
 renderFirmware();
 try{await api('firmware/install',{version:fwExpected.version,confirm:true});}
 catch(e){if(e.status)clearExpected();fwOutcome=e.message+' Die Installation wird nicht automatisch wiederholt.';fwOutcomeKind=e.status?'error':'warning';}
 finally{fwBusy=false;$('fw-confirm').checked=false;await refreshFirmware();}
};
refreshFirmware();setInterval(refreshFirmware,3000);

refresh().then(()=>{if(data&&(!data.wifi_connected||location.hostname==='192.168.4.1'))$('tab-settings').click();});setInterval(refresh,3000);window.addEventListener('beforeunload',e=>{if(dirty){e.preventDefault();e.returnValue='';}});
</script></html>

)F2M";
}
