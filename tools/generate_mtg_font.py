#!/usr/bin/env python3
"""Generate dedicated Noto Sans SC + Source Han Serif SC Heavy + Mana Font for MTG Flavor Browser."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent

# Mana Font 官方符号码位
MANA_CODEPOINTS = [
    0xE600, 0xE601, 0xE602, 0xE603, 0xE604, 0xE605, 0xE606, 0xE607,
    0xE608, 0xE609, 0xE60A, 0xE60B, 0xE60C, 0xE60D, 0xE60E, 0xE60F,
    0xE610, 0xE611, 0xE612, 0xE613, 0xE614, 0xE615, 0xE61A, 0xE62A,
    0xE62E, 0xE904
]

def collect_codepoints():
    # 基础可打印 ASCII
    all_cps = set(range(0x20, 0x7f))
    serif_cps = set(range(0x20, 0x7f))
    
    # 常用全角标点与符号
    extra_symbols = "“”—…·／～【】、，。！？：；（）《》+-*/#%0123456789『』「」"
    for ch in extra_symbols:
        all_cps.add(ord(ch))
        serif_cps.add(ord(ch))

    # 扫描 main 源码中的中文字符串
    scan_files = [
        ROOT / "main/mtg_cards.c",
        ROOT / "main/mtg_ui.c",
        ROOT / "main/mtg_state.c",
        ROOT / "main/mtg_wifi_prov.c",
    ]
    for p in scan_files:
        if p.exists():
            text = p.read_text(encoding="utf-8")
            for ch in text:
                if ord(ch) > 127:
                    all_cps.add(ord(ch))

    # 从 1000 张卡牌中专门提取风味文字码位给思源宋体
    cards_json = ROOT / "assets/cards_1000.json"
    if cards_json.exists():
        cards = json.loads(cards_json.read_text(encoding="utf-8"))
        for c in cards:
            for k in ["flavor_text", "flavor_author", "name_zh"]:
                val = c.get(k)
                if val:
                    for ch in val:
                        if ord(ch) > 127:
                            serif_cps.add(ord(ch))
                            all_cps.add(ord(ch))

    # 显式确保核心 UI 汉字覆盖
    ui_keywords = (
        "万智牌箴言浏览器经典背景叙述"
        "风味牌面效果规则闪电击黑莲花神之愤怒反击咒语黑暗祭礼巨木吞噬怪"
        "心灵塑识师杰斯时间漫步索蓝发电机月智者多美代沉思西瓦巨龙塞拉天使"
        "真名宿敌活旺力赞迪卡伙伴基定面纱的莉莲娜反抗烈炬茜卓撼地妮莎"
        "不休饥渴钨拉莫原初饥渴札卡玛祖神兽意志魔力脑力激荡达硌士的棺柩"
        "约格莫夫的坟墓乌尔博格卡恩的大创车多明纳里亚英雄泰菲力剑齿虎龙王安塔卡"
        "绝望终局伊莫库森林平原海岛沼泽山脉瞬间法术生物神器结界传奇"
        "人类法术师龙天使人鱼浪客元素化身士兵恐龙奥札奇长老猫九头蛇头像"
        "飞行警戒践踏延势不灭敏捷抓牌放逐重置横置生物反击弃牌牺牲指示物"
        "伤害目标牌手牌库墓地战场回合费用攻击防御忠诚度"
        "蓝牙配网微信搜索小程序设备找到选择密码连接当前状态等待连接正在连接"
        "已连接成功未分配离线网络本地系列按键返回浏览下一张切换视图抽牌长按"
        "英文原文双语对照热点网页手机家中目标正在等待路由器分配"
    )
    for ch in ui_keywords:
        all_cps.add(ord(ch))

    # 扩充万智牌中文全量卡牌名库
    zhs_names_path = Path("/Users/Zhuanz/CCDeep/mtg-primer-app/data/magic-cards-zhs-names.json")
    if zhs_names_path.exists():
        try:
            names_data = json.loads(zhs_names_path.read_text(encoding="utf-8"))
            for k, v in names_data.get("names", {}).items():
                for ch in v:
                    if ord(ch) > 127:
                        all_cps.add(ord(ch))
                        serif_cps.add(ord(ch))
        except Exception:
            pass

    # 全量扩充 GB2312 简体中文字库（共 6,763 个汉字：涵盖一级常用字 3755 + 二级次常用字 3008）
    # 彻底覆盖万智牌全牌池文本与网络随机抽取卡牌，彻底根除所有缺字方框！
    for b1 in range(0xB0, 0xF8):
        for b2 in range(0xA1, 0xFF):
            try:
                ch = bytes([b1, b2]).decode('gb2312')
                all_cps.add(ord(ch))
                serif_cps.add(ord(ch))
            except Exception:
                pass

    return sorted(all_cps), sorted(serif_cps)

def main():
    noto_font_path = Path("/private/tmp/folotoy-env/NotoSansSC.ttf")
    serif_font_path = Path("/private/tmp/folotoy-env/SourceHanSerifSC-Heavy.otf")
    mana_font_path = ROOT / "assets/fonts/mana.ttf"
    converter = Path("/private/tmp/folotoy-env/font-tools/node_modules/.bin/lv_font_conv")

    if not noto_font_path.exists():
        raise SystemExit(f"Noto font not found: {noto_font_path}")
    if not serif_font_path.exists():
        raise SystemExit(f"Serif font not found: {serif_font_path}")
    if not mana_font_path.exists():
        raise SystemExit(f"Mana font not found: {mana_font_path}")
    if not converter.exists():
        raise SystemExit(f"Converter not found: {converter}")

    all_cps, serif_cps = collect_codepoints()
    print(f"Collected {len(all_cps)} total codepoints.")
    print(f"Collected {len(serif_cps)} serif flavor codepoints.")
    print(f"Collected {len(MANA_CODEPOINTS)} mana symbol codepoints.")

    out_dir = ROOT / "assets/fonts"
    out_dir.mkdir(parents=True, exist_ok=True)

    from fontTools.ttLib import TTFont
    from fontTools.varLib.instancer import instantiateVariableFont

    with tempfile.TemporaryDirectory(prefix="mtg-font-") as tmp:
        regular_ttf = Path(tmp) / "NotoSansSC-Regular.ttf"
        instantiateVariableFont(TTFont(str(noto_font_path)), {'wght': 400}, inplace=True).save(str(regular_ttf))

        # 过滤字体中真正存在的码位，避免 BOM 等控制字符引起 lv_font_conv 报错
        serif_tt = TTFont(str(serif_font_path))
        serif_cmap = serif_tt.getBestCmap()
        valid_serif_cps = [c for c in serif_cps if c in serif_cmap and c != 0xfeff]

        noto_tt = TTFont(str(regular_ttf))
        noto_cmap = noto_tt.getBestCmap()
        valid_all_cps = [c for c in all_cps if c in noto_cmap and c != 0xfeff]

        mana_tt = TTFont(str(mana_font_path))
        mana_cmap = mana_tt.getBestCmap()
        valid_mana_cps = [c for c in MANA_CODEPOINTS if c in mana_cmap]

        print(f"Supported codepoints in Serif: {len(valid_serif_cps)}, Noto: {len(valid_all_cps)}, Mana: {len(valid_mana_cps)}")

        text_range_all = ",".join(hex(c) for c in valid_all_cps)
        text_range_serif = ",".join(hex(c) for c in valid_serif_cps)
        mana_range = ",".join(hex(c) for c in valid_mana_cps)

        # 1. 生成 mtg_font_serif_20.c (思源宋体 Heavy 大字重，20px，4bpp 抗锯齿)
        c_serif_out = out_dir / "mtg_font_serif_20.c"
        cmd_serif = [
            str(converter),
            "--font", str(serif_font_path),
            "--range", text_range_serif,
            "--font", str(mana_font_path),
            "--range", mana_range,
            "--size", "20",
            "--bpp", "4",
            "--format", "lvgl",
            "--no-compress",
            "--lv-font-name", "mtg_font_serif_20",
            "--lv-include", "lvgl.h",
            "--output", str(c_serif_out),
        ]
        print(f"Running lv_font_conv for Source Han Serif SC Heavy 20px (mtg_font_serif_20)...")
        subprocess.run(cmd_serif, check=True)

        # 2. 生成 mtg_font_16.c (Noto Sans SC 16px，4bpp 抗锯齿 + Mana 符号)
        c_16_out = out_dir / "mtg_font_16.c"
        cmd_16 = [
            str(converter),
            "--font", str(regular_ttf),
            "--range", text_range_all,
            "--font", str(mana_font_path),
            "--range", mana_range,
            "--size", "16",
            "--bpp", "4",
            "--format", "lvgl",
            "--no-compress",
            "--lv-font-name", "mtg_font_16",
            "--lv-include", "lvgl.h",
            "--output", str(c_16_out),
        ]
        print(f"Running lv_font_conv for Noto Sans SC 16px (mtg_font_16)...")
        subprocess.run(cmd_16, check=True)

    # 汇总清单
    all_combined = sorted(set(all_cps + serif_cps + MANA_CODEPOINTS))
    inv_out = out_dir / "mtg_font_inventory.c"
    inv_c = (
        '#include <stdint.h>\n'
        '#include <stddef.h>\n\n'
        'const uint32_t mtg_font_codepoints[] = {\n'
        + ", ".join(hex(c) for c in all_combined)
        + "\n};\n"
        f"const size_t mtg_font_codepoint_count = sizeof(mtg_font_codepoints) / sizeof(mtg_font_codepoints[0]);\n"
    )
    inv_out.write_text(inv_c, encoding="utf-8")

    manifest_out = out_dir / "mtg_font_manifest.json"
    manifest_data = {
        "fonts": ["Source Han Serif SC Heavy 20px", "Noto Sans SC 16px", "Mana Font 1.18.0"],
        "size_serif": 20,
        "size_sans": 16,
        "bpp": 4,
        "count_all": len(all_combined),
        "count_serif": len(serif_cps),
        "codepoints": all_combined,
    }
    manifest_out.write_text(json.dumps(manifest_data, indent=2), encoding="utf-8")
    print(f"Both fonts successfully generated: {c_serif_out} and {c_16_out}!")

if __name__ == "__main__":
    main()
