# Arayüz için sahte cihaz: /api/* uç noktalarını taklit eder. ham voltaj /sim?v=2.1 ile değiştirilir.
import json, random, sys, urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
HTML = sys.argv[1]
st = {"v": 2.10, "derleme": "mock-1", "diag": {"kacirilan":2,"toplam_hata":5,"kopma":1,"son_hata":"MQTT","onceki_uyanik":812,"wifi_ardisik":0,"hizli_deneme":1}}
A = {"ssid":"EvAgi_IoT","sifre_var":True,"ozel_wifi":False,"dhcp":False,"ip":"192.168.1.50","gw":"192.168.1.1","mask":"255.255.255.0","dns":"192.168.1.1","mqtt_host":"","mqtt_port":1883,"mqtt_user":"","mqtt_sifre_var":False,"ozel_mqtt":False,"uyku_dk":30,"pil_kesim":3.4,"kuru_v":2.478,"islak_v":1.14,"kal_min_fark":0.2,"bitki_ad":"","bitki_tur":""}
class H(BaseHTTPRequestHandler):
    def log_message(self,*a): pass
    def js(self,o,c=200):
        b=json.dumps(o).encode(); self.send_response(c); self.send_header("Content-Type","application/json"); self.end_headers(); self.wfile.write(b)
    def ham(self): return round(st["v"]+random.uniform(-0.004,0.004),3)
    def do_GET(self):
        u=urllib.parse.urlparse(self.path)
        if u.path=="/sim": st["v"]=float(urllib.parse.parse_qs(u.query)["v"][0]); return self.js({"ok":True})
        if u.path=="/api/durum":
            h=self.ham(); nem=max(0,min(100,round((A["kuru_v"]-h)/(A["kuru_v"]-A["islak_v"])*100)))
            return self.js({"ad":"toprak-sensoru","derleme":st["derleme"],"mac":"AA:BB:CC:00:11:22","wifi_bagli":True,"ip":"192.168.1.50","ssid":"EvAgi_IoT","rssi":-60,"kanal":2,"ap":False,"mqtt_bagli":True,"ham_v":h,"nem":nem,"pil_v":3.95,"pil_yuzde":73,"calisma_sn":100,"kalan_sn":600,"dongu":3,"bssid":st.get("bssid","02:11:22:33:44:55"),"deco_sabit":"bssid" in st,"wifi_geri_alindi":st.get("geri",False),"wifi_deneme":False,"wifi_varsayilan":False,**st["diag"]})
        if u.path=="/api/ayarlar": return self.js(A)
        if u.path=="/api/agler": return self.js([{"ssid":"EvAgi_IoT","rssi":-80,"kilit":True,"kanal":2,"bssid":"02:11:22:33:44:55"},{"ssid":"EvAgi_IoT","rssi":-48,"kilit":True,"kanal":2,"bssid":"02:11:22:33:44:66"},{"ssid":"baska","rssi":-70,"kilit":True,"kanal":6,"bssid":"11:22:33:44:55:66"}])
        if u.path.startswith("/api/"): return self.js({"ok":True})
        b=open(HTML,"rb").read(); self.send_response(200); self.send_header("Content-Type","text/html; charset=utf-8"); self.end_headers(); self.wfile.write(b)
    def do_POST(self):
        n=int(self.headers.get("Content-Length",0)); raw=self.rfile.read(n)
        q={} if self.path=="/update" else urllib.parse.parse_qs(raw.decode(),keep_blank_values=True)
        if self.path=="/update":
            st["derleme"]="mock-2"
            b=b"Update Successful!"; self.send_response(200); self.end_headers(); self.wfile.write(b); return
        if self.path=="/api/teshis_sifirla":
            st["diag"].update(kacirilan=0,toplam_hata=0,kopma=0,son_hata="Yok",wifi_ardisik=0,hizli_deneme=0)
        if self.path=="/api/deco_sec":
            if st.get("deco_basarisiz"): st["geri"]=True
            else: st["bssid"]=q["bssid"][0]
        if self.path=="/api/ayarlar":
            if "kuru_v" in q:
                k,i=float(q["kuru_v"][0]),float(q["islak_v"][0])
                if not k>i+0.2: return self.js({"ok":False,"hata":"Kalibrasyon: kuru değeri ıslaktan en az 0,2 V büyük olmalı"},400)
                A["kuru_v"],A["islak_v"]=k,i
            if "bitki_ad" in q: A["bitki_ad"]=q["bitki_ad"][0]; A["bitki_tur"]=q["bitki_tur"][0]
        return self.js({"ok":True})
ThreadingHTTPServer(("127.0.0.1",int(sys.argv[2]) if len(sys.argv)>2 else 8765),H).serve_forever()
