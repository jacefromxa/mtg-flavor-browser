<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Magic: The Gathering Wisdom & Flavor Generator

> A pocket multiverse grimoire crafted for the **FoloToy AI Passport** handheld terminal.

![Cover Art](assets/cover.jpg)

## 📖 Introduction

Magic: The Gathering is not just a game of strategy, but a vast multiverse of epic lore. For over thirty years, the flavor texts woven into cards—from the deep reflections of Planeswalkers to ancient Dominarian proverbs—have sparked wonder and imagination.

The *MTG Wisdom & Flavor Generator* transforms your FoloToy AI Passport into a portable multiverse grimoire. With a single click, travel across realms to summon timeless quotes, poetic lore, and classical card wisdom for daily inspiration and fantasy immersion.

## ✨ Features

- **Three Evocative Views**:
  - **Flavor View**: Renders classic Chinese flavor texts in 20px Source Han Serif Heavy, complemented by 12px English original quotes, omitting card names and mana costs for pure, focused reading.
  - **Full Card Art View**: Connects directly to Scryfall official high-definition card art via streaming Baseline JPEG decoding, utilizing 98.5% of the 2.4-inch vertical display.
  - **Oracle & Mana Font View**: Embeds official Mana Font vector symbols with authentic background badges and rules text.
- **Dynamic Draw Animation & In-Flight Interlock**:
  - Displays an ornate floating modal card with a 5-dot flowing golden mana spark animation during online card fetches.
  - Strictly interlocks and debounces repeated button presses until the new card is fully loaded and displayed.
- **1,000 Offline Classics + Infinite Online Library**:
  - Preloaded with 1,000 curated classic cards for instant offline play without network setup.
  - True hardware random entropy drives Fisher-Yates deck shuffling with 0 duplicate draws per cycle.
  - Connect to Wi-Fi to unlock Scryfall's all-era online card database.
- **Effortless Captive Portal Wi-Fi Setup**:
  - Long-press OK to launch setup mode. Connect your phone to hotspot `FoloPassport-WiFi`, open `http://192.168.4.1` in any browser, and configure 2.4GHz Wi-Fi in seconds.
- **Full Chinese Font Coverage**:
  - Fully embeds the complete GB2312 Simplified Chinese character set (6,763 characters) and MTG vocabulary, eliminating missing glyphs and placeholder boxes.

## 🎮 Controls

| Action | Function |
| :--- | :--- |
| **Click OK** | Draw a new card across the multiverse (shows loading animation and interlocks when online) |
| **Click UP / DOWN** | Cycle seamlessly between Flavor Text, Full Card Art, and Oracle Rules views |
| **Long-Press OK** | Enter or exit SoftAP captive web Wi-Fi configuration |

## 🛠️ Build and Flash

### Requirements
- ESP-IDF v5.5.3
- Python 3.9+

### Validation and Build
```bash
# Run host tests and static validation
./tools/validate.sh --static

# Build full merged firmware
./tools/validate.sh --firmware
```

### Flashing
```bash
python -m esptool --chip esp32c3 -p /dev/cu.usbmodem101 -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 \
  build/FoloToy-AI-Passport-full.bin
```

## 📜 License

MIT License
