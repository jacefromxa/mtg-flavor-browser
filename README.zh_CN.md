<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 万智牌箴言生成器 (MTG Wisdom & Flavor Generator)

> 专为 **FoloToy AI Passport** 掌上终端打造的万智牌多重宇宙随身时空秘典。

![封面示意](assets/cover.jpg)

## 📖 项目简介

万智牌不仅是一场智谋的决斗，更是一座浩瀚的叙事宝库。在万智牌三十余年的多重宇宙史诗中，那些印在卡牌底部的风味文字——无论是旅法师的深邃箴言、多明纳里亚的古老谚语，还是赞迪卡冒险者的决绝叹息，都是令人神往的心灵火花。

《万智牌箴言生成器》将 FoloToy AI Passport 变成一本随身时空秘典。只需轻轻一按，即可穿梭于多重宇宙，随机唤醒经典卡牌的风味背景与箴言引语，带来每日灵感与奇幻沉思。

## ✨ 核心特性

- **三重视图沉浸切换**：
  - **风味文字视图 (Flavor View)**：采用 20px 思源宋体 Heavy 大字重诗意呈现中文风味文字，辅以 12px 英文原文，去除牌名与费用干扰，静享纯粹阅读。
  - **高清大画幅牌面 (Face View)**：联网直连 Scryfall 官方高清卡牌，自适应单遍流式解码，2.4 英寸高亮竖屏全画幅展现，竖向空间利用率达 98.5%。
  - **规则与法术力符号 (Oracle View)**：内嵌官方 Mana Font 矢量符号库，还原经典法术力费用与规则叙述。
- **生动的时空抽牌动画与防抖互锁 (In-Flight Interlock)**：
  - 抽牌时居中呈现古典暗金模态框，伴随 5 颗发光法术力能量微粒波浪流水穿梭与状态轮播；
  - 严格静默拦截抽牌期间的重复按键，杜绝网络延迟导致的请求积压与多张牌突兀闪切。
- **千张离线底库 + 线上全时代牌池**：
  - 内置 1,000 张经典精选离线卡牌，开机无需配网即可畅玩；
  - 硬件真随机数发生器驱动 Fisher-Yates 洗牌算法，整轮抽取 0 重复；
  - 联网后接轨 Scryfall 线上全时代牌库，畅享无尽探索。
- **便捷网页热点配网 (Captive Portal)**：
  - 长按 OK 键随时进入配网，手机连接设备热点 `FoloPassport-WiFi` 访问 `http://192.168.4.1` 即可 10 秒完成 2.4GHz Wi-Fi 配置，开机自动重连。
- **全量汉字库扩充**：
  - 完整打包 GB2312 简体中文字库（6,763 个汉字）+ 万智牌专有词库，彻底杜绝生僻字缺字方框。

## 🎮 操作指引

| 操作 | 功能说明 |
| :--- | :--- |
| **单击 OK 键** | 时空漫游抽取下一张牌（联网时呈现抽牌动画并防抖锁定） |
| **单击 UP / DOWN 键** | 在【风味箴言】、【牌面插画】与【规则效果】三重视图间循环切换 |
| **长按 OK 键** | 进入/退出网页热点配网模式 |

## 🛠️ 构建与刷机

### 依赖环境
- ESP-IDF v5.5.3
- Python 3.9+

### 编译与验证
```bash
# 运行单元测试与静态检查
./tools/validate.sh --static

# 全量构建合并固件
./tools/validate.sh --firmware
```

### 烧录固件
```bash
python -m esptool --chip esp32c3 -p /dev/cu.usbmodem101 -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 \
  build/FoloToy-AI-Passport-full.bin
```

## 📜 许可证

MIT License
