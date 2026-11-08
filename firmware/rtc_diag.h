#pragma once
// Derin uykuda korunan teşhis sayaçları (RTC belleği; güç kesilince sıfırlanır).
#include "esp_attr.h"
#include <cstdint>
#include <cstring>

struct RtcDiag {
  uint32_t magic;
  uint32_t dongu;          // soğuk açılıştan beri uyanış sayısı
  uint32_t kacirilan;      // son başarılı gönderimden beri kaçırılan uyanış
  uint32_t toplam_hata;    // soğuk açılıştan beri başarısız uyanış
  uint32_t kopma;          // soğuk açılıştan beri bağlantı kopup yeniden kurulan uyanış
  uint32_t son_hata;       // 0 yok, 1 WiFi, 2 MQTT, 3 kopma, 4 teslim onayı gelmedi
  uint32_t hizli_deneme;   // art arda yapılan 5 dk'lık yeniden denemeler
  uint32_t onceki_uyanik;  // bir önceki uyanışın süresi (ms)
  uint32_t wifi_ardisik;   // art arda WiFi'a bağlanamayan uyanış (3 olunca kurulum ağı açılır)
};
static const uint32_t RTC_DIAG_MAGIC = 0xD1A60002;
RTC_DATA_ATTR static RtcDiag rtc_diag;
static bool rtc_soguk_acilis = false;

static inline const char *hata_adi(uint32_t k) {
  switch (k) { case 1: return "WiFi"; case 2: return "MQTT"; case 3: return "Kopma"; case 4: return "Teslim"; default: return "Yok"; }
}
