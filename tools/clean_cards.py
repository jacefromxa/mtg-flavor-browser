#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Clean all 1000 cards in cards_1000.json and regenerate main/mtg_cards.c with clean strings."""

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def clean_card(c):
    ft = c.get('flavor_text') or ''
    fa = c.get('flavor_author') or ''
    fe = c.get('flavor_en') or ''
    ot = c.get('oracle_text') or ''

    # Normalize newlines
    ft = ft.replace('\\\\n', '\n').replace('\\n', '\n')
    fe = fe.replace('\\\\n', '\n').replace('\\n', '\n')
    ot = ot.replace('\\\\n', '\n').replace('\\n', '\n')

    # Detect author attribution in Chinese flavor text:
    # Matches \n～Author, \n~Author, \n——Author, \n—Author
    m_author = re.search(r'\n[～~———\-]+\s*(.+)$', ft)
    if m_author:
        extracted = m_author.group(1).strip()
        ft = ft[:m_author.start()].strip()
        if not fa:
            fa = f"——{extracted}"

    # Also detect if author is at end without newline: ~Author
    m_trailing = re.search(r'[～~]\s*([^\s]{2,16})$', ft)
    if m_trailing and not fa and len(ft) > 20:
        cand = m_trailing.group(1).strip()
        if not any(cand.endswith(x) for x in ['。', '！', '？', '」', '”', '…']):
            fa = f"——{cand}"
            ft = ft[:m_trailing.start()].strip()

    # Clean stray 'n~' or 'n～' or '\n' in Chinese flavor text
    ft = ft.replace('\n', ' ').strip()
    ft = re.sub(r'\s*n[～~]\s*', '', ft)
    ft = re.sub(r'([^0-9A-Za-z])～([^0-9A-Za-z])', r'\1——\2', ft)

    # Clean English flavor text
    # Detect author at end: e.g. \n—Author, \n- Author
    m_en_author = re.search(r'\n[—\-~]+\s*(.+)$', fe)
    if m_en_author:
        fe = fe[:m_en_author.start()].strip()
    fe = fe.replace('\n', ' ').strip()
    fe = re.sub(r'\s*n[～~]\s*', '', fe)

    # Standardize author format
    if fa:
        fa = fa.replace('\n', '').strip()
        fa = re.sub(r'^[～~———\-]+\s*', '——', fa)
        if not fa.startswith('——'):
            fa = f"——{fa}"

    c['flavor_text'] = ft
    c['flavor_en'] = fe
    c['flavor_author'] = fa
    c['oracle_text'] = ot
    return c

def escape_c_string(s):
    if not s:
        return '""'
    # Escape for C literal:
    # 1. Backslash -> \\
    # 2. Double quote -> \"
    # 3. Newline -> \n
    s = s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    return f'"{s}"'

def main():
    json_path = ROOT / "assets/cards_1000.json"
    cards = json.loads(json_path.read_text(encoding="utf-8"))

    cleaned_cards = [clean_card(c) for c in cards]

    # Save cleaned JSON
    json_path.write_text(json.dumps(cleaned_cards, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Updated {json_path} with {len(cleaned_cards)} cleaned cards.")

    # Generate main/mtg_cards.c
    c_path = ROOT / "main/mtg_cards.c"
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
        f'#define MTG_CARDS_TOTAL {len(cleaned_cards)}',
        '',
        'static const mtg_card_t s_cards[MTG_CARDS_TOTAL] = {'
    ]

    for c in cleaned_cards:
        c_lines.append("    {")
        c_lines.append(f"        .name_zh = {escape_c_string(c.get('name_zh', ''))},")
        c_lines.append(f"        .name_en = {escape_c_string(c.get('name_en', ''))},")
        c_lines.append(f"        .mana_cost = {escape_c_string(c.get('mana_cost', ''))},")
        c_lines.append(f"        .type_line = {escape_c_string(c.get('type_line', ''))},")
        c_lines.append(f"        .oracle_text = {escape_c_string(c.get('oracle_text', ''))},")
        c_lines.append(f"        .flavor_text = {escape_c_string(c.get('flavor_text', ''))},")
        c_lines.append(f"        .flavor_en = {escape_c_string(c.get('flavor_en', ''))},")
        c_lines.append(f"        .flavor_author = {escape_c_string(c.get('flavor_author', ''))},")
        c_lines.append(f"        .pt = {escape_c_string(c.get('pt', ''))},")
        c_lines.append(f"        .set_code = {escape_c_string(c.get('set_code', ''))},")
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
        '',
        'void mtg_cards_reshuffle(uint32_t seed)',
        '{',
        '    shuffle_deck(seed);',
        '}',
        ''
    ])

    c_path.write_text("\n".join(c_lines), encoding="utf-8")
    print(f"Generated {c_path} with {len(cleaned_cards)} cards successfully!")

if __name__ == "__main__":
    main()
