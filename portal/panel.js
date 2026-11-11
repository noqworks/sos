// NOQ SOS page. Served by the pendant itself on the home network and driven by its small API: GET /sos/state and
// POST /sos/<action>. While this page is open the pendant stays awake; network calls (Telegram) run on the device and
// the page follows their result through /sos/state.
(function () {
  "use strict";
  var T = {
    tr: {
      kicker: "Cihazın kendi sayfası",
      lede: "Kolyeye basılınca seçtiğin kişilere Telegram'dan bildirim gider. Arada sunucu, uygulama ya da hesap yok: mesajı kolye kendisi gönderir.",
      connecting: "Kolyeye bağlanılıyor…",
      lost: "Kolyeye ulaşılamıyor. Uykuya geçmiş olabilir: Telegram'dan /uyan yaz (bir sonraki bakışında 10 dakika uyanır) ya da USB-C'den bilgisayara tak. Kart açıktaysa RESET düğmesi de olur.",
      awake: "Kurulum açık: bu sayfa açıkken kolye uyumaz.",
      usb: "Bilgisayara bağlı: kolye uyanık kalıyor.",
      pending: "Gönderilmeyi bekleyen bir bildirim var; internet gelince gider.",
      battery: "Pil %{p}",
      charging: "Şarj oluyor · ≈%{p}",
      sigGood: "iyi",
      kesifKicker: "Keşif modu",
      kesifTitle: "Wi-Fi her köşede yetiyor mu?",
      kesifHelp: "Kolye yalnız ev Wi-Fi'siyle haber verir; kullanılacağı her yerde sinyal güçlü olmalı. Keşfi başlat, kolyeyle evi dolaş ve her köşede düğmeye bas. Keşif boyunca düğme bildirim göndermez; ışık (takılıysa buzzer) sinyali söyler: bir kısa iyi, iki kısa orta, üç kısa zayıf, bir uzun bağlantı yok. 20 dakika sonra kendiliğinden kapanır, düğme yeniden bildirim gönderir.",
      kesifStart: "Keşfi başlat",
      kesifStop: "Keşfi bitir",
      kesifOn: "Keşif açık: düğme şu an bildirim göndermiyor · {m} dk kaldı.",
      kesifNow: "Şu an {s}.",
      kesifOff: "Keşif bitti: düğme yine bildirim gönderiyor.",
      kesifPoint: "Nokta {n}",
      kesifNone: "bağlantı yok",
      sigFair: "orta",
      sigWeak: "zayıf",
      full: "Pil dolu",
      batKicker: "Pil",
      batTitle: "Şarj ve kalan süre",
      batNow: "Şimdi",
      batLast: "Son şarj",
      batLeft: "Tahmini kalan",
      batPast: "Önceki şarjlar",
      batPct: "%{p}",
      batApprox: "{w} civarı",
      batToFull: "dolu",
      batNoCharge: "Henüz kaydedilmedi",
      batLeftCharging: "Şarjda",
      batLeftCalc: "İlk günün ölçümlerinden sonra",
      batLeftOne: "Yaklaşık bir gün",
      batLeftDays: "Yaklaşık {d} gün",
      batDays: "{d} gün",
      batHelp: "Kolye her uyandığında Wi-Fi açılmadan önce pili ölçer. Tahmin bu ölçümlerin düşüş hızından çıkar, pil azaldıkça netleşir. Şarjdayken yüzde yaklaşıktır (≈): şarj akımı gerilimi olduğundan yüksek gösterir.",
      noBattery: "Pil ölçümü yok",
      batFault: "Pil ölçülemiyor",
      batFaultNow: "Ölçülemiyor · D0 {v}",
      batFaultHelp: "Tek hücre LiPo en çok 4,2 V olur; bu okuma pilin değil. D0 ile GND arasındaki 1 MΩ direnci ve GND bağlantısını kontrol et: pil takılıyken D0–GND pilin yarısı olmalı (dolu pilde ~2,1 V). Düzelene kadar yüzde, şarj ve süre gösterilmez.",
      whoKicker: "Kolye kimin",
      whoTitle: "Mesajlarda görünecek ad",
      whoHelp: "Bildirim \"🆘 Ayşe Teyze yardım istiyor!\" gibi gelir.",
      nameLabel: "Ad",
      namePh: "Örn. Ayşe Teyze",
      save: "Kaydet",
      saved: "Kaydedildi",
      botKicker: "Telegram",
      botTitle: "Kolyenin botu",
      bot1: "<b class=\"np-strong\">BotFather'ı aç</b>'a dokun; Telegram /newbot yazılı açılır, gönder.",
      bot2: "Ad sorunca bir ad yaz (ör. \"Ayşe Teyze SOS\"); kullanıcı adı sorunca şunu yapıştır: <b class=\"np-strong np-code\">{user}</b>",
      bot3: "BotFather'ın \"Done! Congratulations…\" mesajını olduğu gibi kopyalayıp aşağıya yapıştır; anahtarı sayfa kendisi ayıklar. Anahtar yalnız kolyede saklanır.",
      openBotFather: "BotFather'ı aç",
      copyUser: "Kullanıcı adını kopyala",
      copied: "Kopyalandı",
      tokenLabel: "Bot anahtarı",
      show: "Anahtarı göster",
      connect: "Bağla",
      checking: "Kontrol ediliyor…",
      tokenBad: "Bu anahtar çalışmadı. BotFather'ın mesajını yeniden kopyalayıp yapıştır.",
      tokenNone: "Yapıştırdığın metinde bot anahtarı yok. BotFather'ın \"Done! Congratulations…\" mesajını kopyala.",
      network: "Kolye internete ulaşamadı. Wi-Fi bağlantısını kontrol edip yeniden dene.",
      botConnected: "Bağlı bot",
      change: "Değiştir",
      peopleKicker: "Bildirim alacaklar",
      peopleTitle: "Kimlere haber gitsin?",
      peopleLede: "<b class=\"np-strong\">Telegram'da aç</b>'a dokun, açılan sohbette <b class=\"np-strong\">Başlat</b>'a bas: birkaç saniyede aşağıdaki listeye eklenirsin, bot da \"Bağlandı\" yazar. Aile grubu için <b class=\"np-strong\">Gruba ekle</b>'yi seç.",
      openTelegram: "Telegram'da aç",
      addGroup: "Gruba ekle",
      others: "Başka biri de bildirim alsın mı? Bu bağlantıyı ona gönder; kurulum açıkken Başlat'a basması yeterli.",
      shareWa: "WhatsApp'ta gönder",
      shareText: "Kolyeden gelecek acil durum bildirimleri için bu bağlantıyı aç ve Başlat'a bas: {link}",
      copyLink: "Bağlantıyı kopyala",
      peopleList: "Bildirim alacaklar",
      noneYet: "Henüz kimse yok. Telegram'da aç'a dokunup Başlat'a bas.",
      remove: "Çıkar",
      max: "En çok 6 alıcı olabilir.",
      testKicker: "Dene",
      testTitle: "Deneme mesajı",
      testHelp: "Listedeki herkese \"deneme\" yazan bir mesaj gider. Gerçek bildirimde kolyenin düğmesine basılır.",
      testAction: "Deneme gönder",
      sending: "Gönderiliyor…",
      testOk: "Gönderildi: {r} alıcıya ulaştı.",
      testFail: "Gönderilemedi. Alıcılar bota Başlat dedi mi? İnternet bağlantısını da kontrol et.",
      last: "Son bildirim {t} · {s}",
      lastTook: " · basıştan {sec} sn sonra",
      lastOk: "herkese ulaştı",
      lastPart: "bazı alıcılara ulaşmadı",
      setKicker: "Ayarlar",
      setTitle: "Kolye",
      msgLang: "Mesaj dili",
      msgLangHelp: "Telegram'a giden mesajların dili.",
      checkin: "Günlük yoklama",
      checkinHelp: "Her gün bu saatte sessiz bir \"kolye çalışıyor\" mesajı gelir. Gelmediği gün pil bitmiş ya da kolye ev ağından çıkmıştır.",
      off: "Kapalı",
      poll: "Telegram komutları",
      pollHelp: "Sohbette /durum ya da /pil yazılınca kolye uyuyorsa bu sıklıkta bakıp cevaplar. Daha sık bakmak pili daha çabuk bitirir.",
      pollOff: "Yalnız basınca ve yoklamada",
      pollEvery: "{m} dakikada bir",
      pollHour: "Saatte bir",
      pollHours: "{h} saatte bir",
      tz: "Saat dilimi",
      tzUse: "Bu telefonun saat dilimini kullan ({z})",
      wifi: "Ev ağı",
      wifiNone: "Bağlı değil",
      wifiChange: "Ağı değiştir",
      wifiSure: "Kolye bu ağı unutur ve yeniden başlar; {host} adlı kurulum ağı açılır. Telegram ayarları kalır.",
      wifiYes: "Ağı unut",
      cancel: "Vazgeç",
      restarting: "Kolye yeniden başlıyor. Telefonunu {host} kurulum ağına bağla.",
      finish: "Kurulumu bitir",
      finishHelp: "Kolye hemen uykuya geçer; düğmeye basılınca uyanır.",
      finishAction: "Bitir",
      sleeping: "Kolye uykuya geçti. Bu sayfayı kapatabilirsin.",
      version: "Yazılım {v}",
      foot: "NOQopen · açık kaynak"
    },
    en: {
      kicker: "The device's own page",
      lede: "A press on the pendant sends a Telegram alert to the people you choose. No server, app or account in between: the pendant sends the message itself.",
      connecting: "Connecting to the pendant…",
      lost: "Cannot reach the pendant. It may be asleep: send /wake in Telegram (it wakes for 10 minutes at its next check) or plug it into a computer over USB-C. If the board is still open, its RESET button works too.",
      awake: "Setup is open: the pendant stays awake while this page is open.",
      usb: "Connected to a computer: the pendant stays awake.",
      pending: "An alert is waiting to be sent; it goes out once the internet is back.",
      battery: "Battery {p}%",
      charging: "Charging · ≈{p}%",
      sigGood: "good",
      kesifKicker: "Discovery mode",
      kesifTitle: "Is the Wi-Fi strong in every corner?",
      kesifHelp: "The pendant can only call through the home Wi-Fi, so the signal must be strong wherever it will be used. Start discovery, walk the house with the pendant and press the button in each spot. During discovery the button sends no alert; the light (and buzzer, if fitted) tells the signal: one short good, two short fair, three short weak, one long no network. It ends by itself after 20 minutes and the button sends alerts again.",
      kesifStart: "Start discovery",
      kesifStop: "End discovery",
      kesifOn: "Discovery on: the button sends no alert right now · {m} min left.",
      kesifNow: "Now {s}.",
      kesifOff: "Discovery ended: the button sends alerts again.",
      kesifPoint: "Spot {n}",
      kesifNone: "no network",
      sigFair: "fair",
      sigWeak: "weak",
      full: "Battery full",
      batKicker: "Battery",
      batTitle: "Charge and time left",
      batNow: "Now",
      batLast: "Last charge",
      batLeft: "Estimated time left",
      batPast: "Previous charges",
      batPct: "{p}%",
      batApprox: "around {w}",
      batToFull: "full",
      batNoCharge: "Not recorded yet",
      batLeftCharging: "Charging",
      batLeftCalc: "After the first day of readings",
      batLeftOne: "About a day",
      batLeftDays: "About {d} days",
      batDays: "{d} days",
      batHelp: "Each time the pendant wakes it measures the battery before Wi-Fi starts. The estimate comes from how fast that reading falls and gets sharper as the battery runs down. While charging the percentage is approximate (≈): the charge current makes the voltage read high.",
      noBattery: "No battery reading",
      batFault: "Cannot read the battery",
      batFaultNow: "Cannot read · D0 {v}",
      batFaultHelp: "A single LiPo cell is 4.2 V at most; this reading is not the battery. Check the 1 MΩ between D0 and GND and its ground: with the battery in, D0 to GND should be half the battery (~2.1 V when full). Until then no percentage, charge or time left is shown.",
      whoKicker: "Whose pendant",
      whoTitle: "The name in the messages",
      whoHelp: "Alerts read like \"🆘 Grandma Ann needs help!\".",
      nameLabel: "Name",
      namePh: "e.g. Grandma Ann",
      save: "Save",
      saved: "Saved",
      botKicker: "Telegram",
      botTitle: "The pendant's bot",
      bot1: "Tap <b class=\"np-strong\">Open BotFather</b>; Telegram opens with /newbot typed in, send it.",
      bot2: "When it asks for a name, type one (e.g. \"Grandma Ann SOS\"); for the username, paste: <b class=\"np-strong np-code\">{user}</b>",
      bot3: "Copy BotFather's \"Done! Congratulations…\" message as it is and paste it below; the page picks the token out. It is stored on the pendant only.",
      openBotFather: "Open BotFather",
      copyUser: "Copy username",
      copied: "Copied",
      tokenLabel: "Bot token",
      show: "Show token",
      connect: "Connect",
      checking: "Checking…",
      tokenBad: "That token did not work. Copy BotFather's message again and paste it.",
      tokenNone: "There is no bot token in that text. Copy BotFather's \"Done! Congratulations…\" message.",
      network: "The pendant could not reach the internet. Check the Wi-Fi and try again.",
      botConnected: "Connected bot",
      change: "Change",
      peopleKicker: "Who gets the alerts",
      peopleTitle: "Who should hear about it?",
      peopleLede: "Tap <b class=\"np-strong\">Open in Telegram</b> and press <b class=\"np-strong\">Start</b> in the chat: within seconds you are on the list below and the bot replies \"Connected\". For a family group choose <b class=\"np-strong\">Add to a group</b>.",
      openTelegram: "Open in Telegram",
      addGroup: "Add to a group",
      others: "Someone else should get alerts too? Send them this link; while setup is open they just press Start.",
      shareWa: "Send on WhatsApp",
      shareText: "Open this link and press Start to get the pendant's emergency alerts: {link}",
      copyLink: "Copy link",
      peopleList: "Getting the alerts",
      noneYet: "Nobody yet. Tap Open in Telegram and press Start.",
      remove: "Remove",
      max: "There can be up to 6 recipients.",
      testKicker: "Try it",
      testTitle: "Test message",
      testHelp: "Everyone on the list gets a message that says it is a test. A real alert comes from the pendant's button.",
      testAction: "Send a test",
      sending: "Sending…",
      testOk: "Sent: reached {r} recipients.",
      testFail: "Could not send. Did the recipients tap Start in the bot? Check the internet connection too.",
      last: "Last alert {t} · {s}",
      lastTook: " · {sec} s after the press",
      lastOk: "reached everyone",
      lastPart: "did not reach everyone",
      setKicker: "Settings",
      setTitle: "Pendant",
      msgLang: "Message language",
      msgLangHelp: "The language of the Telegram messages.",
      checkin: "Daily check-in",
      checkinHelp: "Every day at this hour a silent \"the pendant is working\" message arrives. If it does not, the battery is flat or the pendant is out of the home network.",
      off: "Off",
      poll: "Telegram commands",
      pollHelp: "When someone sends /status or /battery in the chat and the pendant is asleep, it checks this often and answers. Checking more often drains the battery faster.",
      pollOff: "Only on a press and the check-in",
      pollEvery: "Every {m} minutes",
      pollHour: "Every hour",
      pollHours: "Every {h} hours",
      tz: "Time zone",
      tzUse: "Use this phone's time zone ({z})",
      wifi: "Home network",
      wifiNone: "Not connected",
      wifiChange: "Change network",
      wifiSure: "The pendant forgets this network and restarts with the {host} setup network open. The Telegram settings stay.",
      wifiYes: "Forget network",
      cancel: "Cancel",
      restarting: "The pendant is restarting. Join the {host} setup network with your phone.",
      finish: "Finish setup",
      finishHelp: "The pendant goes to sleep now; a press wakes it.",
      finishAction: "Finish",
      sleeping: "The pendant is asleep. You can close this page.",
      version: "Firmware {v}",
      foot: "NOQopen · open source"
    }
  };

  var lang = (navigator.language || "en").toLowerCase().indexOf("tr") === 0 ? "tr" : "en";
  try { var saved = localStorage.getItem("np-lang"); if (saved === "tr" || saved === "en") lang = saved; } catch (x) { /* private mode */ }

  var S = null, wait = null, timer = 0, fails = 0, done = false, nameTouched = false, known = null;

  function $(id) { return document.getElementById(id); }
  function t(k, v) {
    var s = T[lang][k];
    for (var key in (v || {})) s = s.split("{" + key + "}").join(v[key]);
    return s;
  }
  function esc(v) {
    return String(v).replace(/[&<>"']/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;", "'": "&#39;" }[c];
    });
  }
  function show(id, on) { $(id).hidden = !on; }
  function note(id, text) { var el = $(id); el.textContent = text || ""; el.hidden = !text; }
  var MON = { tr: ["Oca", "Şub", "Mar", "Nis", "May", "Haz", "Tem", "Ağu", "Eyl", "Eki", "Kas", "Ara"],
    en: ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"] };
  function two(n) { return (n < 10 ? "0" : "") + n; }
  // the pendant's local time (its time zone), not the phone's
  function when(utc) {
    var d = new Date((utc + ((S && S.tz) || 0) * 60) * 1000);
    return d.getUTCDate() + " " + MON[lang][d.getUTCMonth()] + " " + two(d.getUTCHours()) + ":" + two(d.getUTCMinutes());
  }
  // same thresholds as /status: good from −60 dBm, fair from −70 dBm
  function sig(r) {
    return (r >= -60 ? t("sigGood") : r >= -70 ? t("sigFair") : t("sigWeak")) + " (" + String(r).replace("-", "−") + " dBm)";
  }
  function tzLabel(m) {
    var s = m < 0 ? "−" : "+", a = Math.abs(m), h = Math.floor(a / 60), mm = a % 60;
    return "UTC" + s + h + (mm ? ":" + (mm < 10 ? "0" : "") + mm : "");
  }

  function post(action, params) {
    var q = [];
    for (var k in (params || {})) q.push(k + "=" + encodeURIComponent(params[k]));
    return fetch("/sos/" + action + (q.length ? "?" + q.join("&") : ""), { method: "POST", cache: "no-store" })
      .then(function (r) { return r.json().then(function (d) { return { code: r.status, data: d }; }); })
      .then(function (r) { if (r.data && r.data.v !== undefined) { S = r.data; render(); } return r; });
  }

  // ---- render ---------------------------------------------------------------------------------------------------

  var editToken = false;

  function applyLang() {
    document.documentElement.lang = lang;
    var els = document.querySelectorAll("[data-t]");
    for (var i = 0; i < els.length; i++) els[i].innerHTML = t(els[i].getAttribute("data-t"));
    var b = document.querySelectorAll("[data-lang]");
    for (var j = 0; j < b.length; j++) b[j].setAttribute("aria-current", b[j].getAttribute("data-lang") === lang ? "true" : "false");
    $("eye").setAttribute("aria-label", t("show"));
    $("name").placeholder = t("namePh");
    var sel = $("hour"), v = sel.value, html = "<option value=\"-1\">" + esc(t("off")) + "</option>";
    for (var h = 6; h <= 22; h++) html += "<option value=\"" + h + "\">" + (h < 10 ? "0" : "") + h + ":00</option>";
    sel.innerHTML = html;
    if (v) sel.value = v;
    var ps = $("poll"), pv = ps.value, ph = "";
    [0, 15, 30, 60, 180].forEach(function (m) {
      var l = m === 0 ? t("pollOff") : m === 60 ? t("pollHour") : m > 60 ? t("pollHours", { h: m / 60 }) : t("pollEvery", { m: m });
      ph += "<option value=\"" + m + "\">" + esc(l) + "</option>";
    });
    ps.innerHTML = ph;
    if (pv) ps.value = pv;
    render();
  }

  function copy(text, btn) {
    var ta = document.createElement("textarea");
    ta.value = text; ta.setAttribute("readonly", ""); ta.style.position = "fixed"; ta.style.opacity = "0";
    document.body.appendChild(ta); ta.select();
    try { document.execCommand("copy"); } catch (x) { /* nothing to do */ }
    document.body.removeChild(ta);
    var old = btn.textContent;
    btn.textContent = t("copied");
    setTimeout(function () { btn.textContent = old; }, 1500);
  }
  // BotFather's message, a token with spaces or line breaks, or the token alone: keep the token only
  function pickToken(text) {
    var m = String(text || "").replace(/\s+/g, " ").match(/\d{6,12}:[A-Za-z0-9_-]{35}/);
    return m ? m[0] : "";
  }
  function botUser() { return (S && S.host ? S.host.replace(/-/g, "") : "noqsos").replace(/^noqsos/, "noqsos_") + "_bot"; }
  function startLink() { return "https://t.me/" + S.bot + "?start=sos"; }

  function render() {
    if (!S) return;
    $("chip-host").textContent = S.host + ".local";
    var bat = $("chip-battery");
    bat.hidden = false;
    var F = S.battery && S.battery.fault ? S.battery : null;   // impossible reading: divider fault, not the cell
    var B = F ? null : S.battery, chg = !!(B && B.charging && !B.full);
    bat.classList.toggle("is-none", !B);
    bat.classList.toggle("is-charging", chg);
    bat.classList.toggle("is-full", !!(B && B.full));
    bat.style.setProperty("--np-bat", B ? Math.max(B.pct, 4) / 100 : 0);
    $("bat-text").textContent = F ? t("batFault") : !B ? t("noBattery") : B.full ? t("full") : chg ? t("charging", { p: B.pct }) : t("battery", { p: B.pct });
    show("bat-card", !!(B || F));
    show("bat-fault", !!F);
    ["bat-last", "bat-left"].forEach(function (id) { $(id).parentElement.hidden = !!F; });
    show("bat-help", !F);
    if (F) {
      $("bat-now").textContent = t("batFaultNow", { v: (F.mv / 1000).toFixed(2).replace(".", lang === "tr" ? "," : ".") + " V" });
      $("bat-fault").textContent = t("batFaultHelp");
      show("bat-past-row", false);
    }
    if (B) {
      var volts = (B.mv / 1000).toFixed(2).replace(".", lang === "tr" ? "," : ".") + " V";
      $("bat-now").textContent = (B.full ? t("full") : chg ? t("charging", { p: B.pct }) : t("batPct", { p: B.pct })) + " · " + volts;
      var last = B.charged_at ? when(B.charged_at) : "";
      if (last && !B.charged_seen) last = t("batApprox", { w: last });
      $("bat-last").textContent = last ? last + (B.charged_full ? " · " + t("batToFull") : "") : t("batNoCharge");
      $("bat-left").textContent = B.charging ? t("batLeftCharging") : B.days_left === null || B.days_left === undefined ? t("batLeftCalc") :
        B.days_left < 1.5 ? t("batLeftOne") : t("batLeftDays", { d: Math.round(B.days_left) });
      var past = (B.past || []).map(function (p) { return t("batDays", { d: Math.max(1, Math.round(p.days)) }); }).join(" · ");
      show("bat-past-row", !!past);
      $("bat-past").textContent = past;
    }
    var wifi = $("chip-wifi");
    wifi.hidden = !S.wifi;
    if (S.wifi) wifi.textContent = "Wi-Fi · " + S.wifi.ssid + " · " + sig(S.wifi.rssi);

    var K = S.kesif, kon = !!(K && K.left > 0), pts = (K && K.points) || [];
    show("kesif-card", true);
    $("kesif").textContent = kon ? t("kesifStop") : t("kesifStart");
    $("kesif").setAttribute("aria-pressed", kon ? "true" : "false");
    note("kesif-live", kon ? t("kesifOn", { m: Math.max(1, Math.ceil(K.left / 60)) }) + (S.wifi ? " " + t("kesifNow", { s: sig(S.wifi.rssi) }) : "") :
      pts.length ? t("kesifOff") : "");
    $("kesif-list").hidden = !pts.length;
    $("kesif-list").innerHTML = pts.map(function (p, i) {
      var at = p.t ? when(p.t).split(" ").pop() : "";
      return '<div class="np-set"><div class="np-set-text"><p class="np-set-label">' + esc(t("kesifPoint", { n: i + 1 })) + "</p>" +
        (at ? '<p class="np-small">' + esc(at) + "</p>" : "") + '</div><span class="np-set-value">' + esc(p.rssi ? sig(p.rssi) : t("kesifNone")) + "</span></div>";
    }).join("");
    if (!done) $("status").textContent = (S.usb ? t("usb") : t("awake")) + (S.pending ? " " + t("pending") : "");

    var name = $("name");
    if (!nameTouched && document.activeElement !== name) name.value = S.name || "";

    $("bot2").innerHTML = t("bot2", { user: esc(botUser()) });
    var hasBot = !!S.token && !editToken;
    show("bot-new", !hasBot);
    show("bot-set", hasBot);
    if (S.bot) {
      var a = $("bot-link");
      a.href = "https://t.me/" + encodeURIComponent(S.bot);
      a.textContent = "@" + S.bot;
    }

    show("people-card", !!S.token);
    if (S.token && S.bot) {
      $("tg-start").href = startLink();
      $("tg-group").href = "https://t.me/" + S.bot + "?startgroup=sos";
      $("share-wa").href = "https://wa.me/?text=" + encodeURIComponent(t("shareText", { link: startLink() }));
      var html = "", ids = {};
      (S.people || []).forEach(function (p) {
        ids[p.id] = 1;
        html += "<li class=\"np-person" + (known && !known[p.id] ? " np-fresh" : "") + "\"><span class=\"np-row-name\">" + esc(p.label || p.id) +
          "</span><button type=\"button\" class=\"np-link\" data-rm=\"" + esc(p.id) + "\">" + esc(t("remove")) + "</button></li>";
      });
      known = ids;
      $("people").innerHTML = html;
      show("people-empty", !(S.people || []).length);
    }

    show("test-card", (S.people || []).length > 0);
    if (S.last) {
      var took = S.timing && S.timing.sent ? t("lastTook", { sec: (S.timing.sent / 1000).toFixed(1).replace(".", lang === "tr" ? "," : ".") }) : "";
      note("last", t("last", { t: S.last.at, s: S.last.ok ? t("lastOk") : t("lastPart") }) + took);
    }

    var pills = document.querySelectorAll("[data-ml]");
    for (var i = 0; i < pills.length; i++) pills[i].setAttribute("aria-pressed", pills[i].getAttribute("data-ml") === S.lang ? "true" : "false");
    if (document.activeElement !== $("hour")) $("hour").value = String(S.hour);
    if (document.activeElement !== $("poll") && S.poll != null) $("poll").value = String(S.poll);
    $("tz-value").textContent = tzLabel(S.tz);
    var phone = -new Date().getTimezoneOffset();
    $("tz-use").textContent = t("tzUse", { z: tzLabel(phone) });
    $("tz-use").hidden = phone === S.tz;
    $("wifi-value").textContent = S.wifi ? S.wifi.ssid : t("wifiNone");
    $("version").textContent = S.v ? t("version", { v: S.v }) : "";
  }

  // ---- state polling and jobs -----------------------------------------------------------------------------------

  function poll() {
    clearTimeout(timer);
    if (done) return;
    fetch("/sos/state", { cache: "no-store" }).then(function (r) { return r.json(); }).then(function (d) {
      S = d; fails = 0;
      render();
      if (wait && S.job.kind === wait.kind && S.job.state !== "busy") {
        var w = wait; wait = null;
        w.done(S.job);
      }
      timer = setTimeout(poll, wait ? 800 : 2500);
    }).catch(function () {
      if (++fails >= 2 && !done) $("status").textContent = t("lost");
      timer = setTimeout(poll, 4000);
    });
  }

  // Starts a job on the device; done(job) runs once it has finished.
  function job(action, kind, params, done) {
    return post(action, params).then(function (r) {
      if (r.code === 202) { wait = { kind: kind, done: done }; clearTimeout(timer); timer = setTimeout(poll, 800); }
      else done({ kind: kind, state: "failed", msg: r.code === 409 ? "busy" : "token" });
    }).catch(function () { done({ kind: kind, state: "failed", msg: "network" }); });
  }

  // ---- events ---------------------------------------------------------------------------------------------------

  document.addEventListener("click", function (e) {
    var el = e.target.closest ? e.target.closest("button") : null;
    if (!el) return;
    if (el.hasAttribute("data-lang")) {
      lang = el.getAttribute("data-lang");
      try { localStorage.setItem("np-lang", lang); } catch (x) { /* private mode */ }
      applyLang();
      return;
    }
    if (el.hasAttribute("data-ml")) { post("settings", { lang: el.getAttribute("data-ml") }); return; }
    if (el.hasAttribute("data-rm")) { post("remove", { id: el.getAttribute("data-rm") }); return; }
    switch (el.id) {
      case "eye":
        var p = $("token"), on = p.type === "password";
        p.type = on ? "text" : "password"; el.setAttribute("aria-pressed", on ? "true" : "false");
        return;
      case "token-change":
        editToken = true; render(); $("token").focus();
        return;
      case "copy-user":
        copy(botUser(), el);
        return;
      case "copy-link":
        copy(startLink(), el);
        return;
      case "test":
        el.disabled = true; el.textContent = t("sending"); note("test-msg", "");
        job("test", "test", null, function (j) {
          el.disabled = false; el.textContent = t("testAction");
          note("test-msg", j.state === "ok" ? t("testOk", { r: j.msg }) : t("testFail"));
        });
        return;
      case "tz-use":
        post("settings", { tz: -new Date().getTimezoneOffset() });
        return;
      case "wifi-change":
        show("wifi-confirm", true); $("wifi-confirm").querySelector("p").textContent = t("wifiSure", { host: S ? S.host : "noq-sos" });
        return;
      case "wifi-no":
        show("wifi-confirm", false);
        return;
      case "wifi-yes":
        post("wifi-reset").then(function () { done = true; $("status").textContent = t("restarting", { host: S.host }); show("wifi-confirm", false); });
        return;
      case "kesif":
        post("kesif", { on: S && S.kesif && S.kesif.left > 0 ? 0 : 1 });
        return;
      case "finish":
        post("sleep").then(function () { done = true; $("status").textContent = t("sleeping"); });
        return;
    }
  });

  $("name").addEventListener("input", function () { nameTouched = true; });
  $("who-form").addEventListener("submit", function (e) {
    e.preventDefault();
    var btn = $("name-save");
    btn.disabled = true;
    post("settings", { name: $("name").value.trim() }).then(function () {
      nameTouched = false; btn.textContent = t("saved");
      setTimeout(function () { btn.textContent = t("save"); }, 1500);
    }).catch(function () { $("status").textContent = t("lost"); }).then(function () { btn.disabled = false; });
  });

  $("token-form").addEventListener("submit", function (e) {
    e.preventDefault();
    var v = pickToken($("token").value);
    if (!v) { note("token-msg", t("tokenNone")); $("token").focus(); return; }
    $("token").value = v;
    var btn = $("token-save");
    btn.disabled = true; btn.textContent = t("checking"); note("token-msg", "");
    job("token", "token", { value: v }, function (j) {
      btn.disabled = false; btn.textContent = t("connect");
      if (j.state === "ok") { $("token").value = ""; editToken = false; render(); }
      else note("token-msg", j.msg === "network" ? t("network") : t("tokenBad"));
    });
  });

  $("token").addEventListener("input", function () {
    var v = pickToken($("token").value);
    if (v && v !== $("token").value) $("token").value = v;
    note("token-msg", "");
  });

  $("hour").addEventListener("change", function () { post("settings", { hour: $("hour").value }); });
  $("poll").addEventListener("change", function () { post("settings", { poll: $("poll").value }); });

  $("status").textContent = t("connecting");
  applyLang();
  poll();
})();
