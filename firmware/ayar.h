#pragma once
// =====================================================================
//  Toprak sensörü — kalıcı ayarlar + cihaz içi ayar arayüzü (HTTP API)
//  ESPHome web_server_base üzerinde kendi handler'ımız; arayüz sayfası
//  ayar_ui.h içinde (arayuz/index.html'den build.py ile üretilir).
//  Web sunucusu SADECE ayar modunda başlar; normal uyanışa yük yok.
// =====================================================================
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <deque>
#include <functional>
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_attr.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"
#include "esphome/core/log.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "ayar_ui.h"

namespace toprak {
using namespace esphome;
static const char *const ATAG = "ayar";

// ---------------- kalıcı ayarlar ----------------
static const uint32_t AYAR_SURUM = 0xA1A70002;
struct Ayarlar {
  uint32_t surum;
  char ssid[33];
  char sifre[65];
  uint8_t dhcp;          // 1 = DHCP, 0 = sabit IP
  uint8_t ip[4], gw[4], mask[4], dns[4];
  char mqtt_host[64];
  uint16_t mqtt_port;
  char mqtt_user[33];
  char mqtt_pass[65];
  uint16_t uyku_dk;
  float pil_kesim;
  float kuru_v;
  float islak_v;
} __attribute__((packed));

// Kuru ile ıslak arasında en az bu kadar fark olmalı (gerçek toprakta fark suya göre küçük)
static const float KAL_MIN_FARK = 0.2f;

static Ayarlar A{};
static ESPPreferenceObject a_pref;
// --- WiFi değişikliği güvenliği (v8) ---
// Arayüzden yeni ağ kaydedilince eski ayarlar 'yedek'e yazılır ve yeni ağ DENENİR. Yeniden başlatmadan
// sonra WIFI_DENEME_MS içinde bağlanamazsa yedeğe dönülür ve cihaz ayar modunda yeniden açılır.
// Ayrıca kayıtlı özel ağ art arda WIFI_VARSAYILAN_ESIK uyanış bağlanamazsa (ağ şifresi sonradan değişti
// vb.) o uyanışta yazılımdaki varsayılan ağ listesi kullanılır.
static Ayarlar A_yedek{};
static ESPPreferenceObject a_yedek_pref;
static const uint32_t WIFI_DENEME_MAGIC = 0x7E57A1F1;
static const uint32_t WIFI_GERI_MAGIC = 0x6E21A1F1;
static const uint32_t WIFI_DENEME_MS = 45000;
static const uint32_t WIFI_VARSAYILAN_ESIK = 2;
RTC_NOINIT_ATTR static uint32_t rtc_wifi_deneme;     // yeni ağ deneniyor
RTC_NOINIT_ATTR static char rtc_deneme_tani[128];  // geri alma anındaki ağ durumu (rtc_wifi_geri geçerliyse)
RTC_NOINIT_ATTR static uint32_t rtc_wifi_geri;       // yeni ağ başarısız oldu, geri alındı (arayüze bildirilir)
static bool a_wifi_deneme = false;
// Belirli bir erişim noktasına (BSSID) sabitleme. Ayrı kayıt: Ayarlar yapısı değişmesin.
static const uint32_t HEDEF_SURUM = 0xDEC00001;
struct WifiHedef {
  uint32_t surum;
  uint8_t bssid[6];
  uint8_t kanal;     // 0 = sabitleme yok
} __attribute__((packed));
static WifiHedef H{};
static ESPPreferenceObject h_pref;
// v8.1: "başarısız uyanış" = MQTT'ye ulaşılamadı (WiFi bağlı görünse bile: yanlış IP/ağ geçidi).
// Özel ağ kullanılırken art arda başarısız uyanışlarda dönüşümlü olarak derlemedeki ağ denenir;
// o çalışırsa (soğuk açılışa ya da yeni ayara kadar) onda kalınır.
RTC_DATA_ATTR static uint32_t rtc_ozel_hata = 0;
RTC_DATA_ATTR static bool rtc_varsayilan_kullan = false;
static bool a_wifi_varsayilan = false;               // özel ağ atlandı, varsayılan ağ kullanılıyor
static bool a_ozel_wifi = false;   // ssid ayarlıysa derlemedeki ağ listesi yerine bu kullanılır
static bool a_ozel_mqtt = false;

// Yeniden başlatma sonrası ayar modunda kalmak için (yazılım reset'inde korunur)
static const uint32_t AYAR_ISTEK_MAGIC = 0x5E77AB1E;
RTC_NOINIT_ATTR static uint32_t rtc_ayar_istek;

static void ayar_varsayilan_() {
  memset(&A, 0, sizeof(A));
  A.surum = AYAR_SURUM;
  A.dhcp = 1;
  A.mqtt_port = 1883;
  A.uyku_dk = 30;
  A.pil_kesim = 3.40f;
  A.kuru_v = 2.51f;
  A.islak_v = 1.14f;
}

inline void ayar_yukle() {
  a_yedek_pref = global_preferences->make_preference<Ayarlar>(0x70A1A9u, true);
  a_pref = global_preferences->make_preference<Ayarlar>(0x70A1A7u, true);
  if (!a_pref.load(&A) || A.surum != AYAR_SURUM) {
    ayar_varsayilan_();
    ESP_LOGI(ATAG, "Ayarlar varsayilan (kayit yok)");
  }
  if (A.uyku_dk < 1 || A.uyku_dk > 1440) A.uyku_dk = 30;
  if (!(A.kuru_v > A.islak_v + KAL_MIN_FARK)) { A.kuru_v = 2.51f; A.islak_v = 1.14f; }
  a_ozel_wifi = A.ssid[0] != 0;
  a_ozel_mqtt = A.mqtt_host[0] != 0;
  h_pref = global_preferences->make_preference<WifiHedef>(0x70A1AAu, true);
  if (!h_pref.load(&H) || H.surum != HEDEF_SURUM) { memset(&H, 0, sizeof(H)); H.surum = HEDEF_SURUM; }
}
inline void hedef_kaydet() {
  H.surum = HEDEF_SURUM;
  h_pref.save(&H);
  global_preferences->sync();
}
inline void hedef_temizle() {
  if (H.kanal == 0) return;
  memset(H.bssid, 0, 6);
  H.kanal = 0;
  hedef_kaydet();
}
inline void ayar_kaydet() {
  A.surum = AYAR_SURUM;
  a_pref.save(&A);
  global_preferences->sync();
}

// ---------------- bitki profili (ayrı kayıt: Ayarlar yapısı değişmesin, eski ayarlar korunsun) ----------------
static const uint32_t BITKI_SURUM = 0xB1770001;
struct Bitki {
  uint32_t surum;
  char ad[49];    // görünen ad, ör. "Difenbahya" (UTF-8, en fazla 48 bayt)
  char tur[65];   // bilimsel ad, ör. "Dieffenbachia seguine" (OpenPlantbook araması için)
} __attribute__((packed));
static Bitki B{};
static ESPPreferenceObject b_pref;
static text_sensor::TextSensor *ts_bitki = nullptr, *ts_tur = nullptr;

inline void bitki_yukle() {
  b_pref = global_preferences->make_preference<Bitki>(0x70A1A8u, true);
  if (!b_pref.load(&B) || B.surum != BITKI_SURUM) {
    memset(&B, 0, sizeof(B));
    B.surum = BITKI_SURUM;
  }
  B.ad[48] = 0;
  B.tur[64] = 0;
}
inline void bitki_kaydet() {
  B.surum = BITKI_SURUM;
  b_pref.save(&B);
  global_preferences->sync();
}
inline std::string bitki_ad() { return std::string(B.ad); }
inline std::string bitki_tur() { return std::string(B.tur); }
// Panelde gösterilen cihaz bilgileri (keşif turlarında ve ayar modunda yayınlanır)
inline std::string cihaz_ip() {
  char b[40] = "";
  auto *w = wifi::global_wifi_component;
  if (w->is_connected()) w->get_ip_addresses()[0].str_to(b);
  return b;
}
inline std::string cihaz_ag() {
  char s[wifi::SSID_BUFFER_SIZE] = "";
  return wifi::global_wifi_component->wifi_ssid_to(s);
}
inline std::string cihaz_deco() {
  auto b = wifi::global_wifi_component->wifi_bssid();
  char t[32];
  snprintf(t, sizeof t, "%02X:%02X:%02X:%02X:%02X:%02X k%u", b[0], b[1], b[2], b[3], b[4], b[5],
           (unsigned) wifi::global_wifi_component->get_wifi_channel());
  return t;
}
inline std::string cihaz_mac() {
  char m[18];
  return get_mac_address_pretty_into_buffer(m);
}
inline std::string cihaz_yazilim() {
  char b[Application::BUILD_TIME_STR_SIZE];
  App.get_build_time_string(b);
  return b;
}
// Cihaz bilgisi gönderme aralığı: panel toprak-sensoru/bilgi_araligi konusuna retained saat yazar.
// RTC'de tutulur (güç/OTA sonrası sıfırlanır; o zaman zaten soğuk açılış gönderimi yapılır ve
// retained değer bağlanınca yeniden gelir). Süre, uykuya girerken eklenen uyku süreleriyle izlenir.
static const uint32_t BILGI_SAAT_MIN = 4, BILGI_SAAT_MAX = 720, BILGI_SAAT_VARSAYILAN = 24;
RTC_DATA_ATTR static uint32_t rtc_bilgi_saat = 0;      // 0: panelden değer gelmedi -> varsayılan
RTC_DATA_ATTR static uint32_t rtc_bilgi_gecen_sn = 0;  // son cihaz bilgisi gönderiminden beri (yaklaşık)
inline uint32_t bilgi_saat() { return rtc_bilgi_saat ? rtc_bilgi_saat : BILGI_SAAT_VARSAYILAN; }
inline bool bilgi_zamani() { return rtc_soguk_acilis || rtc_bilgi_gecen_sn >= bilgi_saat() * 3600UL; }
inline void bilgi_gonderildi() { rtc_bilgi_gecen_sn = 0; }
inline void bilgi_uyku_ekle(uint32_t uyku_ms) { rtc_bilgi_gecen_sn += (uyku_ms + millis()) / 1000; }
inline void bilgi_araligi_al(const std::string &s) {
  long h = strtol(s.c_str(), nullptr, 10);
  if (h <= 0) { rtc_bilgi_saat = 0; return; }   // boş/geçersiz: varsayılan
  if (h < (long) BILGI_SAAT_MIN) h = BILGI_SAAT_MIN;
  if (h > (long) BILGI_SAAT_MAX) h = BILGI_SAAT_MAX;
  if ((uint32_t) h != rtc_bilgi_saat) ESP_LOGI("bilgi", "cihaz bilgisi araligi %ld saat", h);
  rtc_bilgi_saat = h;
}
inline void bitki_baglan(text_sensor::TextSensor *ad, text_sensor::TextSensor *tur) { ts_bitki = ad; ts_tur = tur; }

// --- uygulanan değerler (YAML lambdalarından okunur) ---
inline uint32_t ayar_uyku_ms() { return (uint32_t) A.uyku_dk * 60000UL; }
inline float ayar_pil_kesim() { return A.pil_kesim; }
inline float ayar_nem(float v) {
  float y = (A.kuru_v - v) / (A.kuru_v - A.islak_v) * 100.0f;
  return y < 0 ? 0 : (y > 100 ? 100 : y);
}

// Önce ayar_wifi_karar(wifi_ardisik), sonra ayar_uygula_wifi() (ikisi de WiFi kurulmadan önce)
inline void ayar_wifi_karar(uint32_t wifi_ardisik) {
  a_wifi_deneme = rtc_wifi_deneme == WIFI_DENEME_MAGIC;
  if (a_wifi_deneme) {
    ESP_LOGW(ATAG, "Yeni WiFi deneniyor (%s), %u sn icinde MQTT'ye ulasilamazsa geri alinacak", A.ssid, WIFI_DENEME_MS / 1000);
  } else if (a_ozel_wifi && (rtc_varsayilan_kullan || (rtc_ozel_hata % 2) == 1)) {
    a_ozel_wifi = false;
    a_wifi_varsayilan = true;
    ESP_LOGW(ATAG, "Kayitli ag (%s) ile MQTT'ye ulasilamadi (%u): bu uyanista varsayilan ag", A.ssid, rtc_ozel_hata);
  }
  (void) wifi_ardisik;
}
// mqtt on_connect'ten: bağlantı gerçekten çalışıyor
inline void ayar_mqtt_baglandi() {
  if (a_wifi_deneme) {
    a_wifi_deneme = false;
    rtc_wifi_deneme = 0;
    ESP_LOGI(ATAG, "Yeni WiFi basarili (MQTT'ye ulasildi), kalici");
  }
  rtc_varsayilan_kullan = a_wifi_varsayilan;
  rtc_ozel_hata = 0;
}
// başarısız uyanışta (MQTT'ye ulaşılamadı) YAML'dan
inline void ayar_uyanis_basarisiz() {
  if (A.ssid[0]) rtc_ozel_hata++;
  if (a_wifi_varsayilan) rtc_varsayilan_kullan = false;   // varsayılan da çalışmadı
}
// on_boot'ta zamanlayıcı kurar; deneme süresinde bağlanamazsa yedeğe dön
inline void ayar_wifi_deneme_izle();

// WiFi bileşeni kurulmadan ÖNCE çağrılmalı (on_boot priority > 250)
inline void ayar_uygula_wifi() {
  if (!a_ozel_wifi) return;
  auto *w = wifi::global_wifi_component;
  w->clear_sta();
  wifi::WiFiAP ap;
  ap.set_ssid(A.ssid);
  ap.set_password(A.sifre);
  if (!A.dhcp) {
    wifi::ManualIP m{};
    m.static_ip = network::IPAddress(A.ip[0], A.ip[1], A.ip[2], A.ip[3]);
    m.gateway = network::IPAddress(A.gw[0], A.gw[1], A.gw[2], A.gw[3]);
    m.subnet = network::IPAddress(A.mask[0], A.mask[1], A.mask[2], A.mask[3]);
    m.dns1 = network::IPAddress(A.dns[0], A.dns[1], A.dns[2], A.dns[3]);
    ap.set_manual_ip(m);
    ESP_LOGI(ATAG, "Sabit IP %u.%u.%u.%u gw %u.%u.%u.%u mask %u.%u.%u.%u dns %u.%u.%u.%u", A.ip[0], A.ip[1], A.ip[2], A.ip[3],
             A.gw[0], A.gw[1], A.gw[2], A.gw[3], A.mask[0], A.mask[1], A.mask[2], A.mask[3], A.dns[0], A.dns[1], A.dns[2], A.dns[3]);
  }
  if (H.kanal) {   // seçilen erişim noktasına sabitle; ayar modunda başkasına gezinmesin
    wifi::bssid_t b;
    std::copy(H.bssid, H.bssid + 6, b.begin());
    ap.set_bssid(b);
    ap.set_channel(H.kanal);
    w->set_post_connect_roaming(false);
    ESP_LOGI(ATAG, "Erisim noktasi sabit: %02X:%02X:%02X:%02X:%02X:%02X kanal %u", H.bssid[0], H.bssid[1], H.bssid[2], H.bssid[3],
             H.bssid[4], H.bssid[5], H.kanal);
  }
  // Aynı ağ iki kez: ESP32'nin ilk bağlanma denemesi sık sık 'Auth Failed' alıyor,
  // fast_connect ikinci kayda taramadan geçiyor (bkz. toprak-sensoru.yaml, wifi: networks)
  w->add_sta(ap);
  w->add_sta(ap);
  ESP_LOGI(ATAG, "WiFi: kayitli ag kullaniliyor (%s, %s)", A.ssid, A.dhcp ? "DHCP" : "sabit IP");
}
inline void ayar_uygula_mqtt() {
  if (!a_ozel_mqtt) return;
  auto *m = mqtt::global_mqtt_client;
  m->set_broker_address(A.mqtt_host);
  m->set_broker_port(A.mqtt_port ? A.mqtt_port : 1883);
  m->set_username(A.mqtt_user);
  m->set_password(A.mqtt_pass);
  ESP_LOGI(ATAG, "MQTT: %s:%u", A.mqtt_host, A.mqtt_port);
}
// fast_connect kaydında hep 1. kayıtla başla (BSSID/kanalı ESPHome kendisi öğrenip saklıyor)
inline void ayar_fast_connect_sabitle() {
  auto pref = global_preferences->make_preference<wifi::SavedWifiFastConnectSettings>(
      App.get_config_version_hash() + 1, false);
  wifi::SavedWifiFastConnectSettings fc{};
  if (pref.load(&fc) && fc.ap_index > 0) {
    fc.ap_index = 0;
    pref.save(&fc);
  }
}
inline void ayar_fast_connect_sil() {
  auto pref = global_preferences->make_preference<wifi::SavedWifiFastConnectSettings>(
      App.get_config_version_hash() + 1, false);
  wifi::SavedWifiFastConnectSettings fc{};
  fc.ap_index = -1;  // geçersiz → normal tarama
  pref.save(&fc);
}

// ---------------- ayar modu ----------------
static bool am_aktif = false;
static bool am_cikis = false;
static uint32_t am_son = 0;                 // son etkinlik (millis)
static const uint32_t AM_SURE = 10 * 60 * 1000UL;
static sensor::Sensor *s_ham = nullptr, *s_nem = nullptr, *s_pil = nullptr, *s_pil_y = nullptr;
static PollingComponent *pc_ham = nullptr, *pc_pil = nullptr;   // RTTI yok: dynamic_cast kullanılamaz

inline bool ayar_istek_var() { return rtc_ayar_istek == AYAR_ISTEK_MAGIC; }
inline void ayar_istek_temizle() { rtc_ayar_istek = 0; }
inline void ayar_bildirim_temizle() { rtc_wifi_geri = 0; }
inline bool ayar_boot_tusu_ile_uyandi() {
  return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
}
// YAML'dan: ayar_baglan(id(toprak_ham), id(toprak_ham), id(toprak_nemi), id(pil_voltaj), id(pil_voltaj), id(pil_seviyesi))
inline void ayar_baglan(PollingComponent *ham_pc, sensor::Sensor *ham, sensor::Sensor *nem, PollingComponent *pil_pc,
                        sensor::Sensor *pil, sensor::Sensor *pil_y) {
  pc_ham = ham_pc; s_ham = ham; s_nem = nem; pc_pil = pil_pc; s_pil = pil; s_pil_y = pil_y;
}
inline bool ayar_bitti() { return am_cikis || (millis() - am_son > AM_SURE); }
inline uint32_t ayar_kalan_sn() {
  uint32_t g = millis() - am_son;
  return g >= AM_SURE ? 0 : (AM_SURE - g) / 1000;
}

// ana döngüde çalıştırılacak işler (HTTP iş parçacığından gelir)
static std::mutex q_mtx;
static std::deque<std::function<void()>> q_isler;
static uint8_t q_anahtar;
inline void ana_donguye_(std::function<void()> &&f) {
  {
    std::lock_guard<std::mutex> l(q_mtx);
    q_isler.push_back(std::move(f));
  }
  App.scheduler.set_timeout(&q_anahtar, 0, []() {
    std::deque<std::function<void()>> d;
    {
      std::lock_guard<std::mutex> l(q_mtx);
      d.swap(q_isler);
    }
    for (auto &f : d) f();
  });
}

// Yeniden başlatma: yeni yüklenen yazılım ancak açılıştan 10 sn sonra (safe_mode boot_is_good_after)
// "sağlam" işaretlenir. Ondan önce yeniden başlarsa önyükleyici ESKİ yazılıma döner (OTA rollback).
// Bu yüzden açılıştan en az 12 sn geçmeden yeniden başlatma.
inline void ayar_yeniden_baslat_(uint8_t anahtar, uint32_t gecikme) {
  uint32_t g = millis() < 12000 ? std::max<uint32_t>(gecikme, 12000 - millis()) : gecikme;
  App.scheduler.set_timeout(&q_anahtar + anahtar, g, []() { App.safe_reboot(); });
}
inline void ayar_wifi_deneme_izle() {
  if (!a_wifi_deneme) return;
  App.scheduler.set_timeout(&q_anahtar + 3, WIFI_DENEME_MS, []() {
    if (!a_wifi_deneme || mqtt::global_mqtt_client->is_connected()) return;
    ESP_LOGW(ATAG, "Yeni WiFi (%s) %u sn'de baglanamadi: eski ayarlara donuluyor", A.ssid, WIFI_DENEME_MS / 1000);
    {  // teşhis: hata WiFi'da mı, IP'de mi, sunucuya giden yolda mı?
      wifi_ap_record_t r{};
      bool bagli = esp_wifi_sta_get_ap_info(&r) == ESP_OK;
      esp_netif_ip_info_t n{};
      auto *nf = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      if (nf) esp_netif_get_ip_info(nf, &n);
      snprintf(rtc_deneme_tani, sizeof rtc_deneme_tani,
               "wifi=%d bssid=%02X:%02X:%02X:%02X:%02X:%02X rssi=%d kanal=%u ip=" IPSTR " gw=" IPSTR " mask=" IPSTR, bagli,
               r.bssid[0], r.bssid[1], r.bssid[2], r.bssid[3], r.bssid[4], r.bssid[5], r.rssi, r.primary, IP2STR(&n.ip),
               IP2STR(&n.gw), IP2STR(&n.netmask));
      ESP_LOGW(ATAG, "Deneme: %s", rtc_deneme_tani);
    }
    Ayarlar y{};
    if (a_yedek_pref.load(&y) && y.surum == AYAR_SURUM) A = y;
    else { A.ssid[0] = 0; A.sifre[0] = 0; }   // yedek yoksa yazılımdaki ağa dön
    ayar_kaydet();
    hedef_temizle();
    ayar_fast_connect_sil();
    rtc_wifi_deneme = 0;
    rtc_wifi_geri = WIFI_GERI_MAGIC;
    rtc_ayar_istek = AYAR_ISTEK_MAGIC;       // ayar modunda açıl, kullanıcı görsün
    ayar_yeniden_baslat_(4, 300);
  });
}

// ---- küçük JSON yardımcıları ----
struct Js {
  std::string s;
  bool ilk = true;
  Js() { s.reserve(1024); s += '{'; }
  void k_(const char *k) { if (!ilk) s += ','; ilk = false; s += '"'; s += k; s += "\":"; }
  void str(const char *k, const char *v) {
    k_(k); s += '"';
    for (const char *p = v; *p; p++) {
      char c = *p;
      if (c == '"' || c == '\\') { s += '\\'; s += c; }
      else if ((uint8_t) c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); s += b; }
      else s += c;
    }
    s += '"';
  }
  void num(const char *k, double v, int dec = 2) {
    k_(k);
    if (std::isnan(v)) { s += "null"; return; }
    char b[24]; snprintf(b, sizeof b, "%.*f", dec, v); s += b;
  }
  void boo(const char *k, bool v) { k_(k); s += v ? "true" : "false"; }
  void raw(const char *k, const std::string &v) { k_(k); s += v; }
  std::string bitir() { s += '}'; return s; }
};
static void ip_yaz_(char *b, const uint8_t *ip) { snprintf(b, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]); }
static bool ip_oku_(const std::string &t, uint8_t *ip) {
  unsigned a, b, c, d;
  if (sscanf(t.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a > 255 || b > 255 || c > 255 || d > 255) return false;
  ip[0] = a; ip[1] = b; ip[2] = c; ip[3] = d;
  return true;
}

class AyarArayuz : public AsyncWebHandler {
 public:
  bool canHandle(AsyncWebServerRequest *req) const override {
    if (!am_aktif) return false;
    char ub[AsyncWebServerRequest::URL_BUF_SIZE];
    StringRef url = req->url_to(ub);
    if (strncmp(url.c_str(), "/api/", 5) == 0) return true;
    if (req->method() != HTTP_GET) return false;
    if (url == "/" || url == "/index.html") return true;
    // Kurulum ağında (AP) işletim sistemlerinin 'captive portal' kontrollerine de bu sayfayı ver
    if (wifi::global_wifi_component->is_ap_active() && url != "/update") return true;
    return false;
  }
  void handleRequest(AsyncWebServerRequest *req) override {
    am_son = millis();
    char ub[AsyncWebServerRequest::URL_BUF_SIZE];
    StringRef url = req->url_to(ub);
    const char *u = url.c_str();
    if (strcmp(u, "/api/durum") == 0) return this->durum_(req);
    if (strcmp(u, "/api/ayarlar") == 0) {
      if (req->method() == HTTP_POST) return this->ayar_kaydet_(req);
      return this->ayarlar_(req);
    }
    if (strcmp(u, "/api/agler") == 0) return this->agler_(req);
    if (strcmp(u, "/api/tara") == 0) {
      ana_donguye_([]() { tara_(); });
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/kalibre") == 0) {
      bool kuru = req->arg("tip") == "kuru";
      // Ön kontrol (son canlı ölçümle): yanlış ortamda basılırsa kalibrasyonu bozma
      float simdi = s_ham ? s_ham->get_state() : NAN;
      if (!std::isnan(simdi)) {
        char h[200] = "";
        if (!kuru && simdi > A.kuru_v - KAL_MIN_FARK)
          snprintf(h, sizeof h, "Sensör suda görünmüyor (%.2f V, kuru değer %.2f V). Beyaz çizgiye kadar suya batırıp tekrar dene.", simdi, A.kuru_v);
        if (kuru && simdi < A.islak_v + KAL_MIN_FARK)
          snprintf(h, sizeof h, "Sensör kuru görünmüyor (%.2f V, ıslak değer %.2f V). Kurulayıp havada tutarak tekrar dene.", simdi, A.islak_v);
        if (h[0]) {
          Js j; j.boo("ok", false); j.str("hata", h);
          req->send(400, "application/json", j.bitir().c_str());
          return;
        }
      }
      ana_donguye_([kuru]() {
        if (s_ham == nullptr || pc_ham == nullptr) return;
        float t = 0; int n = 0;
        for (int i = 0; i < 5; i++) {   // birkaç taze ölçümün ortalaması
          pc_ham->update();
          if (!std::isnan(s_ham->get_state())) { t += s_ham->get_state(); n++; }
        }
        if (!n) return;
        float v = t / n;
        float k = kuru ? v : A.kuru_v, i = kuru ? A.islak_v : v;
        if (!(k > i + KAL_MIN_FARK)) {   // son güvenlik: kuru ile ıslak arasında anlamlı fark yoksa kaydetme
          ESP_LOGW(ATAG, "Kalibrasyon reddedildi: kuru %.3f, islak %.3f", k, i);
          return;
        }
        A.kuru_v = k; A.islak_v = i;
        ayar_kaydet();
        ESP_LOGI(ATAG, "Kalibrasyon %s = %.3f V", kuru ? "kuru" : "islak", v);
        if (s_nem) s_nem->publish_state(ayar_nem(s_ham->get_state()));
      });
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/olc") == 0) {
      ana_donguye_([]() { olc_(); });
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/mqtt_test") == 0) {
      ana_donguye_([]() {
        a_ozel_mqtt = A.mqtt_host[0] != 0;
        ayar_uygula_mqtt();
        mqtt::global_mqtt_client->disable();
        mqtt::global_mqtt_client->enable();
      });
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/yeniden_baslat") == 0) {
      rtc_ayar_istek = AYAR_ISTEK_MAGIC;
      ayar_yeniden_baslat_(1, 800);
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/deco_sec") == 0) return this->deco_sec_(req);
    if (strcmp(u, "/api/wifi_varsayilan") == 0) {
      // Kayıtlı (özel) WiFi'ı sil: yazılımdaki ağ listesi (sabit BSSID ve IP) kullanılsın
      ana_donguye_([]() {
        A.ssid[0] = 0; A.sifre[0] = 0; A.dhcp = 1;
        ayar_kaydet();
        hedef_temizle();
        rtc_ozel_hata = 0; rtc_varsayilan_kullan = false; rtc_wifi_deneme = 0;
        rtc_ayar_istek = AYAR_ISTEK_MAGIC;
        ESP_LOGI(ATAG, "Ozel WiFi silindi, varsayilan aga donuluyor");
        ayar_yeniden_baslat_(5, 500);
      });
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/teshis_sifirla") == 0) {
      // Sayaçları sıfırla (uyanış sayısı ve önceki uyanık kalma süresi kalır)
      ana_donguye_([]() {
        rtc_diag.kacirilan = 0; rtc_diag.toplam_hata = 0; rtc_diag.kopma = 0;
        rtc_diag.son_hata = 0; rtc_diag.hizli_deneme = 0; rtc_diag.wifi_ardisik = 0;
        ESP_LOGI(ATAG, "Teshis sayaclari sifirlandi");
      });
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/ayar_istek") == 0) {
      // Yazılım yüklemeden önce: yükleme sonrası yeniden başlayınca ayar modunda açıl
      // (RTC_NOINIT yazılım reset'inde korunur; ayar modu bitince temizlenir)
      rtc_ayar_istek = AYAR_ISTEK_MAGIC;
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/cik") == 0) {
      am_cikis = true;
      return this->tamam_(req);
    }
    if (strcmp(u, "/api/fabrika") == 0) {
      ana_donguye_([]() {
        global_preferences->reset();
        rtc_ayar_istek = AYAR_ISTEK_MAGIC;
        ayar_yeniden_baslat_(2, 800);
      });
      return this->tamam_(req);
    }
    if (strncmp(u, "/api/", 5) == 0) { req->send(404, "application/json", "{\"hata\":\"yok\"}"); return; }
    // arayüz sayfası
    auto *r = req->beginResponse(200, "text/html", AYAR_UI_GZ, sizeof(AYAR_UI_GZ));
    r->addHeader("Content-Encoding", "gzip");
    r->addHeader("Cache-Control", "no-store");
    req->send(r);
  }

  static void tara_() {
    auto *w = wifi::global_wifi_component;
    w->set_keep_scan_results(true);
    if (w->is_connected()) {
      wifi_scan_config_t c{};
      c.show_hidden = false;
      esp_wifi_scan_start(&c, false);  // sonuçları ESPHome'un olay işleyicisi toplar
    }
    // bağlı değilken (kurulum ağı) ESPHome kendisi tarıyor
  }
  static void olc_() {
    if (pc_ham) pc_ham->update();
    if (pc_pil) pc_pil->update();
  }

 protected:
  void tamam_(AsyncWebServerRequest *req) { req->send(200, "application/json", "{\"ok\":true}"); }

  void durum_(AsyncWebServerRequest *req) {
    auto *w = wifi::global_wifi_component;
    Js j;
    j.str("ad", App.get_name().c_str());
    char bt[Application::BUILD_TIME_STR_SIZE];
    App.get_build_time_string(bt);
    j.str("derleme", bt);
    char mac[18];
    j.str("mac", get_mac_address_pretty_into_buffer(mac));
    bool bagli = w->is_connected();
    j.boo("wifi_bagli", bagli);
    char b[40] = "";
    if (bagli) {
      w->get_ip_addresses()[0].str_to(b);
      j.str("ip", b);
      char ss[wifi::SSID_BUFFER_SIZE];
      j.str("ssid", w->wifi_ssid_to(ss));
      j.num("rssi", w->wifi_rssi(), 0);
      j.num("kanal", w->get_wifi_channel(), 0);
      auto bs = w->wifi_bssid();
      char bb[18];
      snprintf(bb, sizeof bb, "%02X:%02X:%02X:%02X:%02X:%02X", bs[0], bs[1], bs[2], bs[3], bs[4], bs[5]);
      j.str("bssid", bb);
    }
    j.boo("deco_sabit", a_ozel_wifi && H.kanal != 0);
    j.boo("ap", w->is_ap_active());
    if (w->is_ap_active()) { w->wifi_soft_ap_ip().str_to(b); j.str("ap_ip", b); }
    j.boo("mqtt_bagli", mqtt::global_mqtt_client->is_connected());
    j.num("ham_v", s_ham ? s_ham->get_state() : NAN, 3);
    j.num("nem", s_nem ? s_nem->get_state() : NAN, 0);
    j.num("pil_v", s_pil ? s_pil->get_state() : NAN, 2);
    j.num("pil_yuzde", s_pil_y ? s_pil_y->get_state() : NAN, 0);
    j.num("calisma_sn", millis() / 1000.0, 0);
    j.num("kalan_sn", ayar_kalan_sn(), 0);
    j.num("kacirilan", rtc_diag.kacirilan, 0);
    j.num("toplam_hata", rtc_diag.toplam_hata, 0);
    j.num("kopma", rtc_diag.kopma, 0);
    j.str("son_hata", hata_adi(rtc_diag.son_hata));
    j.num("dongu", rtc_diag.dongu, 0);
    j.num("onceki_uyanik", rtc_diag.onceki_uyanik, 0);
    j.num("wifi_ardisik", rtc_diag.wifi_ardisik, 0);
    j.num("hizli_deneme", rtc_diag.hizli_deneme, 0);
    j.boo("wifi_geri_alindi", rtc_wifi_geri == WIFI_GERI_MAGIC);
    if (rtc_wifi_geri == WIFI_GERI_MAGIC) {
      rtc_deneme_tani[sizeof(rtc_deneme_tani) - 1] = 0;
      j.str("deneme_tani", rtc_deneme_tani);
    }
    j.boo("wifi_varsayilan", a_wifi_varsayilan);
    j.boo("wifi_deneme", a_wifi_deneme);
    req->send(200, "application/json", j.bitir().c_str());
  }

  void ayarlar_(AsyncWebServerRequest *req) {
    Js j;
    // WiFi: kayıt yoksa derlemedeki ağı göster
    if (a_ozel_wifi) {
      j.str("ssid", A.ssid);
    } else {
      auto ap = wifi::global_wifi_component->get_sta();
      j.str("ssid", ap.get_ssid().c_str());
    }
    j.boo("sifre_var", a_ozel_wifi ? A.sifre[0] != 0 : true);
    j.boo("ozel_wifi", a_ozel_wifi);
    j.boo("dhcp", a_ozel_wifi ? A.dhcp != 0 : false);
    char b[16];
    uint8_t ip[4], gw[4], mk[4], dn[4];
    if (a_ozel_wifi) {
      memcpy(ip, A.ip, 4); memcpy(gw, A.gw, 4); memcpy(mk, A.mask, 4); memcpy(dn, A.dns, 4);
    } else {
      auto mi = wifi::global_wifi_component->get_sta().get_manual_ip();
      memset(ip, 0, 4); memset(gw, 0, 4); memset(mk, 0, 4); memset(dn, 0, 4);
      if (mi.has_value()) {
        auto cp = [](const network::IPAddress &a, uint8_t *o) {
          char t[40]; a.str_to(t); ip_oku_(t, o);
        };
        cp(mi->static_ip, ip); cp(mi->gateway, gw); cp(mi->subnet, mk); cp(mi->dns1, dn);
      }
    }
    ip_yaz_(b, ip); j.str("ip", b);
    ip_yaz_(b, gw); j.str("gw", b);
    ip_yaz_(b, mk); j.str("mask", b);
    ip_yaz_(b, dn); j.str("dns", b);
    j.str("mqtt_host", A.mqtt_host);
    j.num("mqtt_port", A.mqtt_port ? A.mqtt_port : 1883, 0);
    j.str("mqtt_user", A.mqtt_user);
    j.boo("mqtt_sifre_var", A.mqtt_pass[0] != 0);
    j.boo("ozel_mqtt", a_ozel_mqtt);
    j.num("uyku_dk", A.uyku_dk, 0);
    j.num("pil_kesim", A.pil_kesim, 2);
    j.num("kuru_v", A.kuru_v, 3);
    j.num("islak_v", A.islak_v, 3);
    j.num("kal_min_fark", KAL_MIN_FARK, 2);
    j.str("bitki_ad", B.ad);
    j.str("bitki_tur", B.tur);
    req->send(200, "application/json", j.bitir().c_str());
  }

  // Şu an çalışan ağın adı/şifresi/IP'si korunarak belirli bir erişim noktasına geç (deneme korumalı)
  void deco_sec_(AsyncWebServerRequest *req) {
    unsigned b[6];
    std::string t = req->arg("bssid");
    int kanal = atoi(req->arg("kanal").c_str());
    const char *hata = nullptr;
    if (sscanf(t.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) hata = "Geçersiz erişim noktası adresi";
    else if (kanal < 1 || kanal > 14) hata = "Geçersiz kanal";
    else if (!wifi::global_wifi_component->is_connected()) hata = "Önce ev ağına bağlı olmalı";
    if (hata) {
      Js j; j.boo("ok", false); j.str("hata", hata);
      req->send(400, "application/json", j.bitir().c_str());
      return;
    }
    uint8_t yeni[6];
    for (int i = 0; i < 6; i++) yeni[i] = b[i];
    std::array<uint8_t, 6> bs;
    std::copy(yeni, yeni + 6, bs.begin());
    bool dhcp = req->arg("dhcp") == "1";   // teşhis: DHCP'nin verdiği adresi görmek için
    ana_donguye_([bs, kanal, dhcp]() {
      auto ap = wifi::global_wifi_component->get_sta();
      if (ap.get_ssid().size() == 0) return;
      Ayarlar y = A;
      strncpy(y.ssid, ap.get_ssid().c_str(), 32); y.ssid[32] = 0;
      strncpy(y.sifre, ap.get_password().c_str(), 64); y.sifre[64] = 0;
      auto mi = ap.get_manual_ip();
      y.dhcp = (dhcp || !mi.has_value()) ? 1 : 0;
      if (mi.has_value()) {
        auto cp = [](const network::IPAddress &a, uint8_t *o) { char c[40]; a.str_to(c); ip_oku_(c, o); };
        cp(mi->static_ip, y.ip); cp(mi->gateway, y.gw); cp(mi->subnet, y.mask); cp(mi->dns1, y.dns);
      }
      A_yedek = A;
      A_yedek.surum = AYAR_SURUM;
      a_yedek_pref.save(&A_yedek);
      A = y;
      ayar_kaydet();
      std::copy(bs.begin(), bs.end(), H.bssid);
      H.kanal = kanal;
      hedef_kaydet();
      ayar_fast_connect_sil();
      rtc_wifi_deneme = WIFI_DENEME_MAGIC;
      rtc_wifi_geri = 0;
      rtc_ozel_hata = 0;
      rtc_varsayilan_kullan = false;
      rtc_ayar_istek = AYAR_ISTEK_MAGIC;
      ESP_LOGI(ATAG, "Erisim noktasi secildi, deneniyor");
      ayar_yeniden_baslat_(6, 500);
    });
    this->tamam_(req);
  }

  void agler_(AsyncWebServerRequest *req) {
    std::string s = "[";
    bool ilk = true;
    for (auto &r : wifi::global_wifi_component->get_scan_result()) {
      if (r.get_is_hidden()) continue;
      Js j;
      j.str("ssid", r.get_ssid().c_str());
      j.num("rssi", r.get_rssi(), 0);
      j.boo("kilit", r.get_with_auth());
      j.num("kanal", r.get_channel(), 0);
      char bb[18];
      auto &bs = r.get_bssid();
      snprintf(bb, sizeof bb, "%02X:%02X:%02X:%02X:%02X:%02X", bs[0], bs[1], bs[2], bs[3], bs[4], bs[5]);
      j.str("bssid", bb);
      if (!ilk) s += ',';
      ilk = false;
      s += j.bitir();
    }
    s += "]";
    req->send(200, "application/json", s.c_str());
  }

  void ayar_kaydet_(AsyncWebServerRequest *req) {
    Ayarlar y = A;
    Bitki yb = B;
    bool bitki_degisti = false;
    std::string hata;
    bool wifi_degisti = false;
    if (req->hasArg("ssid")) {
      std::string ss = req->arg("ssid");
      if (ss.size() > 32) hata = "Ağ adı en fazla 32 karakter olabilir";
      else if (ss != y.ssid) { wifi_degisti = true; strncpy(y.ssid, ss.c_str(), 32); y.ssid[32] = 0; }
      if (req->hasArg("sifre")) {
        std::string p = req->arg("sifre");
        if (!p.empty()) {
          if (p.size() > 64) hata = "WiFi şifresi en fazla 64 karakter olabilir";
          if (p != y.sifre) wifi_degisti = true;
          strncpy(y.sifre, p.c_str(), 64); y.sifre[64] = 0;
        } else if (wifi_degisti) {
          y.sifre[0] = 0;  // yeni ağ, şifre boş = açık ağ
        }
      }
      uint8_t d = req->arg("dhcp") == "1" ? 1 : 0;
      if (d != y.dhcp) wifi_degisti = true;
      y.dhcp = d;
      if (!d) {
        uint8_t t[4];
        const char *alan[4] = {"ip", "gw", "mask", "dns"};
        uint8_t *hedef[4] = {y.ip, y.gw, y.mask, y.dns};
        for (int i = 0; i < 4; i++) {
          if (!ip_oku_(req->arg(alan[i]), t)) { hata = std::string("Geçersiz adres: ") + alan[i]; break; }
          if (memcmp(t, hedef[i], 4) != 0) wifi_degisti = true;
          memcpy(hedef[i], t, 4);
        }
        if (hata.empty()) {
          uint32_t ip = (y.ip[0] << 24) | (y.ip[1] << 16) | (y.ip[2] << 8) | y.ip[3];
          uint32_t gw = (y.gw[0] << 24) | (y.gw[1] << 16) | (y.gw[2] << 8) | y.gw[3];
          uint32_t mk = (y.mask[0] << 24) | (y.mask[1] << 16) | (y.mask[2] << 8) | y.mask[3];
          bool mask_ok = mk != 0 && ((~mk + 1) & ~mk) == 0;   // 255.255.255.0 gibi ardışık 1'ler
          if (ip == 0 || (ip & ~mk) == 0 || (ip & ~mk) == (~mk)) hata = "Geçersiz IP adresi";
          else if (!mask_ok) hata = "Geçersiz alt ağ maskesi (ör. 255.255.255.0)";
          else if (gw == 0 || (gw & mk) != (ip & mk)) hata = "Ağ geçidi IP ile aynı ağda olmalı (ör. 192.168.1.1)";
          else if (gw == ip) hata = "IP ile ağ geçidi aynı olamaz";
          else if (y.dns[0] == 0) hata = "Geçersiz DNS (ağ geçidini yazabilirsin)";
        }
      }
      if (y.ssid[0] == 0) hata = "Ağ adı boş olamaz";
    }
    if (req->hasArg("mqtt_host")) {
      std::string h = req->arg("mqtt_host");
      if (h.size() > 63) hata = "MQTT sunucu adı çok uzun";
      strncpy(y.mqtt_host, h.c_str(), 63); y.mqtt_host[63] = 0;
      int port = atoi(req->arg("mqtt_port").c_str());
      if (port < 1 || port > 65535) hata = "MQTT portu 1-65535 arasında olmalı";
      y.mqtt_port = port;
      std::string us = req->arg("mqtt_user");
      strncpy(y.mqtt_user, us.c_str(), 32); y.mqtt_user[32] = 0;
      if (req->hasArg("mqtt_pass")) {
        std::string p = req->arg("mqtt_pass");
        if (!p.empty()) { strncpy(y.mqtt_pass, p.c_str(), 64); y.mqtt_pass[64] = 0; }
        if (req->arg("mqtt_pass_sil") == "1") y.mqtt_pass[0] = 0;
      }
    }
    if (req->hasArg("uyku_dk")) {
      int u = atoi(req->arg("uyku_dk").c_str());
      if (u < 1 || u > 1440) hata = "Ölçüm aralığı 1-1440 dakika arasında olmalı";
      y.uyku_dk = u;
    }
    if (req->hasArg("pil_kesim")) {
      float p = atof(req->arg("pil_kesim").c_str());
      if (p < 3.0f || p > 3.9f) hata = "Düşük pil eşiği 3,00-3,90 V arasında olmalı";
      y.pil_kesim = p;
    }
    if (req->hasArg("kuru_v")) {
      float k = atof(req->arg("kuru_v").c_str()), i = atof(req->arg("islak_v").c_str());
      if (!(k > i + KAL_MIN_FARK) || k > 3.3f || i < 0.1f) hata = "Kalibrasyon: kuru değeri ıslaktan en az 0,2 V büyük olmalı";
      y.kuru_v = k; y.islak_v = i;
    }
    if (req->hasArg("bitki_ad")) {
      std::string ad = req->arg("bitki_ad"), tur = req->arg("bitki_tur");
      // kırpma yok (UTF-8 karakteri ortadan bölünmesin): uzunsa reddet
      if (ad.size() > 48) hata = "Bitki adı çok uzun (en fazla ~40 karakter)";
      else if (tur.size() > 64) hata = "Tür adı çok uzun (en fazla 64 karakter)";
      else {
        bitki_degisti = ad != yb.ad || tur != yb.tur;
        memset(yb.ad, 0, sizeof yb.ad); memcpy(yb.ad, ad.data(), ad.size());
        memset(yb.tur, 0, sizeof yb.tur); memcpy(yb.tur, tur.data(), tur.size());
      }
    }
    if (!hata.empty()) {
      Js j; j.boo("ok", false); j.str("hata", hata.c_str());
      req->send(400, "application/json", j.bitir().c_str());
      return;
    }
    ana_donguye_([y, yb, wifi_degisti, bitki_degisti]() {
      if (bitki_degisti) {
        B = yb;
        bitki_kaydet();
        // MQTT'ye hemen yayınla (retained): HA bitki adını beklemeden alır
        if (ts_bitki) ts_bitki->publish_state(B.ad);
        if (ts_tur) ts_tur->publish_state(B.tur);
        ESP_LOGI(ATAG, "Bitki: '%s' (%s)", B.ad, B.tur);
      }
      if (wifi_degisti) {
        A_yedek = A;                      // çalışan (mevcut) ayarlar
        A_yedek.surum = AYAR_SURUM;
        a_yedek_pref.save(&A_yedek);
        rtc_wifi_deneme = WIFI_DENEME_MAGIC;
        rtc_wifi_geri = 0;
        rtc_ozel_hata = 0;
        rtc_varsayilan_kullan = false;
      }
      A = y;
      a_ozel_mqtt = A.mqtt_host[0] != 0;
      ayar_kaydet();
      if (wifi_degisti) { hedef_temizle(); ayar_fast_connect_sil(); }
      ESP_LOGI(ATAG, "Ayarlar kaydedildi (wifi %s)", wifi_degisti ? "degisti" : "ayni");
    });
    Js j; j.boo("ok", true); j.boo("yeniden_baslat", wifi_degisti);
    req->send(200, "application/json", j.bitir().c_str());
  }
};

static AyarArayuz *am_handler = nullptr;
inline void ayar_modu_baslat() {
  if (am_aktif) return;
  am_aktif = true;
  am_cikis = false;
  am_son = millis();
  ayar_istek_temizle();
  auto *base = web_server_base::global_web_server_base;
  if (am_handler == nullptr) {
    am_handler = new AyarArayuz();
    base->add_handler(am_handler);
  }
  base->init();
  // kurulum ağı gerekirse çabuk açılsın
  wifi::global_wifi_component->set_ap_timeout(5000);
  ESP_LOGI(ATAG, ">>> AYAR MODU: http://<cihaz-ip>/ (10 dk)");
}
inline bool ayar_modu_aktif() { return am_aktif; }

}  // namespace toprak
using namespace toprak;
