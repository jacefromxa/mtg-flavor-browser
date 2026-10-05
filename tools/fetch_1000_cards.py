#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fetch and build 1000 curated MTG cards with bilingual flavor text."""

import json
import os
import sys
import time
import urllib.request
import urllib.parse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# 导入现有的 150 张优质初始卡牌
sys.path.insert(0, str(ROOT / "tools"))
from build_offline_cards import CARDS as SEED_CARDS

def fetch_additional_cards(target_count=1000):
    all_cards = []
    seen_names = set()

    for c in SEED_CARDS:
        name_en = c["name_en"].strip().lower()
        if name_en not in seen_names:
            seen_names.add(name_en)
            all_cards.append(c)

    print(f"Loaded {len(all_cards)} seed cards.")

    page = 1
    headers = {"User-Agent": "MTGFlavorBrowser/1.0 (ESP32-C3 Firmware)"}

    # 查询条件：具有风味文本的助推包正规卡牌
    queries = ["is:booster", "order:edhrec", "order:released", "lang:zhs"]

    for q in queries:
        if len(all_cards) >= target_count:
            break
        print(f"Fetching with query: {q}...")
        for page in range(1, 30):
            if len(all_cards) >= target_count:
                break
            url = f"https://mtgch.com/api/v1/result?q={urllib.parse.quote(q)}&page_size=100&page={page}"
            try:
                req = urllib.request.Request(url, headers=headers)
                with urllib.request.urlopen(req, timeout=10) as resp:
                    data = json.loads(resp.read().decode("utf-8"))
            except Exception as e:
                print(f"Page {page} fetch error: {e}")
                time.sleep(1)
                continue

            items = data.get("items", [])
            if not items:
                break

            for it in items:
                if len(all_cards) >= target_count:
                    break

                name_en = (it.get("name") or "").strip()
                if not name_en or name_en.lower() in seen_names:
                    continue

                name_zh = (it.get("zhs_name") or it.get("atomic_official_name") or it.get("atomic_translated_name") or "").strip()
                if not name_zh:
                    continue

                f_zh = (it.get("zhs_flavor_text") or it.get("atomic_translated_flavor_text") or "").strip()
                if not f_zh:
                    continue

                f_en = (it.get("flavor_text") or "").strip()
                if not f_en:
                    continue

                mana = (it.get("mana_cost") or "").strip()
                type_line = (it.get("zhs_type_line") or it.get("atomic_translated_type") or it.get("type_line") or "").strip()
                oracle = (it.get("zhs_text") or it.get("atomic_translated_text") or it.get("oracle_text") or "").strip()
                set_code = (it.get("set") or "").upper().strip()

                pt = ""
                p = it.get("power")
                t = it.get("toughness")
                if p is not None and t is not None:
                    pt = f"{p}/{t}"

                author = (it.get("atomic_translated_flavor_name") or "").strip()
                if author and not author.startswith("——") and not author.startswith("—"):
                    author = "——" + author

                card = {
                    "name_zh": name_zh,
                    "name_en": name_en,
                    "mana_cost": mana,
                    "type_line": type_line,
                    "oracle_text": oracle,
                    "flavor_text": f_zh,
                    "flavor_en": f_en,
                    "flavor_author": author,
                    "pt": pt,
                    "set_code": set_code,
                }

                seen_names.add(name_en.lower())
                all_cards.append(card)

            print(f"Query {q} page {page}: Total collected = {len(all_cards)} / {target_count}")
            time.sleep(0.15)

    return all_cards[:target_count]

def escape_c_string(s):
    if not s:
        return '""'
    s = s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    return f'"{s}"'

def main():
    target = 1000
    cache_file = ROOT / "assets/cards_1000.json"
    if cache_file.exists():
        print(f"Loading cached cards from {cache_file}...")
        cards = json.loads(cache_file.read_text(encoding="utf-8"))
    else:
        cards = fetch_additional_cards(target)
        cache_file.parent.mkdir(parents=True, exist_ok=True)
        cache_file.write_text(json.dumps(cards, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"Cached {len(cards)} cards to {cache_file}.")

    print(f"Generating C code for {len(cards)} cards...")

    # 生成 main/mtg_cards.c
    c_path = ROOT / "main/mtg_cards.c"
    h_path = ROOT / "main/mtg_cards.h"

    h_content = f"""#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {{
#endif

typedef struct {{
    const char *name_zh;        /* 中文牌名 */
    const char *name_en;        /* 英文原名 */
    const char *mana_cost;      /* 法术力费用字符串，如 {{1}}{{U}}{{R}} */
    const char *type_line;      /* 类别，如 "生物 ～ 天使" */
    const char *oracle_text;    /* 规则叙述 */
    const char *flavor_text;    /* 风味文字 (中文) */
    const char *flavor_en;      /* 风味文字 (英文) */
    const char *flavor_author;  /* 引言出处/作者 */
    const char *pt;             /* 力量/防御力，如 "4/4" */
    const char *set_code;       /* 系列缩写，如 "LEA #161" */
}} mtg_card_t;

/* 初始化卡牌数据库与洗牌队列 */
void mtg_cards_init(uint32_t seed);

/* 获取卡牌库总数 */
size_t mtg_cards_count(void);

/* 按索引获取卡牌 (0 ~ count-1) */
const mtg_card_t *mtg_cards_get_by_index(size_t index);

/* 随机抽取一张卡牌 (基于无放回洗牌算法，保证每个完整周期内绝不重复) */
const mtg_card_t *mtg_cards_get_random(void);

#ifdef __cplusplus
}}
#endif
"""

    c_lines = [
        '#include "mtg_cards.h"',
        '#include <stdlib.h>',
        '#include <string.h>',
        '#ifdef ESP_PLATFORM',
        '#include "esp_random.h"',
        '#include "esp_log.h"',
        'static const char *TAG = "mtg_cards";',
        '#else',
        '#define esp_random() ((uint32_t)rand())',
        '#define ESP_LOGI(tag, fmt, ...)',
        '#endif',
        '',
        f'#define MTG_CARDS_TOTAL {len(cards)}',
        '',
        'static const mtg_card_t s_cards[MTG_CARDS_TOTAL] = {'
    ]

    for c in cards:
        c_lines.append("    {")
        c_lines.append(f"        .name_zh = {escape_c_string(c['name_zh'])},")
        c_lines.append(f"        .name_en = {escape_c_string(c['name_en'])},")
        c_lines.append(f"        .mana_cost = {escape_c_string(c['mana_cost'])},")
        c_lines.append(f"        .type_line = {escape_c_string(c['type_line'])},")
        c_lines.append(f"        .oracle_text = {escape_c_string(c['oracle_text'])},")
        c_lines.append(f"        .flavor_text = {escape_c_string(c['flavor_text'])},")
        c_lines.append(f"        .flavor_en = {escape_c_string(c['flavor_en'])},")
        c_lines.append(f"        .flavor_author = {escape_c_string(c['flavor_author'])},")
        c_lines.append(f"        .pt = {escape_c_string(c['pt'])},")
        c_lines.append(f"        .set_code = {escape_c_string(c['set_code'])},")
        c_lines.append("    },")

    c_lines.extend([
        '};',
        '',
        '/* 洗牌索引序列 */',
        'static uint16_t s_deck_order[MTG_CARDS_TOTAL];',
        'static size_t s_deck_pos = 0;',
        '',
        'static void shuffle_deck(uint32_t seed)',
        '{',
        '    if (seed != 0) {',
        '        srand(seed);',
        '    }',
        '    for (size_t i = 0; i < MTG_CARDS_TOTAL; i++) {',
        '        s_deck_order[i] = (uint16_t)i;',
        '    }',
        '    /* Fisher-Yates 洗牌算法 */',
        '    for (size_t i = MTG_CARDS_TOTAL - 1; i > 0; i--) {',
        '        uint32_t r = esp_random() % (i + 1);',
        '        uint16_t tmp = s_deck_order[i];',
        '        s_deck_order[i] = s_deck_order[r];',
        '        s_deck_order[r] = tmp;',
        '    }',
        '    s_deck_pos = 0;',
        '    ESP_LOGI(TAG, "已完成离线卡牌库 Fisher-Yates 洗牌，总计 %d 张，无重复循环", MTG_CARDS_TOTAL);',
        '}',
        '',
        'void mtg_cards_init(uint32_t seed)',
        '{',
        '    shuffle_deck(seed);',
        '}',
        '',
        'size_t mtg_cards_count(void)',
        '{',
        '    return MTG_CARDS_TOTAL;',
        '}',
        '',
        'const mtg_card_t *mtg_cards_get_by_index(size_t index)',
        '{',
        '    if (index >= MTG_CARDS_TOTAL) {',
        '        return &s_cards[0];',
        '    }',
        '    return &s_cards[index];',
        '}',
        '',
        'const mtg_card_t *mtg_cards_get_random(void)',
        '{',
        '    if (s_deck_pos >= MTG_CARDS_TOTAL) {',
        '        shuffle_deck(0);',
        '    }',
        '    uint16_t idx = s_deck_order[s_deck_pos++];',
        '    return &s_cards[idx];',
        '}',
        ''
    ])

    h_path.write_text(h_content, encoding="utf-8")
    c_path.write_text("\n".join(c_lines), encoding="utf-8")
    print(f"Generated {h_path} and {c_path} with {len(cards)} cards successfully!")

if __name__ == "__main__":
    main()
