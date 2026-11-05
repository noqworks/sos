# Sürüm notları

English: [CHANGELOG.md](CHANGELOG.md)

NOQ SOS için bütün değişiklikler, en yenisi üstte. Sürümler ANA.ARA.YAMA biçiminde:

- **Yama (1.0.x)**: düzeltme. Parçalar ve yazılım uyumlu kalır.
- **Ara sürüm (1.x.0)**: yeni özellik ya da parça, önceki yapımlarla uyumlu.
- **Ana sürüm (2.0.0)**: yeni parça ya da yeni kablolama gerektiren değişiklik.

_`releases.json` dosyasından `tools/changelog.py` ile üretilir. Bu dosyayı değil, onu düzenle._

## Sıradaki: 0.2.0

Planlananlar; talep ve sorulardan. Önerin ya da takıldığın bir yer varsa: [konular](https://github.com/noqworks/sos/issues).

### Donanım

- Baskı kolye kutusu: önde düğme, şarj için USB-C, ip için halka.

### Yazılım

- D0 bölücüsüyle pil ölçümü, masada denenmiş ve hücrenin gerçek yüzde eğrisiyle.
- Daha hızlı bildirim: Telegram bağlantı bilgisini uykuda saklayıp şifreli el sıkışmayı kısaltmak.

### Belgeler

- Yapım ve bitmiş kolye fotoğrafları.

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
