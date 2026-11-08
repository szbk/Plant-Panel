// =====================================================================
//  Toprak Nem Sensörü — Silindirik Kapsül Kutu
//  LilyGO T-Energy S3 (1x18650 arka yüzde) + Capacitive Soil Sensor v2.0
//
//  Yerleşim (üstten bakış, -Y = ön):
//     [ ESP modülü / ön yüz ]  ->  [ PCB ]  ->  [ 18650 pil ]  ->  [ sensör kafası ]
//  - Kart dik durur: ANTEN UCU YUKARIDA (plastik kapak altında), USB-C/anahtar/tuşlar aşağıda.
//  - Kart iki raya kayarak oturur; şarj için kapağı aç, kartı yukarı çek (kablo boşluğu var).
//  - Sensör pilin arkasında, tabandaki yarıktan toprağa iner. Beyaz çizgi = kutunun dibi.
//  - Kapak: VİDALI (sağ diş, adım 2.5 mm, ~2 tur). Saat yönünde çevirince kapanır.
//    Yan yüzde kavrama olukları var. Diş yanakları 45°: iki parça da desteksiz basılır.
//
//  Baskı: Gövde dik (taban tablada), kapak TERS (düz üst yüz tablada). Destek gerekmez.
//  Tablaya değen kenarlar 45° pahla başlar, sonra yuvarlanır (çıkıntı sorunu olmasın diye).
// =====================================================================

/* [Çıktı] */
part = "assembly"; // [body, cap, assembly, section]

/* [Genel] */
$fn    = 160;
wall   = 2.2;    // gövde duvarı
Ri     = 24.5;   // iç yarıçap: USB-C ve ON/OFF anahtarı kart kenarından taşıyor; kart vida ağzından geçebilsin diye 23.5'ten büyütüldü
R      = Ri + wall;
floor_t = 3.0;   // taban kalınlığı
fillet_bottom = 5;   // gövde alt kenar yarıçapı
fillet_top    = 7;   // kapak üst kenar yarıçapı

/* [T-Energy S3 — DXF ve ürün sayfasından] */
pcb_w   = 29.1;  // PCB genişliği
pcb_l   = 91.3;  // PCB boyu
pcb_t   = 1.6;
// Kullanıcı ölçümleri (2026-11-05): ön yüzde en yüksek parça ON/OFF anahtarı; ESP modülü 3.1;
// anahtar ön yüzünden pil arkasına toplam 24.4 mm.
front_h = 3.4;   // PCB ön yüzündeki en yüksek parça (anahtar ~3.4, ESP modülü 3.1)
cell_d  = 18.6;  // 18650 + yuva (24.4 - 3.4 - 1.6 - 0.8)
board_lift = 2.0;   // kart altı boşluk (USB-C kart kenarından 1.8 mm taşıyor)
board_dx = 0.8;  // kartı +X'e kaydır (USB-C tüneli duvardan uzaklaşsın)
// ON/OFF anahtarı: +X kenarı, ön yüzde, kartın tuşlu alt kenarından ~7.3 mm (merkez, fotoğraftan; anahtar 8.6 boyunda), kenardan 1.9 mm taşar
sw_zc   = 7.3;   sw_len = 8.6;  sw_out = 1.9;  sw_h = 3.4;
// USB-C: YAN GİRİŞ — ağzı -X'e (kartın uzun kenarına) bakıyor, kenardan 1.8 mm taşıyor.
// Ön yüzde, merkezi ön yüzden 3.2 mm yukarıda. Kablo yandan takılır → delik YAN DUVARDA.
usb_zc  = 8.5;   // USB-C merkezi kart alt kenarından: RST tuşunun üst ucundan 5.7 (kullanıcı) + tuş/kenar ~2.8 (foto)
usb_dy  = 3.2;   usb_w = 9.0;  usb_t = 3.3;  usb_out = 1.8;  usb_dep = 7.4;   // w: kart boyunca
plug_w  = 12.0 + 1.4;  plug_t = 6.5 + 0.4;  plug_r = 1.5;   // yan delik (fiş gövdesi + pay)

/* [Toprak sensörü — kullanıcı ölçümü] */
s_w     = 22.6;  // PCB genişliği
s_t     = 1.5;   // PCB kalınlığı
s_head  = 24.8;  // beyaz çizgiden konnektör pinlerine
s_conn  = 7.6;   // konnektör yüksekliği (PCB yüzeyinden)
s_gap   = 3.0;   // pil ile sensör arka yüzü arası (mor R4 kablosu 2.8 mm)
// Taban yarığı (v7): kullanıcı sensör kalınlığını 1.5–1.6 ölçtü. v6'da yarık 2.1 mm (her yüzde 0.3)
// + ağızda 4.5 mm'ye açılan 45° silikon cebi vardı → altta belirgin boşluk görünüyordu.
s_t_max  = 1.6;   // ölçülen en kalın değer
slot_clr = 0.2;   // toplam boşluk (her yüzde 0.1): sıkıştırmaz, oynatmaz. Sıkı gelirse 0.3 yap.
slot_t   = s_t_max + slot_clr;   // 1.8 mm
slot_w   = s_w + 0.4;            // 23.0 mm (v6: 23.2)
// V-çentikler (sensör PCB'nin iki kenarında) — kullanıcı ölçümü
// Çentik YAMUK: kenarda geniş açılır, 45° yanaklarla içeri girer, dipte düz devam eder.
n_z     = 9.6;   // beyaz çizgiden çentik merkezine
n_open  = 5.3;   // çentiğin kenardaki açıklığı — test tarağında 3 numara (5.3) tam oturdu
n_flat  = 2.7;   // çentik dibindeki düz kısım (kullanıcı ölçümü ~2.7)
n_dep   = (n_open - n_flat) / 2;   // 45° yanak → derinlik
n_h     = n_open;

/* [Sensör kelepçesi (2 parça)] */
// Sensör kutu DIŞINDA kelepçeye takılır: iki yarım, V-çentiklere giren üçgen dişlerle sensörü
// sarar, 1.75 mm filament pimlerle birbirine geçer. Sonra kelepçe+sensör yukarıdan kutuya
// indirilir ve iki M3x12 vida ile (üstten, tornavidayla) tabandaki ayaklara vidalanır.
cl_top   = 15;     // kelepçe üstü (z, beyaz çizgi = 0)
cl_side  = 16.5;   // yan duvar dış yüzü |x| (ön yarımda silindirle kırpılır)
cl_gap   = 0.1;    // sensör kenarı ile yan duvar arası (az olsun: x'te kayma = dikeyde boşluk)
cl_rel   = 3.0;    // orta bölgede sensör yüzünden boşluk (bileşenler / R4 kablosu)
cl_plate = 2.0;    // boşluğun arkasındaki plaka
cl_strip = 3.0;    // sensör yüzüne temas eden kenar şeridi genişliği (x)
tooth_c  = 0.05;   // diş ile çentik yanakları arası pay (sıkı otursun)
pin_d    = 1.95;   // 1.75 filament pim deliği
pin_x    = 13.9;   // pim ekseni |x| (yan duvarın ortası)
pin_z    = [9.9, 13.0];   // pim yükseklikleri (altta vida ayağı boşluğu var)
pin_dep_f = 2.5;   // ön yarımda delik derinliği (kırpılmış köşeyi delmesin)
pin_dep_b = 2.5;   // arka yarımda delik derinliği (vida deliğine değmesin)
pin_len  = 4.6;    // filament pim boyu (1.75 mm filamentten ~4.5 mm kes)
ear_x    = 15.4;   // vida ekseni |x|
ear_r    = 3.6;    // kulak yarıçapı
ped_h    = 5;      // tabandaki vida ayağı yüksekliği (z = floor_t .. floor_t+ped_h)
scr_pilot = 2.6;   // ayaktaki pilot delik (M3 plastiğe diş açar; PLA için 2.7)
scr_clr   = 3.4;   // kulaktaki geçiş deliği

/* [Kapak — vidalı] */
cap_h   = 13;    // kapağın gövde üstünden yüksekliği
cap_top = 2.4;   // kapak tavan kalınlığı
lip_d   = 7;     // kapak boynunun gövdeye girme derinliği
lip_t   = 1.6;
th_pitch = 2.5;  // diş adımı (sağ diş: saat yönünde kapanır)
th_depth = 0.9;  // diş yüksekliği
th_tip   = 0.4;  // diş tepe genişliği (yanaklar 45° → desteksiz basılır)
th_rclr  = 0.3;  // radyal boşluk
grip_n   = 36;   // kavrama oluğu sayısı
// Kartı aşağı bastıran halka (kapak tavanından iner). Yarıçap 10.06–11.66 arasında olmalı:
// böylece halka PCB'nin üst kenarına ortaya yakın iki noktadan değer, ESP modülünün
// (PCB önünde, y<-11.66) üstünden geçmez. Kapak hangi açıda kapanırsa kapansın çalışır.
press_r   = 11.0;
press_t   = 1.6;
press_gap = 0.2;  // kapak tam kapandığında halka ile kart arası (sıkı olsun istersen 0 / -0.2)
grip_d   = 0.6;  // oluk derinliği

// ---------------------------------------------------------------------
// Kesit yerleşimi (Y ekseni). Değerler, silindir eksenine göre.
// Zarf hesabı: ön yüz y=0 ... konnektör y=37 → en küçük daire merkezi y=15.16
c_off     = 15.16;
y_front   = 0 - c_off;                       // modül ön yüzü
y_pcb     = front_h - c_off;                 // PCB ön yüzü
y_cell    = front_h + pcb_t + 0.8 + cell_d/2 - c_off;   // pil ekseni
y_sens    = front_h + pcb_t + 0.8 + cell_d + s_gap - c_off; // sensör PCB arka yüzü

z_board0  = floor_t + board_lift;
z_board1  = z_board0 + pcb_l;
body_h    = z_board1 + 1.2;                  // kart üstü + pay
echo(str("Dış ölçü: Ø", 2*R, " x ", body_h + cap_h, " mm  (gövde ", body_h, ")"));

// --- yardımcı: tablaya bakan kenar için 45° pah + yay -----------------
function arc_pts(cx, cz, r, a0, a1, n=24) =
    [for (i=[0:n]) let(a = a0 + (a1-a0)*i/n) [cx + r*cos(a), cz + r*sin(a)]];

// gövde duvar kesiti (r,z)
module body_profile() {
    rf = fillet_bottom;
    p45 = [R - rf + rf*cos(-45), rf + rf*sin(-45)];
    pts = concat(
        [[0, 0], [p45[0] - p45[1], 0]],
        arc_pts(R - rf, rf, rf, -45, 0),
        [[R, body_h], [Ri, body_h], [Ri, floor_t], [0, floor_t]]
    );
    polygon(pts);
}

// --- vida dişi (helis, 45° yanaklı trapez kesit) ---------------------
// r0: diş tabanı yarıçapı, dir=-1 içe (gövde iç dişi), dir=+1 dışa (kapak dış dişi)
module thread(r0, z0, length, dir) {
    b = th_tip + 2*th_depth;      // taban genişliği
    e = 0.3;                      // duvara gömme
    prof = [[r0 - dir*e, -b/2], [r0 + dir*th_depth, -th_tip/2],
            [r0 + dir*th_depth,  th_tip/2], [r0 - dir*e,  b/2]];
    n_per = 96;
    turns = (length - b) / th_pitch;
    N = max(2, ceil(turns * n_per));
    pts = [for (i=[0:N]) let(a = 360*turns*i/N, zz = z0 + b/2 + th_pitch*turns*i/N)
             for (q = prof) [q[0]*cos(a), q[0]*sin(a), zz + q[1]]];
    // tüm yüzler üçgen (CGAL düzlemsel olmayan dörtgenleri kabul etmiyor)
    faces = concat(
        [[0,1,2], [0,2,3]],
        [[4*N+3, 4*N+2, 4*N+1], [4*N+3, 4*N+1, 4*N]],
        [for (i=[0:N-1]) for (k=[0:3]) let(k2=(k+1)%4) each
            [[4*i+k, 4*(i+1)+k, 4*(i+1)+k2], [4*i+k, 4*(i+1)+k2, 4*i+k2]]]
    );
    polyhedron(pts, faces, convexity=10);
}
r_lip_o = Ri - th_depth - th_rclr;   // kapak boynu dış yarıçapı

// kapak kesiti (r,z), z=0 = gövde üstü
module cap_profile() {
    rf = fillet_top;
    p45 = [R - rf + rf*cos(45), cap_h - rf + rf*sin(45)];
    outer = concat(
        [[R, 0]],
        arc_pts(R - rf, cap_h - rf, rf, 0, 45),
        [[p45[0] - (cap_h - p45[1]), cap_h], [0, cap_h]]
    );
    // iç boşluk tavanı
    pts = concat(outer, [[0, cap_h - cap_top], [R - wall, cap_h - cap_top], [R - wall, 0]]);
    polygon(pts);
    // boyun (gövdenin içine girer, dış dişli) + kapak duvarına bağlayan flanş
    translate([r_lip_o - lip_t, -lip_d]) square([lip_t, lip_d + 1.8]);
    translate([r_lip_o - lip_t, 0]) square([R - (r_lip_o - lip_t) - 0.01, 1.8]);
}

// --- gövde ------------------------------------------------------------
module body() {
    difference() {
        union() {
            rotate_extrude() body_profile();
            intersection() {
                cylinder(r = Ri + 0.5, h = body_h);
                union() { translate([board_dx, 0, 0]) { rails(); stops(); front_pad(); } pedestals(); }
            }
            // iç diş (gövde ağzında)
            thread(Ri, body_h - lip_d - 0.6, lip_d, -1);
        }
        pedestal_holes();
        // sensör yarığı (tabandan), sensör kalınlığının ortasına göre; silikon cebi YOK (v7)
        translate([-slot_w/2, y_sens + s_t/2 - slot_t/2, -1]) cube([slot_w, slot_t, floor_t + 2]);
    }
}

// kart rayları: PCB kenarlarını tutan yarıklı dikmeler
module rails() {
    // Kart üstten sürülerek takılır; USB-C (-X) ve ON/OFF anahtarı (+X) ön yüzde ve kart
    // kenarından taşıyor → ön dudak kartın tüm boyunca OLAMAZ. Raylar: arka dudak + yan duvar
    // (kart arkadan ve yandan tutulur); ön dudak sadece +X'te anahtarın altında kalan bölgede.
    slot_t = pcb_t + 0.4;
    for (s = [-1, 1]) {
        z1 = z_board0 + 65;                          // üstteki G/IO1/IO2 kablolarına değmesin
        xi = pcb_w/2 - 1.4;                          // rayın iç kenarı |x|
        xo = Ri + 1;
        mirror([s > 0 ? 0 : 1, 0, 0]) difference() {
            union() {
                translate([xi, y_pcb + 0.05, floor_t - 0.01]) cube([xo - xi, slot_t + 2.3, z1 - floor_t]);
                // ön dudak (v7: İKİ tarafta): tabandan kartın alt kenarının 2.2 mm üstüne kadar.
                // +X'te anahtarın, -X'te USB-C'nin altında kalır (USB-C altı ~z_board0+4, ±1 belirsiz).
                translate([xi, y_pcb - 2.5, floor_t - 0.01])
                    cube([xo - xi, 2.6, z_board0 + lip_top - floor_t]);
            }
            // PCB yarığı
            translate([pcb_w/2 - 1.5, y_pcb - 0.2, z_board0 - 1]) cube([2.3, slot_t, z1 - z_board0 + 2]);
        }
    }
}

// Üst ön dayanak (v7): kartın öne devrilmesini önler. Kart sadece arkadan (ray dudakları) ve
// alttan (ön dudaklar) tutuluyordu; üst kısım öne serbestti. Duvardan çıkan bir kaburga ESP
// modülünün ön yüzüne pad_clr boşlukla dayanır. Kart üstten sürülürken modül bu yüzeye sürtünerek
// iner; USB-C (x<-8) ve anahtar (x>12.5) bu x aralığından geçmez → takmayı engellemez.
// Alt yüzü 45° (desteksiz baskı). Kapak boynuna (r>21.4, z>body_h-8) değmemesi için üstü kısa.
lip_top = 2.2;                   // ön dudak üstü, kart alt kenarından
mod_w0 = -9; mod_w1 = 9;         // ESP modülü x aralığı (kart koordinatı, DXF/foto tahmini)
mod_h  = 26;                     // modül boyu (kart üst kenarından aşağı)
pad_x0 = -6; pad_x1 = 7;         // dayanak genişliği (modülün içinde kalsın)
pad_clr = 0.2;                   // modül ön yüzü ile dayanak arası
pad_z1 = body_h - lip_d - 2.5;   // dayanak üstü (kapak boynunun altında)
pad_z0 = z_board1 - mod_h + 2;   // dayanak altı (modül alt kenarının 2 mm üstü)
module front_pad() {
    yb = y_pcb - 3.1 - pad_clr;          // dayanak arka yüzü (modüle bakan)
    d  = yb - (-Ri - 1);                 // duvara kadar derinlik
    intersection() {
        cylinder(r = Ri + 0.5, h = 200);
        hull() {
            translate([pad_x0, -Ri - 1, pad_z0]) cube([pad_x1 - pad_x0, d, pad_z1 - pad_z0]);
            translate([pad_x0, -Ri - 1, pad_z0 - d]) cube([pad_x1 - pad_x0, 0.01, 0.01]);   // 45° alt yüz
        }
    }
}

// kartın oturduğu dipler (kenarlarda, anahtarın altında)
module stops() {
    for (s = [-1, 1])
        translate([s > 0 ? pcb_w/2 - 1.6 : -pcb_w/2 - 0.4, y_pcb - 0.6, floor_t - 0.01])
            cube([2.0, pcb_t + 1.2, board_lift]);
}

// --- USB-C şarj deliği (yan duvarda, -X) ------------------------------
usb_y  = y_pcb - usb_dy;                       // ağız merkezi y
usb_z  = z_board0 + usb_zc;                    // ağız merkezi z
usb_mx = board_dx - pcb_w/2 - usb_out;         // ağzın x'i (kart kenarı - taşma)
// y-z kesitinde yuvarlatılmış dikdörtgen, -X yönünde uzar
module yz_rrect_prism(w, h, r, x0, L) {   // w: z boyu, h: y boyu
    hull() for (sy = [-1, 1], sz = [-1, 1])
        translate([x0, usb_y + sy*(h/2 - r), usb_z + sz*(w/2 - r)]) rotate([0, -90, 0]) cylinder(r = r, h = L, $fn = 24);
}
module usb_hole() {      // fiş gövdesi duvardan geçip USB-C ağzına kadar ulaşır
    yz_rrect_prism(plug_w, plug_t, plug_r, usb_mx - 0.3, 20);
}
module usb_tube() {      // delikten içeri uzanan kılavuz boru (fişi düz yönlendirir)
    intersection() {
        yz_rrect_prism(plug_w + 2.4, plug_t + 2.4, plug_r + 1.2, usb_mx - 0.3, 20);
        cylinder(r = Ri + 0.5, h = 200);
    }
}

// --- sensör kelepçesi -------------------------------------------------
cl_y   = y_sens + s_t/2;              // ayrım düzlemi (sensör kalınlığının ortası)
cl_yb  = y_sens - cl_rel - cl_plate;  // arka yarım arka yüzü
cl_yf  = y_sens + s_t + cl_rel + cl_plate;   // ön yarım ön yüzü (silindirle kırpılır)
ear_y  = y_sens - 4.0;                // vida ekseni y (pim deliklerinden uzak)
ear_z0 = floor_t + ped_h;             // kulak altı = ayak üstü
inner_x = s_w/2 - cl_strip;           // boşluk bölgesinin |x| sınırı

// V-diş (x-z kesiti), y yönünde uzatılır
module tooth2d() {
    // yamuk diş: çentiğin aynısı, yanaklardan tooth_c içeride; ucu dibe değmesin diye 0.15 kısa
    o = n_open/2 - tooth_c;          // kenar hizasında yarım genişlik
    d = n_dep - 0.15;                // diş derinliği
    polygon([[s_w/2 + cl_gap + 0.5, n_z - o],
             [s_w/2, n_z - o],
             [s_w/2 - d, n_z - (o - d)],
             [s_w/2 - d, n_z + (o - d)],
             [s_w/2, n_z + o],
             [s_w/2 + cl_gap + 0.5, n_z + o]]);
}
module teeth(y0, y1) {   // iki kenar
    for (m = [0, 1]) mirror([m, 0, 0])
        translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) tooth2d();
}
module pin_holes(back) {
    for (sx = [-1, 1]) for (z = pin_z)
        translate([sx*pin_x, back ? cl_y - pin_dep_b : cl_y - 0.01, z]) rotate([-90, 0, 0])
            cylinder(d = pin_d, h = (back ? pin_dep_b : pin_dep_f) + 0.01, $fn = 24);
}
module pins() {   // gösterim
    for (sx = [-1, 1]) for (z = pin_z) color("white")
        translate([sx*pin_x, cl_y - pin_dep_b + 0.2, z]) rotate([-90, 0, 0]) cylinder(d = 1.75, h = pin_len, $fn = 16);
}
// yarım: y0..y1 arası blok, sensör boşluğu, orta rahatlama cebi, dişler
module clamp_half(back) {
    y0 = back ? cl_yb : cl_y;
    y1 = back ? cl_y  : cl_yf;
    sy0 = back ? y_sens : cl_y;              // sensörün bu yarımdaki kısmı
    sy1 = back ? cl_y   : y_sens + s_t;
    ry0 = back ? y_sens - cl_rel : y_sens + s_t - 0.01;   // rahatlama cebi
    ry1 = back ? y_sens + 0.01  : y_sens + s_t + cl_rel;
    intersection() {
        cylinder(r = Ri - 0.4, h = 200);
        difference() {
            union() {
                translate([-cl_side, y0, floor_t]) cube([2*cl_side, y1 - y0, cl_top - floor_t]);
                if (back) for (sx = [-1, 1])   // vida kulakları (ayrım düzlemini geçmez)
                    intersection() {
                        hull() {
                            translate([sx*ear_x, ear_y, ear_z0]) cylinder(r = ear_r, h = cl_top - ear_z0);
                            translate([sx*(cl_side - 1), ear_y, ear_z0]) cylinder(r = ear_r, h = cl_top - ear_z0);
                        }
                        translate([-50, -50, 0]) cube([100, 50 + cl_y, 100]);
                    }
            }
            // sensör (tam genişlik + yan pay)
            translate([-(s_w/2 + cl_gap), sy0 - 0.01, floor_t - 1]) cube([s_w + 2*cl_gap, sy1 - sy0 + 0.02, 50]);
            // orta rahatlama cebi (sadece kenar şeritleri sensöre değer)
            translate([-inner_x, ry0, floor_t - 1]) cube([2*inner_x, ry1 - ry0, 50]);
            pin_holes(back);
            for (sx = [-1, 1]) translate([sx*ear_x, ear_y, 0]) {
                if (back) translate([0, 0, ear_z0 - 1]) cylinder(d = scr_clr, h = 50, $fn = 32);
                cylinder(r = ear_r + 0.25, h = ear_z0 + 0.15);   // ayak boşluğu
            }
        }
    }
    teeth(sy0, sy1);
}
module clamp_back()  clamp_half(true);
module clamp_front() clamp_half(false);

// gövde tabanındaki vida ayakları
module pedestals() {
    for (sx = [-1, 1]) translate([sx*ear_x, ear_y, floor_t - 0.01]) cylinder(r = ear_r, h = ped_h + 0.01);
}
module pedestal_holes() {
    for (sx = [-1, 1]) translate([sx*ear_x, ear_y, 1.5]) cylinder(d = scr_pilot, h = 50, $fn = 32);
}
module clamp_screws() {   // M3x12 gösterim
    for (sx = [-1, 1]) color("gold") translate([sx*ear_x, ear_y, 0]) {
        translate([0, 0, cl_top - 12]) cylinder(d = 2.9, h = 12, $fn = 24);
        translate([0, 0, cl_top]) cylinder(d = 5.5, h = 2, $fn = 24);
    }
}

// --- kapak ------------------------------------------------------------
module cap() {
    difference() {
        union() {
            rotate_extrude() cap_profile();
            // dış diş (boyunda), ağızdan 0.6 mm içeriden başlar
            thread(r_lip_o, -lip_d + 0.3, lip_d - 0.6, +1);
            // kart baskı halkası: tavandan kartın üst kenarına
            press_z0 = z_board1 + press_gap - body_h;      // kapak koordinatında
            translate([0, 0, press_z0])
                difference() {
                    cylinder(r = press_r + press_t/2, h = cap_h - cap_top - press_z0 + 0.01);
                    translate([0, 0, -1]) cylinder(r = press_r - press_t/2, h = cap_h + 20);
                }
        }
        // kavrama olukları (dikey yüzeyde, döndürmek için)
        for (i = [0:grip_n-1]) rotate([0, 0, i*360/grip_n])
            translate([R - grip_d, -0.75, 0.6]) cube([grip_d + 1, 1.5, cap_h - fillet_top*0.55]);
    }
}

// --- önizleme yer tutucuları ------------------------------------------
module board_dummies() {
    translate([board_dx, 0, 0]) {
        color("#1a1a1a") translate([-pcb_w/2, y_pcb, z_board0]) cube([pcb_w, pcb_t, pcb_l]);
        color("silver")  translate([-9, y_pcb - 3.1, z_board1 - 26]) cube([18, 3.1, 25.5]);   // ESP modülü
        color("#666")    translate([pcb_w/2 - 2.0, y_pcb - sw_h, z_board0 + sw_zc - sw_len/2])
                             cube([2.0 + sw_out, sw_h, sw_len]);                                 // ON/OFF
        color("silver")  translate([-pcb_w/2 - usb_out, y_pcb - usb_dy - usb_t/2, z_board0 + usb_zc - usb_w/2])
                             cube([usb_dep, usb_t, usb_w]);                                      // USB-C (yan)
        // pil
        color("lightblue") translate([0, y_cell, z_board0 + 13]) cylinder(d = 18.4, h = 65);
    }
}
module sensor_dummy(len_below = 70) {   // beyaz çizgi z=0
    difference() {
        translate([-s_w/2, y_sens, -len_below]) cube([s_w, s_t, len_below + s_head]);
        for (m = [0, 1]) mirror([m, 0, 0])   // V-çentikler
            translate([0, y_sens + s_t + 1, 0]) rotate([90, 0, 0]) linear_extrude(s_t + 2)
                polygon([[s_w/2 + 0.01, n_z - n_open/2], [s_w/2 - n_dep, n_z - n_flat/2],
                         [s_w/2 - n_dep, n_z + n_flat/2], [s_w/2 + 0.01, n_z + n_open/2]]);
    }
    color("white") translate([-5, y_sens + s_t, s_head - 2]) cube([10, s_conn, 8]);
    for (i = [-1, 0, 1]) color(["black","red","gold"][i+1])
        translate([i*2.5, y_sens + s_t + 3, s_head + 6]) cylinder(d = 1.4, h = 18);
}
module dummies() { board_dummies(); color("#222") sensor_dummy(); }

if (part == "hcut") intersection(){ union(){body(); dummies();} translate([-50,-50,0]) cube([100,100,40]); }
else if (part == "body") body();
else if (part == "usb_test") intersection() { body(); translate([-50, -50, 0]) cube([100, 100, 26]); }   // alt 26 mm prova halkası
else if (part == "top_test") intersection() { body(); translate([-50, -50, pad_z0 - 25]) cube([100, 100, 200]); }   // üst prova halkası (ön dayanak + diş)
else if (part == "padcheck") intersection() { body(); translate([board_dx, 0, 0]) translate([-9, y_pcb - 3.1, z_board1 - 26]) cube([18, 3.1, 25.5]); }   // boş çıkmalı
else if (part == "cap") translate([0, 0, cap_h]) rotate([180, 0, 0]) cap();   // baskı yönü
else if (part == "sectop") difference() {
    union() { body(); translate([0, 0, body_h]) cap(); dummies(); }
    translate([0, -100, -100]) cube(200);
    translate([-100,-100,-100]) cube([200,200,170]);
}
else if (part == "section") difference() {
    union() { body(); translate([0, 0, body_h]) cap(); dummies(); }
    translate([0, -100, -100]) cube(200);
}
else if (part == "clamp_back")  rotate([-90, 0, 0]) translate([0, -cl_y, 0]) clamp_back();    // baskı: ayrım yüzü tablada
else if (part == "clamp_front") rotate([90, 0, 0]) translate([0, -cl_y, 0]) clamp_front();    // baskı: ayrım yüzü tablada
else if (part == "explode") {
    body(); translate([0,0,body_h+35]) cap(); dummies();
    color("orange") translate([0, 0, 60]) translate([0, -8, 0]) clamp_back();
    color("tomato") translate([0, 0, 60]) translate([0, 8, 0]) clamp_front();
}
else if (part == "clamp_explode") {   // sensör + kelepçe, kutu dışında, patlatılmış
    color("#222") sensor_dummy(20);
    color("orange") translate([0, -14, 0]) clamp_back();
    color("tomato") translate([0, 14, 0]) clamp_front();
    pins();
}
else if (part == "clamp_on") { pins(); color("#222") sensor_dummy(20); color("orange") clamp_back(); color("tomato") clamp_front(); }
else if (part == "install") {   // kelepçeli sensör kutuya iniyor + vidalar
    difference() { body(); translate([-100, 0, -1]) cube([200, 100, 200]); translate([0,0,0]) translate([-100,-100,40]) cube([200,200,200]); }
    translate([0, 0, 45]) { color("#222") sensor_dummy(30); color("orange") clamp_back(); color("tomato") clamp_front(); }
    translate([0, 0, 70]) clamp_screws();
}
else if (part == "installed") intersection() {
    union() { body(); color("#222") sensor_dummy(); color("orange") clamp_back(); color("tomato") clamp_front(); clamp_screws(); }
    translate([-30, -30, -5]) cube([60, 60, 30]);
}
else if (part == "clash") {   // boş çıkmalı
    intersection() { union() { clamp_back(); clamp_front(); } body(); }
}
else if (part == "pincheck") {   // boş çıkmalı: pim deliğinin 0.5 mm çevresinde boşluk olmamalı
    for (back = [true, false]) difference() {
        intersection() {
            for (sx = [-1, 1]) for (z = pin_z)
                translate([sx*pin_x, back ? cl_y - pin_dep_b - 0.5 : cl_y + 0.01, z]) rotate([-90, 0, 0])
                    cylinder(d = pin_d + 1.0, h = (back ? pin_dep_b + 0.5 : pin_dep_f + 0.5) - 0.01, $fn = 24);
        }
        clamp_half(back);
        pin_holes(back);
    }
}
else if (part == "railview") difference() { body(); translate([-100, -100, 40]) cube(200); translate([-100, -3, -1]) cube(200); }
else if (part == "railcheck") intersection() { body(); board_dummies(); }
else if (part == "insertcheck") intersection() {   // kart üstten sürülürken bir şeye takılıyor mu
    body();
    translate([board_dx, 0, 0]) for (i = [0:4]) hull() for (dz = [0, 120]) translate([0, 0, dz]) {
        if (i == 0) translate([-pcb_w/2, y_pcb, z_board0]) cube([pcb_w, pcb_t, pcb_l]);
        if (i == 1) translate([-9, y_pcb - 3.1, z_board1 - 26]) cube([18, 3.1, 25.5]);
        if (i == 2) translate([pcb_w/2 - 2.0, y_pcb - sw_h, z_board0 + sw_zc - sw_len/2]) cube([2.0 + sw_out, sw_h, sw_len]);
        if (i == 3) translate([-pcb_w/2 - usb_out, y_pcb - usb_dy - usb_t/2, z_board0 + usb_zc - usb_w/2]) cube([usb_dep, usb_t, usb_w]);
        if (i == 4) translate([0, y_cell, z_board0 + 13]) cylinder(d = 18.4, h = 65);
    }
}
else if (part == "usbview") intersection() {   // USB ekseninden yatay kesit
    union() { body(); board_dummies(); }
    translate([-40, -40, -1]) cube([80, 80, usb_z + 1]);
}
else if (part == "railcheck") intersection() { body(); board_dummies(); }
else if (part == "insertcheck") intersection() {   // kart üstten sürülürken bir şeye takılıyor mu
    body();
    translate([board_dx, 0, 0]) for (i = [0:4]) hull() for (dz = [0, 120]) translate([0, 0, dz]) {
        if (i == 0) translate([-pcb_w/2, y_pcb, z_board0]) cube([pcb_w, pcb_t, pcb_l]);
        if (i == 1) translate([-9, y_pcb - 3.1, z_board1 - 26]) cube([18, 3.1, 25.5]);
        if (i == 2) translate([pcb_w/2 - 2.0, y_pcb - sw_h, z_board0 + sw_zc - sw_len/2]) cube([2.0 + sw_out, sw_h, sw_len]);
        if (i == 3) translate([-pcb_w/2 - usb_out, y_pcb - usb_dy - usb_t/2, z_board0 + usb_zc - usb_w/2]) cube([usb_dep, usb_t, usb_w]);
        if (i == 4) translate([0, y_cell, z_board0 + 13]) cylinder(d = 18.4, h = 65);
    }
}
else if (part == "usbview") difference() {   // USB tüneli kesiti
    union() { body(); board_dummies(); }
    translate([-100, usb_y, -10]) cube([200, 100, 200]);   // kesit: USB ekseninden
    translate([-100, -100, 30]) cube([200, 200, 200]);
}
else if (part == "notch_test") {   // çentik test tarağı: 4 yamuk diş, açıklık 4.7 / 5.0 / 5.3 / 5.6
    // Her dişi sensörün çentiğine kenardan bastır. Dişin iki yanındaki omuzlar sensör kenarına
    // değiyor VE diş oynamıyorsa doğru boy odur. Diş numarası = altındaki delik sayısı.
    opens = [4.7, 5.0, 5.3, 5.6];
    difference() {
        union() {
            cube([46, 9, 1.6]);
            for (i = [0:3]) let(o = opens[i], d = (o - n_flat)/2, x0 = 6 + i*11.3)
                translate([x0, 9, 0]) linear_extrude(1.6)
                    polygon([[-o/2, 0], [-n_flat/2, d], [n_flat/2, d], [o/2, 0]]);
        }
        for (i = [0:3]) for (k = [0:i])
            translate([6 + i*11.3 - i*0.9 + k*1.8, 4, -1]) cylinder(d = 1.2, h = 5, $fn = 16);
    }
}
else if (part == "toothview") {   // y = sensör içi kesit, önden: diş çentikte
    yc = y_sens + 0.4;
    color("#333") linear_extrude(0.1) intersection() {
        projection(cut = true) rotate([-90, 0, 0]) translate([0, -yc, 0]) sensor_dummy(5);
        translate([s_w/2 - 6, n_z - 6]) square([10, 12]); }
    color("orange") translate([0, 0, 0.2]) linear_extrude(0.1) intersection() {
        projection(cut = true) rotate([-90, 0, 0]) translate([0, -yc, 0]) clamp_back();
        translate([s_w/2 - 6, n_z - 6]) square([10, 12]); }
}
else if (part == "clash_s") intersection() { clamp_back(); clamp_front(); }
else if (part == "clash_d") intersection() { union() { clamp_back(); clamp_front(); } board_dummies(); }
else if (part == "clash_sens") intersection() { union() { clamp_back(); clamp_front(); } sensor_dummy(); }
else { body(); translate([0, 0, body_h]) cap(); }
