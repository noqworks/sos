# Changelog

Türkçe: [CHANGELOG.tr.md](CHANGELOG.tr.md)

All changes to NOQ SOS, newest first. Versions are MAJOR.MINOR.PATCH:

- **Patch (1.0.x)**: fixes. Parts and firmware stay compatible.
- **Minor (1.x.0)**: new features or parts, compatible with earlier builds.
- **Major (2.0.0)**: changes that need new parts or new wiring.

_Generated from `releases.json` by `tools/changelog.py`. Edit that file, not this one._

## Next: 0.4.0

Planned, from requests and questions. Suggest something or tell us what you hit: [issues](https://github.com/noqworks/sos/issues).

### Firmware

- Faster alerts: keep the Telegram connection details across sleep so the secure handshake is shorter.

### Documentation

- Photos of the build and the finished pendant.

## 0.3.0 · 2026-11-11

Printed case, battery log, /wake and discovery mode.

### Hardware

- A printed pendant case: button on the front, USB-C for charging, a loop for the cord.

### Firmware

- Battery log: the cell read before Wi-Fi at every wake, a charge indicator, the last charge, estimated days left and how long the previous charges lasted; on the pendant's page, in /battery and in the check-in. (Umut)
- An impossible battery reading (above 4.4 V or wandering: the D0 divider’s lower resistor or ground open) is shown as “cannot read” with the voltage and what to check, and no longer makes up a percentage, charges or time left; a log written from such readings is cleared. (Umut)
- /wake and /uyan: wakes a wrapped-up pendant (RESET out of reach) for 10 minutes and opens its page. (Umut)
- Discovery mode: started from the pendant's page; for 20 minutes the button sends no alert, each press makes the light tell the Wi-Fi signal and the page lists the spots. (Umut)

### Documentation

- Wiring diagram in Turkish too; the guide explains changing settings later with /wake.

**Compatibility:** Update over Wi-Fi from 0.2.0; the bot, the people and the settings stay. The battery log starts with the first wake after the update. The case fits the XIAO ESP32S3 stack in the README; for another battery or layout change the sizes in hardware/cad/sos_case.py and print it again.
**Thanks:** Umut
**Files:** [source and print files (zip)](https://open.noq.works/files/sos/sos-0.3.0-source.zip) · [GitHub release](https://github.com/noqworks/sos/releases/tag/v0.3.0)

## 0.2.0 · 2026-11-05

Ask the pendant from Telegram: /status and /battery, in Turkish /durum and /pil.

### Firmware

- Telegram commands in a private chat or a family group: /status, /battery, /help and in Turkish /durum, /pil, /yardim. The answer comes as a reply in the language of the command; only chats on the list get one. (Umut)
- Asleep, the pendant wakes every hour (or every 15 or 30 minutes, every 3 hours, or only on a press and the check-in) to answer waiting commands; awake, it answers at once. (Umut)
- The commands show up in Telegram's / menu, in Turkish for Turkish-language apps. A command already answered is not answered again after a power loss.

### Setup

- Pendant page: Telegram commands setting under Pendant; the bot's Connected reply names the commands.

**Compatibility:** Update over Wi-Fi from 0.1.0; the bot, the people and the settings stay. Checking for commands every hour uses more battery than the daily check-in alone. Battery reading still not tested and no case yet.
**Thanks:** Umut
**Files:** [source and print files (zip)](https://open.noq.works/files/sos/sos-0.2.0-source.zip) · [GitHub release](https://github.com/noqworks/sos/releases/tag/v0.2.0)

## 0.1.0 · 2026-11-05

First test build: one press sends a Telegram alert straight from the pendant.

### Firmware

- Deep sleep on a XIAO ESP32S3; a press wakes it, joins the home Wi-Fi and alerts every chat on the list.
- An alert that cannot go out is kept and retried after 1, 2, 5 and then every 10 minutes; the message says how late it is.
- Optional daily silent check-in and a low battery message; updates over Wi-Fi while the pendant is awake.

### Setup

- Wi-Fi from the browser installer or the NOQ Open setup network, which then opens the pendant's own page.
- On that page: open BotFather with /newbot ready, paste its whole message (the token is picked out), then tap Start in the bot to join the list.

**Compatibility:** Test build. Battery reading is not tested yet and there is no case yet; build it on the bench first.
**Files:** [source and print files (zip)](https://open.noq.works/files/sos/sos-0.1.0-source.zip) · [GitHub release](https://github.com/noqworks/sos/releases/tag/v0.1.0)
