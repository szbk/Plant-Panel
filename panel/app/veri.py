"""SQLite kaydı ve MQTT mesajlarını uyanış başına tek satırda toplayan yapı."""
import os
import sqlite3
import threading
import time

VERI_DIR = os.environ.get("VERI_DIR", "/veri")
DB_YOLU = os.path.join(VERI_DIR, "bitki.db")
SESSIZLIK_SN = 3     # bu kadar mesaj gelmezse uyanış bitti sayılır
BIRLESTIR_SN = 60    # ayar modunda 5 sn'lik ölçümler tek satıra birleşir
SENSOR_YOK_V = 0.8   # altı: sensör kablosu takılı değil, nem anlamsız
ILK_SENSOR = "toprak-sensoru"   # iceri_aktar varsayılan sensör kimliği

# MQTT konusundaki sensör adı -> tablo sütunu
SUTUNLAR = {
    "toprak_nemi": "nem",
    "toprak_ham_voltaj": "ham",
    "pil_voltaj__": "pil",
    "pil_seviyesi": "pil_yuzde",
    "wifi_sinyal": "rssi",
    "ba__lanma_s__resi": "baglanma",
    "__nceki_uyan__k_kalma": "uyanik",
    "ka____r__lan_uyan____": "kacirilan",
    "toplam_hata": "toplam_hata",
    "kopma_say__s__": "kopma",
}
METIN_SUTUNLARI = {"son_hata": "son_hata"}
AYAR_KONULARI = {
    "bitki": "bitki",
    "bitki_t__r__": "tur",
    "ip_adresi": "ip",
    "ag_adi": "ag",
    "bagli_deco": "deco",
    "mac_adresi": "mac",
    "yazilim": "yazilim",
}
TUM_SUTUNLAR = list(SUTUNLAR.values()) + list(METIN_SUTUNLARI.values())


def baglan():
    os.makedirs(VERI_DIR, exist_ok=True)
    db = sqlite3.connect(DB_YOLU, timeout=10)
    db.row_factory = sqlite3.Row
    db.execute("PRAGMA journal_mode=WAL")
    db.execute("CREATE TABLE IF NOT EXISTS ayar (k TEXT PRIMARY KEY, v TEXT)")
    tablo = """CREATE TABLE IF NOT EXISTS {ad} (sensor TEXT NOT NULL, ts INTEGER NOT NULL, nem REAL, ham REAL,
        pil REAL, pil_yuzde REAL, rssi REAL, baglanma REAL, uyanik REAL, kacirilan REAL, toplam_hata REAL,
        kopma REAL, son_hata TEXT, PRIMARY KEY (sensor, ts))"""
    db.execute(tablo.format(ad="olcum"))
    return db


def ayar_yaz(db, k, v):
    db.execute("INSERT INTO ayar VALUES (?, ?) ON CONFLICT(k) DO UPDATE SET v = excluded.v", (k, v))
    db.commit()


def ayar_oku(db, k, varsayilan=None):
    r = db.execute("SELECT v FROM ayar WHERE k = ?", (k,)).fetchone()
    return r["v"] if r else varsayilan


class Toplayici:
    """Her sensör için bir uyanıştaki mesajları biriktirir, sessizlik olunca tek satır yazar."""

    def __init__(self):
        self.kilit = threading.Lock()
        self.bekleyen = {}   # sensor -> {"veri": {}, "ilk": ts, "son": ts}

    def ekle(self, sensor, ts, ad, deger):
        """deger metindir. Bilinmeyen ya da sayıya çevrilemeyen değerler atlanır."""
        if ad in SUTUNLAR:
            try:
                d = float(deger)
            except ValueError:
                return
            if d != d:
                return
            sutun = SUTUNLAR[ad]
        elif ad in METIN_SUTUNLARI:
            d, sutun = deger, METIN_SUTUNLARI[ad]
        else:
            return
        with self.kilit:
            b = self.bekleyen.setdefault(sensor, {"veri": {}, "ilk": ts, "son": ts})
            b["veri"][sutun] = d
            b["son"] = ts

    def hazir_olanlar(self, simdi):
        with self.kilit:
            return [k for k, b in self.bekleyen.items() if simdi - b["son"] >= SESSIZLIK_SN]

    def bosalt(self, db, sensor):
        with self.kilit:
            b = self.bekleyen.pop(sensor, None)
        if not b:
            return
        t, ts = b["veri"], int(b["ilk"])
        # aynı konu düzenini kullanan başka ESPHome cihazları toprak sensörü sayılmasın
        kayitli = db.execute("SELECT 1 FROM olcum WHERE sensor = ? LIMIT 1", (sensor,)).fetchone()
        if not kayitli and "ham" not in t and "nem" not in t:
            return
        if t.get("ham") is not None and t["ham"] < SENSOR_YOK_V:
            t["nem"] = None
        onceki = db.execute(
            "SELECT ts FROM olcum WHERE sensor = ? AND ts <= ? ORDER BY ts DESC LIMIT 1", (sensor, ts)
        ).fetchone()
        if onceki and ts - onceki["ts"] < BIRLESTIR_SN:
            ayarlar = ", ".join(f"{k} = ?" for k in t)
            db.execute(f"UPDATE olcum SET {ayarlar} WHERE sensor = ? AND ts = ?", (*t.values(), sensor, onceki["ts"]))
        else:
            sutunlar = ", ".join(t)
            isaretler = ", ".join("?" for _ in t)
            db.execute(f"INSERT INTO olcum (sensor, ts, {sutunlar}) VALUES (?, ?, {isaretler})", (sensor, ts, *t.values()))
        db.commit()


def simdi():
    return time.time()
