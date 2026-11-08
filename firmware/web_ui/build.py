#!/usr/bin/env python3
"""arayuz/index.html -> ayar_ui.h (gzip'li C dizisi). Derlemeden önce çalıştır."""
import gzip, pathlib
k = pathlib.Path(__file__).resolve().parent
html = (k / "index.html").read_bytes()
gz = gzip.compress(html, 9, mtime=0)
satirlar = [", ".join(str(b) for b in gz[i:i + 24]) for i in range(0, len(gz), 24)]
out = ("#pragma once\n// OTOMATIK URETILDI: arayuz/build.py (kaynak: arayuz/index.html). Elle duzenleme.\n"
       "#include <cstdint>\n"
       f"static const uint8_t AYAR_UI_GZ[{len(gz)}] = {{\n  " + ",\n  ".join(satirlar) + "\n};\n")
(k.parent / "ayar_ui.h").write_text(out)
print(f"ayar_ui.h: {len(html)} B html -> {len(gz)} B gzip")
