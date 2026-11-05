<p align="center"><img src="docs/noqopen.svg" alt="NOQ Open" height="40"></p>

# NOQ SOS

A call pendant with one button. A press sends a Telegram message to the family, straight from the pendant: no server, app or account of ours in between. Seeed Studio XIAO ESP32S3, a push button and a small LiPo cell.

**Project page and browser installer:** https://open.noq.works/projects/sos/ · [Türkçe](README.tr.md)

> **Test build (0.2.0).** It works on the bench: press, Wi-Fi, Telegram, commands. The battery reading is not tested yet and there is no case yet; both come in 0.3.0.

## What it does

- **One press, one message.** The board sleeps; a press wakes it, it joins the home Wi-Fi and sends "🆘 *name* needs help!" with the time of the press to every chat on its list.
- **Nothing in between.** The family creates its own Telegram bot; the pendant keeps the token and talks to Telegram itself.
- **Does not give up.** An alert that cannot go out (no Wi-Fi, no internet) is kept and retried after 1, 2, 5 and then every 10 minutes; the message says how late it is.
- **Says it is alive.** Optional silent daily check-in at a chosen hour, and a message when the battery runs low (with the battery reading fitted).
- **Its own pages.** First power-on opens a `noq-sos-xxxxxx` network with the NOQ Open Wi-Fi setup page; on the home network the pendant serves its own page at `http://noq-sos-xxxxxx.local` for the name, the bot, the recipients and a test message.
- **Joining is a tap.** While setup is open, anyone who taps *Start* in the bot (or adds it to a group) is on the list and gets a "Connected" reply. Outside setup nobody new is added.
- **Ask it from Telegram.** `/status`, `/battery`, `/help`, or in Turkish `/durum`, `/pil`, `/yardim`, in a private chat or a family group (there `/durum@bot`, which Telegram's `/` menu fills in). The answer comes as a reply, in the language of the command; only chats on the list get one. Awake, the pendant answers at once; asleep, it wakes every hour to answer (15 or 30 minutes, 3 hours or only on a press and the check-in: *Telegram commands* on its page).

## Repository

| Path | Contents |
|---|---|
| `firmware/noq-sos.yaml` | ESPHome configuration (pins, Wi-Fi, setup) |
| `components/noq_sos/` | ESPHome external component: sleep and wake-up, Telegram, retries, check-in, the setup page and the pendant's page with their API |
| `portal/` | Page sources: `tokens.css` → `components.css` → setup page `index.html` + `app.js`, pendant page `panel.html` + `panel.css` + `panel.js`; `build.py` inlines and gzips them into `components/noq_sos/portal_page.h` and `panel_page.h` |
| `releases.json` | Versions and release notes; `tools/changelog.py` writes `CHANGELOG.md` / `CHANGELOG.tr.md` from it |
| `docs/wiring.svg` | Wiring diagram |

## Build

Wire it as in `docs/wiring.svg`:

1. **Button:** one leg to D1, the diagonal leg to GND. No resistor; the internal pull-up is used.
2. **Cell:** a single LiPo cell on the BAT+ / BAT− pads on the back (red to BAT+). It charges over USB-C.
3. **Battery reading (optional):** BAT+ → 1 MΩ → D0 → 1 MΩ → GND, 100 nF from D0 to GND.
4. **Buzzer or vibration (optional):** D3 → +, − → GND; a motor through a transistor.

The XIAO's own LED (GPIO21) shows what happens: one blink on a press, three when the message is out, one long when it could not go.

## Firmware

**Browser:** open https://open.noq.works/install/sos/ in Chrome or Edge on a desktop. The wizard flashes the board over USB and joins Wi-Fi, then opens the pendant's page.

**ESPHome:**

```bash
esphome run firmware/noq-sos.yaml
```

Change pins with the `substitutions` at the top of the file. Later updates go over Wi-Fi while the pendant is awake.

**Pages:** edit files in `portal/`, then run `python3 portal/build.py` and rebuild the firmware.

### Awake and asleep

- Awake for 10 minutes after power-on, a reset or an install; while its page is open; while it is on a computer.
- Asleep otherwise. A press, a pending retry, the daily check-in or the command check (every hour by default) wakes it.
- To change something later: press the board's reset button (or plug it into a computer) and open its page.
- Change network: *Change network* on the pendant's page; the Telegram settings stay.

## Versions

Versions are MAJOR.MINOR.PATCH. What changed and what is planned next: [CHANGELOG.md](CHANGELOG.md).

## Licence

- Firmware, pages and component: [MIT](LICENSE)
- Wiring diagram and documentation: [CC BY 4.0](LICENSE-HARDWARE)

Attribution: *NOQ SOS, NOQ Open · open.noq.works/projects/sos*

Made one or improved it? Open an [issue](https://github.com/noqworks/sos/issues) or write to hello@noq.works.
