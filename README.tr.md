<p align="center"><img src="docs/noqopen.svg" alt="NOQ Open" height="40"></p>

# NOQ SOS

Tek düğmeli çağrı kolyesi. Basınca aileye Telegram'dan mesaj gider; mesajı kolye kendisi gönderir, arada bizim sunucumuz, uygulamamız ya da hesabımız yok. Seeed Studio XIAO ESP32S3, bir düğme ve küçük bir LiPo pil.

**Proje sayfası ve tarayıcıdan kurulum:** https://open.noq.works/tr/projeler/sos/ · [English](README.md)

> **Deneme sürümü (0.3.0).** Masada çalışıyor: basış, Wi-Fi, Telegram, komutlar. 0.3.0'da yeni: basılı kutu, pil defteri, /uyan ve keşif modu. Karşılaştığın sorunu issues'a yaz.

## Ne yapar

- **Tek basış, tek mesaj.** Kart uyur; basınca uyanır, ev Wi-Fi'ına bağlanır ve listedeki her sohbete basış saatiyle "🆘 *ad* yardım istiyor!" gönderir.
- **Arada kimse yok.** Aile kendi Telegram botunu kurar; anahtar kolyede durur, kolye Telegram'la kendisi konuşur.
- **Pes etmez.** Gönderilemeyen bildirim (Wi-Fi ya da internet yok) saklanır; 1, 2, 5 dakika sonra, sonra 10 dakikada bir yeniden denenir, mesaj ne kadar geciktiğini yazar.
- **Yaşadığını söyler.** İsteğe bağlı, seçilen saatte sessiz günlük yoklama; pil ölçümü takılıysa pil azaldı mesajı.
- **Kendi sayfaları.** İlk açılışta `noq-sos-xxxxxx` ağı NOQ Open Wi-Fi kurulum sayfasıyla açılır; ev ağında kolye `http://noq-sos-xxxxxx.local` adresinde kendi sayfasını sunar: ad, bot, alıcılar, deneme mesajı.
- **Katılmak bir dokunuş.** Kurulum açıkken botta *Başlat*'a dokunan (ya da botu bir gruba ekleyen) listeye girer ve "Bağlandı" yanıtı alır. Kurulum dışında listeye kimse eklenmez.
- **Telegram'dan sor.** `/durum`, `/pil`, `/yardim` ya da İngilizce `/status`, `/battery`, `/help`; özel sohbette ya da aile grubunda (orada `/durum@bot`, Telegram'ın `/` menüsü kendisi yazar). Cevap komuta yanıt olarak, komutun dilinde gelir; yalnız listedeki sohbetler cevap alır. Kolye uyanıksa hemen, uyuyorsa saatte bir uyanıp cevaplar (15 ya da 30 dakika, 3 saat ya da yalnız basınca ve yoklamada: sayfasında *Telegram komutları*).

## Depo

| Yol | İçerik |
|---|---|
| `firmware/noq-sos.yaml` | ESPHome yapılandırması (pinler, Wi-Fi, kurulum) |
| `components/noq_sos/` | ESPHome dış bileşeni: uyku ve uyanma, Telegram, yeniden deneme, yoklama, kurulum sayfası ve kolye sayfası ile API'leri |
| `portal/` | Sayfa kaynakları: `tokens.css` → `components.css` → kurulum sayfası `index.html` + `app.js`, kolye sayfası `panel.html` + `panel.css` + `panel.js`; `build.py` bunları tek dosyaya gömüp sıkıştırarak `components/noq_sos/portal_page.h` ve `panel_page.h` yazar |
| `releases.json` | Sürümler ve sürüm notları; `CHANGELOG.md` / `CHANGELOG.tr.md` bundan `tools/changelog.py` ile üretilir |
| `docs/wiring.svg` | Bağlantı şeması |

## Yapım

`docs/wiring.svg`'deki gibi bağla:

1. **Düğme:** bir bacak D1'e, çapraz bacak GND'ye. Direnç yok; içteki pull-up kullanılır.
2. **Pil:** tek hücreli LiPo, arka yüzdeki BAT+ / BAT− pedlerine (kırmızı BAT+'ya). USB-C ile şarj olur.
3. **Pil ölçümü (isteğe bağlı):** BAT+ → 1 MΩ → D0 → 1 MΩ → GND, D0 ile GND arasına 100 nF.
4. **Bip ya da titreşim (isteğe bağlı):** D3 → +, − → GND; motor bir transistörle.

XIAO'nun kendi LED'i (GPIO21) ne olduğunu gösterir: basınca bir kez, mesaj gidince üç kez yanıp söner, gidemediyse bir kez uzun yanar.

## Yazılım

**Tarayıcı:** masaüstünde Chrome ya da Edge ile https://open.noq.works/tr/kurulum/sos/ adresini aç. Sihirbaz kartı USB'den yükler, Wi-Fi'a bağlar ve kolyenin sayfasını açar.

**ESPHome:**

```bash
esphome run firmware/noq-sos.yaml
```

Pinleri dosyanın başındaki `substitutions` ile değiştir. Sonraki güncellemeler kolye uyanıkken Wi-Fi üzerinden gider.

**Sayfalar:** `portal/` içindeki dosyaları düzenle, `python3 portal/build.py` çalıştır, yazılımı yeniden derle.

### Uyanık ve uykuda

- Güç verildikten, sıfırlandıktan ya da yüklendikten sonra 10 dakika; sayfası açıkken; bilgisayara takılıyken uyanık.
- Geri kalan zaman uykuda. Basış, bekleyen bir yeniden deneme, günlük yoklama ya da komut bakışı (varsayılan saatte bir) onu uyandırır.
- Sonradan bir şey değiştirmek için: Telegram'dan /uyan yaz (kolye bir sonraki bakışında 10 dakika uyanık kalır) ya da USB-C'den bilgisayara tak ve sayfasını aç. Kart henüz sarılmadıysa RESET düğmesi de olur.
- Ağı değiştirmek: kolye sayfasında *Ağı değiştir*; Telegram ayarları kalır.

## Sürümler

Sürümler ANA.ARA.YAMA biçiminde. Ne değişti, sırada ne var: [CHANGELOG.tr.md](CHANGELOG.tr.md).

## Lisans

- Yazılım, sayfalar ve bileşen: [MIT](LICENSE)
- Bağlantı şeması ve belgeler: [CC BY 4.0](LICENSE-HARDWARE)

Kaynak gösterme: *NOQ SOS, NOQ Open · open.noq.works/projects/sos*

Yaptın ya da geliştirdin mi? Bir [konu](https://github.com/noqworks/sos/issues) aç ya da hello@noq.works'e yaz.
