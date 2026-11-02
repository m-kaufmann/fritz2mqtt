"""Local browser-only update simulation. Never connects to a device or broker."""
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import json

ROOT=Path(__file__).resolve().parents[1]
STATE={'current':'0.1.0-dev','latest':'0.1.0','checking':0,'installing':0,'posts':0}
class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args): pass
    def send(self,body,code=200,kind='application/json'):
        self.send_response(code);self.send_header('Content-Type',kind);self.end_headers()
        self.wfile.write((json.dumps(body) if kind=='application/json' else body).encode())
    def do_GET(self):
        if self.path=='/':
            text=(ROOT/'components/fritzbox_aha/gateway.html').read_text()
            text=text.replace('<main>','<main><p style="background:#FFF2D8;padding:10px">Lokale Simulation · keine Geräte- oder MQTT-Verbindung</p>')
            self.send(text,kind='text/html; charset=utf-8');return
        if self.path=='/gateway/api/status':
            self.send({'csrf':'preview-only','location':'Demo','devices':[],'wifi_connected':True,'fresh':False,'diagnostic':'Vorschau ohne FRITZ!Box'});return
        if self.path=='/gateway/api/firmware':
            checking=STATE['checking']>0;installing=STATE['installing']>0
            progress=0
            if checking:STATE['checking']-=1
            if installing:
                progress=(4-STATE['installing'])*25;STATE['installing']-=1
                if STATE['installing']==0:STATE['current']=STATE['latest']
            self.send({'enabled':True,'current':STATE['current'],'latest':STATE['latest'],'device':'DEMO-ESP','profile':'esp32-4mb-v1','checking':checking,'installing':installing,'can_install':STATE['current']!=STATE['latest'] and not checking and not installing,'progress':progress,'message':'Vorschau bereit','reason':'','posts':STATE['posts']});return
        self.send({'error':'Nur Firmware-Vorschau'},404)
    def do_POST(self):
        try:body=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))))
        except Exception:self.send({'error':'JSON erwartet'},400);return
        if self.headers.get('X-Gateway-Token')!='preview-only':self.send({'error':'Token'},403);return
        if self.path=='/gateway/api/firmware/check' and body=={}:STATE['checking']=1;self.send({'accepted':True},202);return
        if self.path=='/gateway/api/firmware/install' and body=={'confirm':True,'version':STATE['latest']} and STATE['current']!=STATE['latest']:
            STATE['posts']+=1;STATE['installing']=3;self.send({'accepted':True},202);return
        self.send({'error':'Keine Gerätebedienung in dieser Simulation'},400)
if __name__=='__main__':
    print('Simulation only: http://127.0.0.1:8927/',flush=True)
    HTTPServer(('127.0.0.1',8927),Handler).serve_forever()
