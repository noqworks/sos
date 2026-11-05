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
      lost: "Kolyeye ulaşılamıyor. Uykuya geçmiş olabilir: kartın RESET düğmesine bas ya da bilgisayara tak; kurulum için 10 dakika uyanık kalır.",
      awake: "Kurulum açık: bu sayfa açıkken kolye uyumaz.",
      usb: "Bilgisayara bağlı: kolye uyanık kalıyor.",
      pending: "Gönderilmeyi bekleyen bir bildirim var; internet gelince gider.",
      battery: "Pil %{p}",
      noBattery: "Pil ölçümü yok",
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
      lost: "Cannot reach the pendant. It may be asleep: press the board's RESET button or plug it into a computer; it stays awake for 10 minutes of setup.",
      awake: "Setup is open: the pendant stays awake while this page is open.",
      usb: "Connected to a computer: the pendant stays awake.",
      pending: "An alert is waiting to be sent; it goes out once the internet is back.",
      battery: "Battery {p}%",
      noBattery: "No battery reading",
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
    bat.textContent = S.battery ? t("battery", { p: S.battery.pct }) : t("noBattery");
    var wifi = $("chip-wifi");
    wifi.hidden = !S.wifi;
    if (S.wifi) wifi.textContent = "Wi-Fi · " + S.wifi.ssid;
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

  $("status").textContent = t("connecting");
  applyLang();
  poll();
})();
