# Sürüm notları

English: [CHANGELOG.md](CHANGELOG.md)

NOQ SOS için bütün değişiklikler, en yenisi üstte. Sürümler ANA.ARA.YAMA biçiminde:

- **Yama (1.0.x)**: düzeltme. Parçalar ve yazılım uyumlu kalır.
- **Ara sürüm (1.x.0)**: yeni özellik ya da parça, önceki yapımlarla uyumlu.
- **Ana sürüm (2.0.0)**: yeni parça ya da yeni kablolama gerektiren değişiklik.

_`releases.json` dosyasından `tools/changelog.py` ile üretilir. Bu dosyayı değil, onu düzenle._

## Sıradaki: 0.3.0

Planlananlar; talep ve sorulardan. Önerin ya da takıldığın bir yer varsa: [konular](https://github.com/noqworks/sos/issues).

### Donanım

- Baskı kolye kutusu: önde düğme, şarj için USB-C, ip için halka.

### Yazılım

- D0 bölücüsüyle pil ölçümü, masada denenmiş ve hücrenin gerçek yüzde eğrisiyle.
- Daha hızlı bildirim: Telegram bağlantı bilgisini uykuda saklayıp şifreli el sıkışmayı kısaltmak.

### Belgeler

- Yapım ve bitmiş kolye fotoğrafları.

## 0.2.0 · 2026-11-05

Kolyeye Telegram'dan sor: /durum ve /pil, İngilizce /status ve /battery.

### Yazılım

- Özel sohbette ya da aile grubunda Telegram komutları: /durum, /pil, /yardim ve İngilizce /status, /battery, /help. Cevap komuta yanıt olarak, komutun dilinde gelir; yalnız listedeki sohbetler cevap alır. (Umut)
- Kolye uyurken saatte bir (ya da 15, 30 dakikada, 3 saatte bir ya da yalnız basınca ve yoklamada) uyanıp bekleyen komutları cevaplar; uyanıkken hemen cevaplar. (Umut)
- Komutlar Telegram'ın / menüsünde görünür, Türkçe uygulamada Türkçe. Cevaplanmış bir komut güç kesilince yeniden cevaplanmaz.

### Kurulum

- Kolyenin sayfası: Kolye bölümünde Telegram komutları ayarı; botun Bağlandı yanıtı komutları söyler.

**Uyumluluk:** 0.1.0'dan Wi-Fi üzerinden güncellenir; bot, kişiler ve ayarlar kalır. Saatte bir komutlara bakmak yalnız günlük yoklamadan daha çok pil harcar. Pil ölçümü hâlâ denenmedi, kutusu henüz yok.
**Teşekkürler:** Umut
**Dosyalar:** [kaynak ve baskı dosyaları (zip)](https://open.noq.works/files/sos/sos-0.2.0-source.zip) · [GitHub sürümü](https://github.com/noqworks/sos/releases/tag/v0.2.0)

## 0.1.0 · 2026-11-05

İlk deneme sürümü: tek basış, kolyeden doğrudan Telegram bildirimi gönderir.

### Yazılım

- XIAO ESP32S3 derin uykuda bekler; basınca uyanır, ev Wi-Fi'ına bağlanır ve listedeki bütün sohbetlere bildirim gönderir.
- Gönderilemeyen bildirim saklanır; 1, 2, 5 dakika sonra ve ardından 10 dakikada bir yeniden denenir, mesaj ne kadar geciktiğini yazar.
- İsteğe bağlı günlük sessiz yoklama ve pil azaldı mesajı; kolye uyanıkken Wi-Fi üzerinden güncelleme.

### Kurulum

- Wi-Fi tarayıcı kurulumundan ya da NOQ Open kurulum ağından; ardından kolyenin kendi sayfası açılır.
- O sayfada: BotFather /newbot hazır açılır, mesajının tamamı yapıştırılır (anahtarı sayfa ayıklar), sonra botta Başlat'a basan listeye girer.

**Uyumluluk:** Deneme sürümü. Pil ölçümü henüz denenmedi ve kutusu henüz yok; önce masada kur.
**Dosyalar:** [kaynak ve baskı dosyaları (zip)](https://open.noq.works/files/sos/sos-0.1.0-source.zip) · [GitHub sürümü](https://github.com/noqworks/sos/releases/tag/v0.1.0)
