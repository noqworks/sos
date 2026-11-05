(function () {
  "use strict";
  var T = {
    tr: {
      kicker: "Wi-Fi kurulumu",
      title: "Ev ağını seç",
      lede: "{p} yalnız 2,4 GHz Wi-Fi ağlarına bağlanır. Ağını seç, şifreni gir; gerisini cihaz halleder.",
      device: "Cihaz",
      nearby: "Yakındaki ağlar",
      refresh: "Yenile",
      empty: "Ağ bulunamadı. Modemin açık ve cihaza yakın olduğundan emin olup yenile.",
      manual: "Ağın listede yok mu? Adını yaz",
      ssid: "Ağ adı",
      psk: "Şifre",
      chosen: "Seçilen ağ: ",
      openNet: "Açık ağ, şifre gerekmez.",
      show: "Şifreyi göster",
      connect: "Bağlan",
      saving: "Kaydediliyor…",
      secured: "şifreli",
      error: "Kaydedilemedi. Cihaza yakın olduğundan emin olup yeniden dene.",
      savedKicker: "Kaydedildi",
      doneTitle: "{p} ev ağına bağlanıyor",
      doneLede: "Bu kurulum ağı birkaç saniye içinde kapanır, telefonun ev ağına kendiliğinden döner.",
      s1: "Telefonun ev ağına dönsün; birkaç saniye sürer.",
      s2: "Tarayıcıda <b class=\"np-strong\">{h}.local</b> adresini aç; Telegram'ı ve bildirim alacak kişileri oradan eklersin. Açılmazsa modeminin cihaz listesinde {h} adının yanındaki IP adresini aç.",
      s3: "Kurulum bitene kadar kolye uyumaz. Sayfayı kapatınca birkaç dakika sonra uykuya geçer; düğmeye basılınca uyanır.",
      retry: "Bağlanamazsa bu kurulum ağı açık kalır; şifreyi kontrol edip yeniden dene.",
      openPage: "Kolyenin sayfasını aç",
      openHint: "Telefon ev ağına dönünce dokun; Telegram'ı oradan bağlarsın. Sayfa bir dakika içinde kendisi de açılır.",
      foot: "NOQopen · açık kaynak"
    },
    en: {
      kicker: "Wi-Fi setup",
      title: "Choose your home network",
      lede: "{p} connects to 2.4 GHz Wi-Fi networks only. Pick your network, enter its password and the device takes it from there.",
      device: "Device",
      nearby: "Nearby networks",
      refresh: "Refresh",
      empty: "No networks found. Make sure your router is on and close to the device, then refresh.",
      manual: "Network not listed? Type its name",
      ssid: "Network name",
      psk: "Password",
      chosen: "Selected network: ",
      openNet: "Open network, no password needed.",
      show: "Show password",
      connect: "Connect",
      saving: "Saving…",
      secured: "secured",
      error: "Could not save. Stay close to the device and try again.",
      savedKicker: "Saved",
      doneTitle: "{p} is joining your network",
      doneLede: "This setup network closes in a few seconds and your phone returns to your home network on its own.",
      s1: "Let your phone return to your home network; it takes a few seconds.",
      s2: "Open <b class=\"np-strong\">{h}.local</b> in a browser to connect Telegram and choose who gets the alerts. If it does not open, use the IP address listed next to {h} in your router.",
      s3: "The pendant stays awake until setup is done. A few minutes after you close the page it goes to sleep; a press wakes it.",
      retry: "If it cannot connect, this setup network stays open; check the password and try again.",
      openPage: "Open the pendant's page",
      openHint: "Tap once your phone is back on your home network; you connect Telegram there. The page also opens on its own within a minute.",
      foot: "NOQopen · open source"
    }
  };
  var nav = (navigator.language || "en").toLowerCase();
  var lang = nav.indexOf("tr") === 0 ? "tr" : "en";
  var s = { product: "NOQ SOS", id: "", host: "", aps: null, sel: null, manual: false, tries: 0, timer: 0 };

  function $(id) { return document.getElementById(id); }
  function t(k) { return T[lang][k].split("{p}").join(s.product).split("{h}").join(esc(s.host || s.product.toLowerCase())); }
  function esc(v) {
    return String(v).replace(/[&<>"']/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;", "'": "&#39;" }[c];
    });
  }
  function bars(rssi) {
    var n = rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -75 ? 2 : 1, out = "";
    for (var i = 0; i < 4; i++) {
      var h = 4 + i * 4;
      out += "<rect" + (i < n ? "" : " class=\"off\"") + " x=\"" + (3 + i * 5) + "\" y=\"" + (20 - h) + "\" width=\"3\" height=\"" + h + "\" rx=\"1\" fill=\"currentColor\" stroke=\"none\"/>";
    }
    return "<svg class=\"np-icon np-bars\" viewBox=\"0 0 24 24\" aria-hidden=\"true\">" + out + "</svg>";
  }
  var LOCK = "<svg class=\"np-icon\" viewBox=\"0 0 24 24\" aria-hidden=\"true\"><rect x=\"5\" y=\"11\" width=\"14\" height=\"9\" rx=\"2\"/><path d=\"M8 11V8a4 4 0 0 1 8 0v3\"/></svg>";

  function applyLang() {
    document.documentElement.lang = lang;
    var els = document.querySelectorAll("[data-t]");
    for (var i = 0; i < els.length; i++) els[i].innerHTML = t(els[i].getAttribute("data-t"));
    var b = document.querySelectorAll("[data-lang]");
    for (var j = 0; j < b.length; j++) b[j].setAttribute("aria-current", b[j].getAttribute("data-lang") === lang ? "true" : "false");
    $("eye").setAttribute("aria-label", t("show"));
    $("device").textContent = t("device") + " · " + s.product + (s.id ? " " + s.id : "");
    renderNets();
    renderForm();
  }

  function renderNets() {
    var ul = $("nets");
    if (s.aps === null) {
      ul.innerHTML = "<li class=\"np-skel\"></li><li class=\"np-skel\"></li><li class=\"np-skel\"></li>";
      $("empty").hidden = true;
      return;
    }
    var html = "";
    for (var i = 0; i < s.aps.length; i++) {
      var a = s.aps[i];
      var on = !s.manual && s.sel && s.sel.ssid === a.ssid;
      html += "<li><button type=\"button\" class=\"np-row\" data-i=\"" + i + "\" aria-pressed=\"" + (on ? "true" : "false") + "\">" +
        "<span class=\"np-row-name\">" + esc(a.ssid) + "</span><span class=\"np-row-meta\">" +
        (a.lock ? "<span aria-label=\"" + t("secured") + "\" role=\"img\">" + LOCK + "</span>" : "") + bars(a.rssi) + "</span></button></li>";
    }
    ul.innerHTML = html;
    $("empty").hidden = s.aps.length > 0;
  }

  function renderForm() {
    var show = s.manual || !!s.sel;
    $("form").hidden = !show;
    if (!show) return;
    $("ssid-field").hidden = !s.manual;
    var open = !s.manual && s.sel && !s.sel.lock;
    $("psk-field").hidden = open;
    $("chosen").hidden = s.manual;
    $("chosen").innerHTML = s.sel && !s.manual ? t("chosen") + "<b class=\"np-strong\">" + esc(s.sel.ssid) + "</b>" + (open ? " · " + t("openNet") : "") : "";
  }

  function load() {
    clearTimeout(s.timer);
    fetch("/config.json", { cache: "no-store" }).then(function (r) { return r.json(); }).then(function (d) {
      var name = String(d.name || ""), m = name.match(/^(.*)-([0-9a-f]{6})$/);
      var base = m ? m[1] : name;
      s.product = "NOQ SOS";
      s.id = m ? m[2] : "";
      s.host = name;
      document.title = s.product + " · Wi-Fi";
      var best = {};
      (d.aps || []).forEach(function (a) {
        if (!a || !a.ssid) return;
        if (!best[a.ssid] || best[a.ssid].rssi < a.rssi) best[a.ssid] = { ssid: a.ssid, rssi: +a.rssi, lock: !!+a.lock };
      });
      var list = Object.keys(best).map(function (k) { return best[k]; }).sort(function (x, y) { return y.rssi - x.rssi; });
      if (!list.length && s.tries++ < 5) { s.aps = null; s.timer = setTimeout(load, 3000); }
      else s.aps = list;
      applyLang();
    }).catch(function () {
      if (s.tries++ < 5) s.timer = setTimeout(load, 3000); else { s.aps = []; applyLang(); }
    });
  }

  document.addEventListener("click", function (e) {
    var el = e.target.closest ? e.target.closest("button") : null;
    if (!el) return;
    if (el.hasAttribute("data-lang")) { lang = el.getAttribute("data-lang"); applyLang(); return; }
    if (el.hasAttribute("data-i")) {
      s.sel = s.aps[+el.getAttribute("data-i")]; s.manual = false; $("err").hidden = true;
      renderNets(); renderForm();
      if (s.sel.lock) $("psk").focus(); else $("connect").focus();
      return;
    }
    if (el.id === "manual") { s.manual = true; s.sel = null; renderNets(); renderForm(); $("ssid").focus(); return; }
    if (el.id === "refresh") { s.aps = null; s.tries = 0; renderNets(); load(); return; }
    if (el.id === "eye") {
      var p = $("psk"), on = p.type === "password";
      p.type = on ? "text" : "password"; el.setAttribute("aria-pressed", on ? "true" : "false");
    }
  });

  $("form").addEventListener("submit", function (e) {
    e.preventDefault();
    var ssid = s.manual ? $("ssid").value.trim() : (s.sel && s.sel.ssid);
    var psk = $("psk-field").hidden ? "" : $("psk").value;
    if (!ssid) { $("ssid").focus(); return; }
    var btn = $("connect");
    btn.disabled = true; btn.textContent = t("saving"); $("err").hidden = true;
    fetch("/wifisave?ssid=" + encodeURIComponent(ssid) + "&psk=" + encodeURIComponent(psk), { cache: "no-store" })
      .then(function (r) {
        if (!r.ok) throw new Error(r.status);
        $("setup").hidden = true; $("done").hidden = false; window.scrollTo(0, 0);
        openWhenHome();
      })
      .catch(function () { $("err").hidden = false; })
      .then(function () { btn.disabled = false; btn.textContent = t("connect"); });
  });

  // iPhone setup sheets cannot leave for Safari with a plain link; x-safari-http opens the page in Safari (iOS 17+)
  function pageUrl() { return "http://" + (s.host || "noq-sos") + ".local/"; }
  function openWhenHome() {
    var ios = /iPhone|iPad|iPod/.test(navigator.userAgent);
    var a = $("open-page");
    a.href = (ios ? "x-safari-" : "") + pageUrl();
    var tries = 0;
    (function probe() {
      if (tries++ > 60) return;
      var img = new Image();
      img.onload = function () { location.href = pageUrl(); };
      img.onerror = function () { setTimeout(probe, 2000); };
      img.src = pageUrl() + "icon.png?t=" + Date.now();
    })();
  }

  applyLang();
  load();
})();
