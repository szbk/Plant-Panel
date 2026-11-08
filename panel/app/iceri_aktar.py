"""Geçmişi içeri aktarır: `mosquitto_sub -t '#' -F '%I %t %p'` çıktısını uyanışlara böler, olcum tablosuna yazar.
Kullanım: python -m app.iceri_aktar /mqtt.log 2026-11-06T11:35:00
"""
import datetime as dt
import re
import sys

from . import veri

SENSOR = veri.ILK_SENSOR
SATIR = re.compile(rb"^(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d[+-]\d{4}) toprak-sensoru/sensor/([^/]+)/state ?(.*)$")


def main(yol, baslangic):
    bas = dt.datetime.fromisoformat(baslangic).astimezone().timestamp()
    db = veri.baglan()
    t = veri.Toplayici()
    adet = 0
    with open(yol, "rb") as f:
        for ham in f:
            m = SATIR.match(ham.rstrip(b"\n"))
            if not m:
                continue
            ts = dt.datetime.strptime(m[1].decode(), "%Y-%m-%dT%H:%M:%S%z").timestamp()
            if ts < bas:
                continue
            if t.hazir_olanlar(ts):
                t.bosalt(db, SENSOR)
                adet += 1
            t.ekle(SENSOR, ts, m[2].decode(), m[3].decode("utf-8", "replace"))
    t.bosalt(db, SENSOR)
    print(f"{adet + 1} uyanış işlendi, tabloda {db.execute('SELECT COUNT(*) c FROM olcum').fetchone()['c']} satır")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
