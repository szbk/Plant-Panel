"""Plant Panel: MQTT'den sensör verisini alır, SQLite'a yazar, web sayfası sunar.
Her sensör MQTT konusunun ilk parçasıyla ayrılır: <sensör>/sensor/<değer>/state
"""
import os
import re
import statistics
import threading
import time

import paho.mqtt.client as mqtt
from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import FileResponse, Response

from . import veri

MQTT_HOST = os.environ.get("MQTT_HOST", "host.docker.internal")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
MQTT_USER = os.environ.get("MQTT_USER", "")
MQTT_PASS = os.environ.get("MQTT_PASS", "")
FOTO_DIR = os.path.join(veri.VERI_DIR, "foto")
FOTO_MAX = 12 * 1024 * 1024
STATIK = os.path.join(os.path.dirname(__file__), "static")
ID_KALIBI = re.compile(r"^[A-Za-z0-9_-]{1,64}$")

app = FastAPI(title="Bitki paneli", docs_url=None, redoc_url=None)
toplayici = veri.Toplayici()
durum = {"mqtt": False, "istemci": None}
# Ayar modu anahtarı: <sensör>/ayar_modu konusunda retained ON. Sensör uyanınca kaydı kendisi siler.
ayar_bekliyor = {}    # sensor -> True: ON yazıldı, sensör henüz uyanmadı
ayar_acildi = {}      # sensor -> sensörün kaydı sildiği an (ayar moduna girdi)
ayar_iptal = set()    # kaydı panel sildi: "sensör açtı" sayılmasın
AYAR_MODU_SN = 600
# Sensör IP adresi henüz gelmediyse kullanılacak adresler: CIHAZ_ADRESLERI="toprak-sensoru=192.168.1.50,..."
CIHAZ_ADRESI = dict(p.split("=", 1) for p in os.environ.get("CIHAZ_ADRESLERI", "").split(",") if "=" in p)
# Cihaz bilgilerini (IP, ağ, MAC...) gönderme aralığı, saat. Sensör <id>/bilgi_araligi (retained) okur.
BILGI_SAAT_MIN, BILGI_SAAT_MAX, BILGI_SAAT_VARSAYILAN = 4, 720, 24


def ayar_mesaji(sensor, deger):
    if deger == "ON":
        ayar_bekliyor[sensor] = True
        ayar_acildi.pop(sensor, None)
    elif not deger:
        if ayar_bekliyor.pop(sensor, False) and sensor not in ayar_iptal:
            ayar_acildi[sensor] = time.time()
        ayar_iptal.discard(sensor)


def mesaj_geldi(_istemci, _kullanici, m):
    parcalar = m.topic.split("/")
    if len(parcalar) == 2 and parcalar[1] == "ayar_modu" and ID_KALIBI.match(parcalar[0]):
        ayar_mesaji(parcalar[0], m.payload.decode("utf-8", "replace"))
        return
    if len(parcalar) == 2 and parcalar[1] == "bilgi_araligi" and ID_KALIBI.match(parcalar[0]):
        db = veri.baglan()
        veri.ayar_yaz(db, f"bilgi_araligi:{parcalar[0]}", m.payload.decode("utf-8", "replace").strip())
        db.close()
        return
    if len(parcalar) != 4 or parcalar[1] != "sensor" or parcalar[3] != "state" or not ID_KALIBI.match(parcalar[0]):
        return
    sensor, ad = parcalar[0], parcalar[2]
    deger = m.payload.decode("utf-8", "replace")
    if ad in veri.AYAR_KONULARI:
        if deger:
            db = veri.baglan()
            veri.ayar_yaz(db, f"{veri.AYAR_KONULARI[ad]}:{sensor}", deger)
            db.close()
        return
    if m.retain:   # bağlanınca gelen eski değer; yeni ölçüm değil
        return
    toplayici.ekle(sensor, time.time(), ad, deger)


def baglandi(istemci, _k, _b, kod, _o=None):
    durum["mqtt"] = not kod.is_failure
    if durum["mqtt"]:
        ayar_bekliyor.clear()   # kayıtlı ON'lar abonelikle yeniden gelir
        istemci.subscribe([("+/sensor/+/state", 0), ("+/ayar_modu", 1), ("+/bilgi_araligi", 1)])
    print(f"MQTT {MQTT_HOST}:{MQTT_PORT}: {kod}", flush=True)


def koptu(_i, _k, _b, kod, _o=None):
    durum["mqtt"] = False
    print(f"MQTT koptu: {kod}", flush=True)


def yazici():
    db = veri.baglan()
    while True:
        time.sleep(1)
        for sensor in toplayici.hazir_olanlar(time.time()):
            toplayici.bosalt(db, sensor)


@app.on_event("startup")
def basla():
    veri.baglan().close()
    os.makedirs(FOTO_DIR, exist_ok=True)
    i = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="plant-panel")
    if MQTT_USER:
        i.username_pw_set(MQTT_USER, MQTT_PASS)
    i.on_connect, i.on_disconnect, i.on_message = baglandi, koptu, mesaj_geldi
    i.reconnect_delay_set(2, 30)
    durum["istemci"] = i
    i.connect_async(MQTT_HOST, MQTT_PORT, keepalive=30)
    i.loop_start()
    threading.Thread(target=yazici, daemon=True).start()


def foto_yolu(sensor):
    return os.path.join(FOTO_DIR, sensor + ".foto")


def kimlik(sensor):
    if not ID_KALIBI.match(sensor):
        raise HTTPException(404, "Sensör yok")
    return sensor


def ayar_durumu(sensor, ip=None):
    acildi = ayar_acildi.get(sensor)
    if acildi and time.time() - acildi > AYAR_MODU_SN:
        ayar_acildi.pop(sensor, None)
        acildi = None
    return {
        "bekliyor": bool(ayar_bekliyor.get(sensor)),
        "acildi_sn": int(time.time() - acildi) if acildi else None,
        "adres": ip or CIHAZ_ADRESI.get(sensor),
    }


def bilgi_araligi(db, sensor):
    v = veri.ayar_oku(db, f"bilgi_araligi:{sensor}", "")
    secili = int(v) if v.isdigit() else None
    return {"saat": secili or BILGI_SAAT_VARSAYILAN, "varsayilan": secili is None,
            "en_az": BILGI_SAAT_MIN, "en_cok": BILGI_SAAT_MAX}


def ozet(db, sensor):
    son = db.execute("SELECT * FROM olcum WHERE sensor = ? ORDER BY ts DESC LIMIT 1", (sensor,)).fetchone()
    if not son:
        return None
    # nem boş olabilir (sensör takılı değil): son geçerli değeri ayrıca ver
    son_nem = db.execute(
        "SELECT ts, nem, ham FROM olcum WHERE sensor = ? AND nem IS NOT NULL ORDER BY ts DESC LIMIT 1", (sensor,)
    ).fetchone()
    zamanlar = [r["ts"] for r in db.execute("SELECT ts FROM olcum WHERE sensor = ? ORDER BY ts DESC LIMIT 30", (sensor,))]
    araliklar = [a - b for a, b in zip(zamanlar, zamanlar[1:])]
    yol = foto_yolu(sensor)
    cihaz = {k: veri.ayar_oku(db, f"{k}:{sensor}", "") for k in ("ip", "ag", "deco", "mac", "yazilim")}
    return {
        "id": sensor,
        "cihaz": cihaz,
        "bitki": veri.ayar_oku(db, f"bitki:{sensor}", sensor),
        "tur": veri.ayar_oku(db, f"tur:{sensor}", ""),
        "aciklama": veri.ayar_oku(db, f"aciklama:{sensor}", ""),
        "son": dict(son),
        "son_nem": dict(son_nem) if son_nem else None,
        "yas_sn": int(time.time() - son["ts"]),
        "aralik_sn": statistics.median(araliklar) if araliklar else None,
        "olcum_sayisi": db.execute("SELECT COUNT(*) c FROM olcum WHERE sensor = ?", (sensor,)).fetchone()["c"],
        "foto": int(os.path.getmtime(yol)) if os.path.exists(yol) else None,
        "ayar_modu": ayar_durumu(sensor, cihaz["ip"]),
        "bilgi_araligi": bilgi_araligi(db, sensor),
    }


@app.get("/api/sensorler")
def api_sensorler():
    db = veri.baglan()
    ids = [r["sensor"] for r in db.execute("SELECT DISTINCT sensor FROM olcum ORDER BY sensor")]
    liste = []
    bas = int(time.time()) - 86400
    for s in ids:
        o = ozet(db, s)
        # kartlardaki küçük eğri: son 24 saat, en fazla ~48 nokta
        o["iz"] = [
            dict(r)
            for r in db.execute(
                "SELECT MIN(ts) ts, AVG(nem) nem FROM olcum WHERE sensor = ? AND ts >= ? AND nem IS NOT NULL"
                " GROUP BY ts / 1800 ORDER BY ts",
                (s, bas),
            )
        ]
        liste.append(o)
    db.close()
    liste.sort(key=lambda x: x["bitki"].casefold())
    return {"mqtt_bagli": durum["mqtt"], "mqtt_sunucu": f"{MQTT_HOST}:{MQTT_PORT}", "sensorler": liste}


@app.get("/api/sensor/{sensor}/durum")
def api_durum(sensor: str):
    db = veri.baglan()
    o = ozet(db, kimlik(sensor))
    db.close()
    if not o:
        raise HTTPException(404, "Sensör yok")
    return {**o, "mqtt_bagli": durum["mqtt"], "mqtt_sunucu": f"{MQTT_HOST}:{MQTT_PORT}"}


@app.get("/api/sensor/{sensor}/gecmis")
def api_gecmis(sensor: str, saat: int = 24):
    saat = max(1, min(saat, 24 * 365))
    bas = int(time.time()) - saat * 3600
    kova = max(1, saat * 3600 // 600)   # en fazla ~600 nokta
    db = veri.baglan()
    rows = db.execute(
        "SELECT MIN(ts) ts, AVG(nem) nem, AVG(ham) ham, AVG(pil) pil, AVG(rssi) rssi FROM olcum"
        " WHERE sensor = ? AND ts >= ? GROUP BY ts / ? ORDER BY ts",
        (kimlik(sensor), bas, kova),
    ).fetchall()
    db.close()
    return {"bas": bas, "bit": int(time.time()), "kova_sn": kova, "noktalar": [dict(r) for r in rows]}


@app.get("/api/sensor/{sensor}/olcumler")
def api_olcumler(sensor: str, adet: int = 50):
    db = veri.baglan()
    rows = db.execute(
        "SELECT * FROM olcum WHERE sensor = ? ORDER BY ts DESC LIMIT ?", (kimlik(sensor), max(1, min(adet, 500)))
    ).fetchall()
    db.close()
    return [dict(r) for r in rows]


@app.put("/api/sensor/{sensor}/aciklama")
async def aciklama_yaz(sensor: str, istek: Request):
    kimlik(sensor)
    metin = (await istek.body()).decode("utf-8", "replace").strip()
    if len(metin) > 2000:
        raise HTTPException(413, "Açıklama en fazla 2000 karakter olabilir")
    db = veri.baglan()
    veri.ayar_yaz(db, f"aciklama:{sensor}", metin)
    db.close()
    return {"ok": True}


@app.put("/api/sensor/{sensor}/ayar-modu")
async def ayar_modu(sensor: str, istek: Request):
    kimlik(sensor)
    ac = (await istek.body()).decode().strip() == "ON"
    i = durum["istemci"]
    if not durum["mqtt"] or i is None:
        raise HTTPException(503, "MQTT sunucusuna bağlı değil")
    if not ac:
        ayar_iptal.add(sensor)
    i.publish(f"{sensor}/ayar_modu", "ON" if ac else None, qos=1, retain=True).wait_for_publish(3)
    return {"ok": True}


def resim_turu(b):
    """Saydamlık korunsun diye PNG ve WebP olduğu gibi saklanır."""
    if b[:3] == b"\xff\xd8\xff":
        return "image/jpeg"
    if b[:8] == b"\x89PNG\r\n\x1a\n":
        return "image/png"
    if b[:4] == b"RIFF" and b[8:12] == b"WEBP":
        return "image/webp"
    return None


@app.put("/api/sensor/{sensor}/bilgi-araligi")
async def bilgi_araligi_yaz(sensor: str, istek: Request):
    kimlik(sensor)
    metin = (await istek.body()).decode().strip()
    if not metin.isdigit() or not BILGI_SAAT_MIN <= int(metin) <= BILGI_SAAT_MAX:
        raise HTTPException(400, f"Aralık {BILGI_SAAT_MIN} ile {BILGI_SAAT_MAX} saat arasında olmalı")
    i = durum["istemci"]
    if not durum["mqtt"] or i is None:
        raise HTTPException(503, "MQTT sunucusuna bağlı değil")
    i.publish(f"{sensor}/bilgi_araligi", metin, qos=1, retain=True).wait_for_publish(3)
    db = veri.baglan()
    veri.ayar_yaz(db, f"bilgi_araligi:{sensor}", metin)
    db.close()
    return {"ok": True, "saat": int(metin)}


@app.put("/api/sensor/{sensor}/foto")
async def foto_yukle(sensor: str, istek: Request):
    yol = foto_yolu(kimlik(sensor))
    govde = await istek.body()
    if len(govde) > FOTO_MAX:
        raise HTTPException(413, "Fotoğraf 12 MB'tan büyük olamaz")
    if not resim_turu(govde):
        raise HTTPException(415, "Dosya JPEG, PNG ya da WebP değil")
    with open(yol + ".gecici", "wb") as f:
        f.write(govde)
    os.replace(yol + ".gecici", yol)
    return {"ok": True}


@app.delete("/api/sensor/{sensor}/foto")
def foto_sil(sensor: str):
    yol = foto_yolu(kimlik(sensor))
    if os.path.exists(yol):
        os.remove(yol)
    return {"ok": True}


@app.get("/foto/{sensor}")
def foto(sensor: str):
    yol = foto_yolu(kimlik(sensor))
    if not os.path.exists(yol):
        raise HTTPException(404, "Fotoğraf yok")
    with open(yol, "rb") as f:
        tur = resim_turu(f.read(16))
    return FileResponse(yol, media_type=tur or "application/octet-stream", headers={"Cache-Control": "no-cache"})


@app.get("/")
def sayfa():
    return FileResponse(os.path.join(STATIK, "index.html"), headers={"Cache-Control": "no-cache"})


@app.get("/saglik")
def saglik():
    return Response("ok" if durum["mqtt"] else "mqtt yok", status_code=200 if durum["mqtt"] else 503)
