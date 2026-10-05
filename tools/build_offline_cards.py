#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate rich 150-card offline database for MTG Flavor Browser with bilingual flavor texts."""

import os
import sys

CARDS = [
    {
        "name_zh": "闪电击",
        "name_en": "Lightning Bolt",
        "mana_cost": "{R}",
        "type_line": "瞬间",
        "oracle_text": "闪电击对任意一个目标造成3点伤害。",
        "flavor_text": "“思考电光，你便成灰。”",
        "flavor_en": "\"Think lightning, and you are ash.\"",
        "flavor_author": "——多明纳里亚谚语",
        "pt": "",
        "set_code": "LEA #161"
    },
    {
        "name_zh": "神之愤怒",
        "name_en": "Wrath of God",
        "mana_cost": "{2}{W}{W}",
        "type_line": "法术",
        "oracle_text": "消灭所有生物。它们不能重生。",
        "flavor_text": "“若无仁慈，万物尽归虚无。”",
        "flavor_en": "\"Without mercy, all creation falls to dust.\"",
        "flavor_author": "——教会圣典",
        "pt": "",
        "set_code": "4ED #60"
    },
    {
        "name_zh": "塞拉天使",
        "name_en": "Serra Angel",
        "mana_cost": "{3}{W}{W}",
        "type_line": "生物 ～ 天使",
        "oracle_text": "飞行，警戒",
        "flavor_text": "“生于纯白信仰，以剑与羽翼捍卫正义之光。”",
        "flavor_en": "\"Born with wings of light and a sword of purity, she is the defender of the innocent.\"",
        "flavor_author": "——塞拉圣典",
        "pt": "4/4",
        "set_code": "LEA #39"
    },
    {
        "name_zh": "剑击归农",
        "name_en": "Swords to Plowshares",
        "mana_cost": "{W}",
        "type_line": "瞬间",
        "oracle_text": "放逐目标生物。其操控者获得等同于其力量的生命。",
        "flavor_text": "“将怒火熔铸为耕具，让战火平息于沃野。”",
        "flavor_en": "\"The smallest deed of peace outshines the greatest conquest of war.\"",
        "flavor_author": "——农事谚语",
        "pt": "",
        "set_code": "LEA #42"
    },
    {
        "name_zh": "放逐之径",
        "name_en": "Path to Exile",
        "mana_cost": "{W}",
        "type_line": "瞬间",
        "oracle_text": "放逐目标生物。其操控者可以从其牌库搜寻一张基本地牌并横置进战场。",
        "flavor_text": "“不要在此停留，这片土地不属于你的罪孽。”",
        "flavor_en": "\"There is a place for all things, but this realm is no longer yours.\"",
        "flavor_author": "——班特结界师",
        "pt": "",
        "set_code": "CFX #15"
    },
    {
        "name_zh": "大修道士艾蕾侬",
        "name_en": "Elesh Norn, Grand Cenobite",
        "mana_cost": "{5}{W}{W}",
        "type_line": "传奇生物 ～ 执政官",
        "oracle_text": "警戒\n由你操控的其他生物得+2/+2。\n由对手操控的生物得-2/-2。",
        "flavor_text": "“凡肉躯皆有欠缺，唯白瓷方得纯洁永生。”",
        "flavor_en": "\"The Gitaxians whisper among themselves, but all will be unified beneath our white porcelain.\"",
        "flavor_author": "——艾蕾侬",
        "pt": "4/7",
        "set_code": "NPH #9"
    },
    {
        "name_zh": "艾斯波哨兵",
        "name_en": "Esper Sentinel",
        "mana_cost": "{W}",
        "type_line": "神器生物 ～ 人类／士兵",
        "oracle_text": "每当任一对手施放其每回合第一个非生物咒语时，除非该牌手支付{X}，否则你抓一张牌，X为该哨兵的力量。",
        "flavor_text": "“每有一记不谐音符，金属的低鸣便洞察分毫。”",
        "flavor_en": "\"Every discordant pulse echoes across the etherium grid.\"",
        "flavor_author": "——艾斯波观测录",
        "pt": "1/1",
        "set_code": "MH2 #12"
    },
    {
        "name_zh": "日冠赫利欧德",
        "name_en": "Heliod, Sun-Crowned",
        "mana_cost": "{2}{W}",
        "type_line": "传奇结界生物 ～ 神",
        "oracle_text": "不灭\n只要你的献力小于五，它便不是生物。\n每当你获得生命时，在目标生物或结界上放置一个+1/+1指示物。",
        "flavor_text": "“太阳升起时，一切凡人的狂妄皆化作暗淡虚影。”",
        "flavor_en": "\"When the sun ascends, the hollow boasts of mortals vanish in blinding light.\"",
        "flavor_author": "——尼兹神话",
        "pt": "5/5",
        "set_code": "THB #18"
    },
    {
        "name_zh": "旭日泰坦",
        "name_en": "Sun Titan",
        "mana_cost": "{4}{W}{W}",
        "type_line": "生物 ～ 巨人",
        "oracle_text": "警戒\n每当旭日泰坦进战场或攻击时，你可以将目标法术力值小于等于3的永久物牌从你的坟墓场移回战场。",
        "flavor_text": "“旭日之下，被遗忘的誓言重焕生机。”",
        "flavor_en": "\"A radiant blaze of justice that resurrects fallen oaths from the soil.\"",
        "flavor_author": "——班特战歌",
        "pt": "6/6",
        "set_code": "M11 #35"
    },
    {
        "name_zh": "白骑士",
        "name_en": "White Knight",
        "mana_cost": "{W}{W}",
        "type_line": "生物 ～ 人类／骑士",
        "oracle_text": "先攻，反黑保护",
        "flavor_text": "“只向纯洁屈膝，面对邪恶绝不后退一步。”",
        "flavor_en": "\"Out of the blackness, holy light; out of the light, an unyielding sword.\"",
        "flavor_author": "——骑士团信条",
        "pt": "2/2",
        "set_code": "LEA #43"
    },
    {
        "name_zh": "封口",
        "name_en": "Silence",
        "mana_cost": "{W}",
        "type_line": "瞬间",
        "oracle_text": "本回合中，你的对手不能施放咒语。",
        "flavor_text": "“静止比言语更有力量，沉默即是至高的权威。”",
        "flavor_en": "\"Quiet is greater than rhetoric, and peace is loudest in solitude.\"",
        "flavor_author": "——阿喀洛斯戒律",
        "pt": "",
        "set_code": "M10 #31"
    },
    {
        "name_zh": "平衡",
        "name_en": "Balance",
        "mana_cost": "{1}{W}",
        "type_line": "法术",
        "oracle_text": "各牌手弃掉手牌，直到每位手牌数与拥有最少手牌者相等；然后以同样方式牺牲地和生物。",
        "flavor_text": "“在天秤两端，凡人与君王等重。”",
        "flavor_en": "\"Nature demands symmetry; justice delivers equality.\"",
        "flavor_author": "——法官箴言",
        "pt": "",
        "set_code": "LEA #3"
    },
    {
        "name_zh": "坚贞大天使艾维欣",
        "name_en": "Archangel Avacyn",
        "mana_cost": "{3}{W}{W}",
        "type_line": "传奇生物 ～ 天使",
        "oracle_text": "闪现，飞行，警戒\n当艾维欣进战场时，由你操控的永久物获得不灭直到回合结束。",
        "flavor_text": "“一道坚如磐石的金色黎明，撕破依尼翠永恒的长夜。”",
        "flavor_en": "\"A golden dawn breaks across the moors, shattering Innistrad's endless gloom.\"",
        "flavor_author": "——月主教颂词",
        "pt": "4/4",
        "set_code": "SOI #5"
    },
    {
        "name_zh": "太阳之宠艾紫培",
        "name_en": "Elspeth, Sun's Champion",
        "mana_cost": "{4}{W}{W}",
        "type_line": "传奇旅法师 ～ 艾紫培",
        "oracle_text": "+1：派出三个1/1白色士兵衍生生物。\n-3：消灭所有力量大于或等于4的生物。\n-7：你获得具有飞行与+2/+2的徽记。",
        "flavor_text": "“哪怕被诸神背叛，我仍将为每一个无辜者拔剑。”",
        "flavor_en": "\"Though abandoned by gods, she draws steel for the weak.\"",
        "flavor_author": "——塞洛斯民谣",
        "pt": "[4]",
        "set_code": "THS #9"
    },
    {
        "name_zh": "祖神恩泽",
        "name_en": "Ancestral Mask",
        "mana_cost": "{2}{W}",
        "type_line": "结界 ～ 结附于生物",
        "oracle_text": "战场上每有一个其他结界，佩带此牌的生物便得+2/+2。",
        "flavor_text": "“千百代先祖的面容注视着你，他们的意志赋予你无穷铠甲。”",
        "flavor_en": "\"Countless forebears watch through the mask, their ancient valor woven into iron.\"",
        "flavor_author": "——克撒传",
        "pt": "",
        "set_code": "MMQ #229"
    },
    {
        "name_zh": "崇圣地",
        "name_en": "Sacred Ground",
        "mana_cost": "{1}{W}",
        "type_line": "结界",
        "oracle_text": "每当由对手操控的咒语或异能导致由你操控的地置入坟墓场时，将该地牌移回战场。",
        "flavor_text": "“凡人可践踏青草，但大地的根基永远受神圣庇护。”",
        "flavor_en": "\"Mortals may scorch the field, but the soil itself is guarded by heaven.\"",
        "flavor_author": "——塞拉誓约",
        "pt": "",
        "set_code": "STH #112"
    },
    {
        "name_zh": "坚毅大天使琳法拉",
        "name_en": "Linvala, Keeper of Silence",
        "mana_cost": "{2}{W}{W}",
        "type_line": "传奇生物 ～ 天使",
        "oracle_text": "飞行\n由对手操控的生物之起动异能都不能起动。",
        "flavor_text": "“喧嚣暂歇，世界在纯粹的宁静中聆听裁决。”",
        "flavor_en": "\"No wing flaps, no spell whispers; tranquility falls like a hammer.\"",
        "flavor_author": "——赞迪卡铭文",
        "pt": "3/4",
        "set_code": "ROE #33"
    },
    {
        "name_zh": "崇高大天使",
        "name_en": "Sublime Archangel",
        "mana_cost": "{2}{W}{W}",
        "type_line": "生物 ～ 天使",
        "oracle_text": "飞行，颂威\n由你操控的其他生物均具有颂威异能。",
        "flavor_text": "“所有目光凝视于一刃，所有希望寄托于一人。”",
        "flavor_en": "\"One champion carries the righteous will of a thousand faithful hearts.\"",
        "flavor_author": "——阿托卡骑士团",
        "pt": "4/3",
        "set_code": "M13 #36"
    },
    {
        "name_zh": "遗忘轮",
        "name_en": "Oblivion Ring",
        "mana_cost": "{2}{W}",
        "type_line": "结界",
        "oracle_text": "当遗忘轮进战场时，放逐另一个目标非地永久物，直到遗忘轮离开战场为止。",
        "flavor_text": "“金色光环锁住岁月，凡入其中者既不在生，亦不在死。”",
        "flavor_en": "\"A ring of golden amber holds them between past and forgotten future.\"",
        "flavor_author": "——洛温寓言",
        "pt": "",
        "set_code": "LRW #34"
    },
    {
        "name_zh": "黎明使者雷亚",
        "name_en": "Reya Dawnbringer",
        "mana_cost": "{6}{W}{W}{W}",
        "type_line": "传奇生物 ～ 天使",
        "oracle_text": "飞行\n在你的维持开始时，你可以将目标生物牌从你的坟墓场移回战场。",
        "flavor_text": "“哪怕最深沉的黑暗，也会在她的羽翼前化作晨曦。”",
        "flavor_en": "\"Life returns at her arrival, casting away the cold slumber of the grave.\"",
        "flavor_author": "——入侵史诗",
        "pt": "4/6",
        "set_code": "INV #32"
    },
    {
        "name_zh": "纯净光辉",
        "name_en": "Radiant Purge",
        "mana_cost": "{1}{W}",
        "type_line": "瞬间",
        "oracle_text": "放逐目标多色生物或多色结界。",
        "flavor_text": "“唯有最单纯的信念，能斩断纷繁纠缠的宿怨。”",
        "flavor_en": "\"Unfiltered truth burns away the tangled deceit of many-colored ambition.\"",
        "flavor_author": "——鞑契战记",
        "pt": "",
        "set_code": "DTK #31"
    },
    {
        "name_zh": "反击咒语",
        "name_en": "Counterspell",
        "mana_cost": "{U}{U}",
        "type_line": "瞬间",
        "oracle_text": "反击目标咒语。",
        "flavor_text": "“不。”",
        "flavor_en": "\"It was probably a bad idea anyway. No.\"",
        "flavor_author": "——艾拉达力",
        "pt": "",
        "set_code": "TMP #57"
    },
    {
        "name_zh": "时间漫步",
        "name_en": "Time Walk",
        "mana_cost": "{1}{U}",
        "type_line": "法术",
        "oracle_text": "在这一回合后，你进行额外的一个回合。",
        "flavor_text": "“昨日如水流逝，唯智者截断波澜。”",
        "flavor_en": "\"A single stride across the river of moments, changing fate forever.\"",
        "flavor_author": "——时间学者",
        "pt": "",
        "set_code": "LEA #85"
    },
    {
        "name_zh": "心灵塑识师杰斯",
        "name_en": "Jace, the Mind Sculptor",
        "mana_cost": "{2}{U}{U}",
        "type_line": "传奇旅法师 ～ 杰斯",
        "oracle_text": "+2：检视目标牌手牌库顶牌。\n0：抓三张牌，然后置回两张。\n-1：将目标生物移回其拥有者手牌。\n-12：放逐目标牌手牌库所有牌。",
        "flavor_text": "“你所有的秘密，在我眼前不过是敞开的卷轴。”",
        "flavor_en": "\"All your secrets are but an open scroll before my sight.\"",
        "flavor_author": "——杰斯·贝连",
        "pt": "[3]",
        "set_code": "WWK #31"
    },
    {
        "name_zh": "脑力激荡",
        "name_en": "Brainstorm",
        "mana_cost": "{U}",
        "type_line": "瞬间",
        "oracle_text": "抓三张牌，然后将你手牌中的两张牌以任意顺序置于你的牌库顶。",
        "flavor_text": "“思绪如电光石火，于刹那间洞悉三千世界。”",
        "flavor_en": "\"A sudden strike of genius turns defeat into flawless victory.\"",
        "flavor_author": "——冰雪时代",
        "pt": "",
        "set_code": "ICE #61"
    },
    {
        "name_zh": "意志魔力",
        "name_en": "Force of Will",
        "mana_cost": "{3}{U}{U}",
        "type_line": "瞬间",
        "oracle_text": "你可以支付1点生命并从你的手上放逐一张蓝色牌，而不用支付此咒语的法术力费用。\n反击目标咒语。",
        "flavor_text": "“我以此念，断绝你的万千妄想。”",
        "flavor_en": "\"I deny your reality and substitute my own unbroken purpose.\"",
        "flavor_author": "——意志法师",
        "pt": "",
        "set_code": "ALL #28"
    },
    {
        "name_zh": "沉思",
        "name_en": "Ponder",
        "mana_cost": "{U}",
        "type_line": "法术",
        "oracle_text": "检视你牌库顶的三张牌，然后以任意顺序放回。你可以洗牌。抓一张牌。",
        "flavor_text": "“未来有千百条道路，而我正在挑选最优雅的一条。”",
        "flavor_en": "\"Tomorrow belongs to those who prepare for its arrival today.\"",
        "flavor_author": "——欧祝泰学者",
        "pt": "",
        "set_code": "LRW #79"
    },
    {
        "name_zh": "旋风裂隙",
        "name_en": "Cyclonic Rift",
        "mana_cost": "{1}{U}",
        "type_line": "瞬间",
        "oracle_text": "将目标非地永久物移回其拥有者手上。超载{6}{U}。",
        "flavor_text": "“狂风将卷走一切不属于风暴的尘埃。”",
        "flavor_en": "\"The ocean does not negotiate with the shore when fury stirs the deep.\"",
        "flavor_author": "——伊捷工匠",
        "pt": "",
        "set_code": "RTR #35"
    },
    {
        "name_zh": "月智者多美代",
        "name_en": "Tamiyo, the Moon Sage",
        "mana_cost": "{3}{U}{U}",
        "type_line": "传奇旅法师 ～ 多美代",
        "oracle_text": "+1：横置目标永久物，其操控者下一个重置步骤中不能重置。\n-2：抓等同于目标牌手操控横置生物数量的牌。\n-8：获得拥有无限手牌且牌张入坟场即移回手牌的徽记。",
        "flavor_text": "“月光如墨，静述万千世界之兴亡。”",
        "flavor_en": "\"The moon's light tells the rise and fall of countless distant worlds.\"",
        "flavor_author": "——多美代画卷",
        "pt": "[4]",
        "set_code": "AVR #79"
    },
    {
        "name_zh": "全知全能",
        "name_en": "Omniscience",
        "mana_cost": "{7}{U}{U}{U}",
        "type_line": "结界",
        "oracle_text": "你可以从你手上施放法术力咒语，且不需支付其法术力费用。",
        "flavor_text": "“当你通晓一切，万象皆遵你号令。”",
        "flavor_en": "\"The universe whispers its deepest truths to those who listen without pride.\"",
        "flavor_author": "——秘术圣卷",
        "pt": "",
        "set_code": "M13 #63"
    },
    {
        "name_zh": "先制地脉",
        "name_en": "Leyline of Anticipation",
        "mana_cost": "{2}{U}{U}",
        "type_line": "结界",
        "oracle_text": "如果此牌在你的起手手牌中，你可以在游戏开始时将其置入战场。\n你可以将非地咒语视同具有闪现异能来施放。",
        "flavor_text": "“在第一个念头诞生前，结局早已写定。”",
        "flavor_en": "\"Before the first spark catches, the final tapestry is already woven.\"",
        "flavor_author": "——地脉秘史",
        "pt": "",
        "set_code": "M11 #61"
    },
    {
        "name_zh": "真名宿敌",
        "name_en": "True-Name Nemesis",
        "mana_cost": "{1}{U}{U}",
        "type_line": "生物 ～ 人鱼／浪客",
        "oracle_text": "于真名宿敌进战场时，选择一位牌手。\n真名宿敌具有反所选牌手保护。",
        "flavor_text": "“知晓其名者，方知何为不可阻挡的宿命。”",
        "flavor_en": "\"To speak the true name of the deep is to bind fate itself to your will.\"",
        "flavor_author": "——梅洛克古训",
        "pt": "3/1",
        "set_code": "C13 #63"
    },
    {
        "name_zh": "冰霜泰坦",
        "name_en": "Frost Titan",
        "mana_cost": "{4}{U}{U}",
        "type_line": "生物 ～ 巨人",
        "oracle_text": "每当冰霜泰坦成为咒语或异能的目标时，除非操控者支付{2}，否则反击之。\n进战场或攻击时横置目标永久物且下回合不重置。",
        "flavor_text": "“严冬漫步于大地，脚步所至，万物陷入长眠。”",
        "flavor_en": "\"Winter's icy step stills the pulse of the warmest heart.\"",
        "flavor_author": "——极地编年史",
        "pt": "6/6",
        "set_code": "M11 #55"
    },
    {
        "name_zh": "神秘导师",
        "name_en": "Mystical Tutor",
        "mana_cost": "{U}",
        "type_line": "瞬间",
        "oracle_text": "从你的牌库搜寻一张瞬间或法术牌并展示，然后洗牌并将该牌置于牌库顶。",
        "flavor_text": "“智慧不在于懂得一切，而在于知晓在何时何处寻觅。”",
        "flavor_en": "\"Wisdom lies not in holding every answer, but in knowing where truth waits.\"",
        "flavor_author": "——陶拉里亚院规",
        "pt": "",
        "set_code": "MIR #80"
    },
    {
        "name_zh": "操控记忆杰斯",
        "name_en": "Jace, Memory Adept",
        "mana_cost": "{3}{U}{U}",
        "type_line": "传奇旅法师 ～ 杰斯",
        "oracle_text": "+1：抓一张牌。目标牌手将其牌库顶牌置入坟墓场。\n0：目标牌手将其牌库顶十张牌置入坟墓场。\n-7：任意数量的目标牌手各抓二十张牌。",
        "flavor_text": "“记忆如沙砾，只需轻吹一口气，便消散无踪。”",
        "flavor_en": "\"Memories are dust in the wind, easily scattered by a single thought.\"",
        "flavor_author": "——杰斯·贝连",
        "pt": "[4]",
        "set_code": "M12 #58"
    },
    {
        "name_zh": "溯寻宝船",
        "name_en": "Treasure Cruise",
        "mana_cost": "{7}{U}",
        "type_line": "法术",
        "oracle_text": "掘穴（你施放此咒语时，每从坟墓场放逐一张牌，便能为此咒语支付{1}。）\n抓三张牌。",
        "flavor_text": "“航向过去的余烬，换回无尽的知识。”",
        "flavor_en": "\"Through the wreckage of yesterday sails the vessel of tomorrow.\"",
        "flavor_author": "——苏勒台航海志",
        "pt": "",
        "set_code": "KTK #59"
    },
    {
        "name_zh": "知识渴求",
        "name_en": "Thirst for Knowledge",
        "mana_cost": "{2}{U}",
        "type_line": "瞬间",
        "oracle_text": "抓三张牌。然后除非你弃掉一张神器牌，否则弃掉两张牌。",
        "flavor_text": "“头脑如无底深渊，知识越深，饥渴愈烈。”",
        "flavor_en": "\"Curiosity is a fire that feeds upon the books it burns.\"",
        "flavor_author": "——秘罗地学者",
        "pt": "",
        "set_code": "MRD #59"
    },
    {
        "name_zh": "探寻秘密",
        "name_en": "Delver of Secrets",
        "mana_cost": "{U}",
        "type_line": "生物 ～ 人类／法术师",
        "oracle_text": "在你的维持开始时，检视你的牌库顶牌。若为瞬间或法术牌，则展示并转化它。",
        "flavor_text": "“每一次探究未知，都在向不可名状的蜕变迈近一步。”",
        "flavor_en": "\"Curiosity is the first fragile step down the path to monstrous truth.\"",
        "flavor_author": "——依尼翠笔记",
        "pt": "1/1",
        "set_code": "ISD #51"
    },
    {
        "name_zh": "水没废墟",
        "name_en": "Tolarian Academy",
        "mana_cost": "",
        "type_line": "传奇地",
        "oracle_text": "{T}：你操控每有一个神器，便加{U}。",
        "flavor_text": "“在这片被时间撕裂的海域，天才与毁灭并肩沉睡。”",
        "flavor_en": "\"Here time fractured, leaving brilliant minds asleep beneath crystal waves.\"",
        "flavor_author": "——克撒纪事",
        "pt": "",
        "set_code": "USG #330"
    },
    {
        "name_zh": "黑暗祭礼",
        "name_en": "Dark Ritual",
        "mana_cost": "{B}",
        "type_line": "瞬间",
        "oracle_text": "加{B}{B}{B}。",
        "flavor_text": "“来自幽冥的低语，以暗影编织命运。”",
        "flavor_en": "\"Whispers from the deep, weaving fate in shadow and blood.\"",
        "flavor_author": "——约格莫夫的门徒",
        "pt": "",
        "set_code": "LEA #98"
    },
    {
        "name_zh": "面纱的莉莲娜",
        "name_en": "Liliana of the Veil",
        "mana_cost": "{1}{B}{B}",
        "type_line": "传奇旅法师 ～ 莉莲娜",
        "oracle_text": "+1：每位牌手各弃一张牌。\n-2：目标牌手牺牲一个生物。\n-6：将目标牌手操控的永久物分为两份，由其选择一份牺牲。",
        "flavor_text": "“死亡并非终点，而是我号令的开始。”",
        "flavor_en": "\"Death is not the end, merely the beginning of your obedience to me.\"",
        "flavor_author": "——莉莲娜·维斯",
        "pt": "[3]",
        "set_code": "ISD #105"
    },
    {
        "name_zh": "思绪倾轧",
        "name_en": "Thoughtseize",
        "mana_cost": "{B}",
        "type_line": "法术",
        "oracle_text": "目标牌手展示其手牌。你从中选择一张非地牌。该牌手弃掉该牌。你失去2点生命。",
        "flavor_text": "“最深沉的恐惧不是死亡，而是被看透的软弱。”",
        "flavor_en": "\"Any mind is an open gate to one who knows which key to turn.\"",
        "flavor_author": "——洛温寓言",
        "pt": "",
        "set_code": "LRW #145"
    },
    {
        "name_zh": "生灵终结",
        "name_en": "Damnation",
        "mana_cost": "{2}{B}{B}",
        "type_line": "法术",
        "oracle_text": "消灭所有生物。它们不能重生。",
        "flavor_text": "“它们祈求日光，却迎来了黑夜。”",
        "flavor_en": "\"They prayed for mercy, but the sky answered with ash and silence.\"",
        "flavor_author": "——时空混沌",
        "pt": "",
        "set_code": "PLC #63"
    },
    {
        "name_zh": "送上终点",
        "name_en": "Fatal Push",
        "mana_cost": "{B}",
        "type_line": "瞬间",
        "oracle_text": "消灭目标法术力值小于或等于2的生物。反抗——如果你本回合有永久物离开战场，则改为小于等于4。",
        "flavor_text": "“只需轻轻一推，巨人亦将跌入深渊。”",
        "flavor_en": "\"The higher they climb, the sweeter the fall from grace.\"",
        "flavor_author": "——反抗军信条",
        "pt": "",
        "set_code": "AER #57"
    },
    {
        "name_zh": "黑暗亲信",
        "name_en": "Dark Confidant",
        "mana_cost": "{1}{B}",
        "type_line": "生物 ～ 人类／法术师",
        "oracle_text": "在你的维持开始时，展示你牌库顶牌并置入手牌。你失去等同于其法术力值的生命。",
        "flavor_text": "“伟大总有代价，而这代价通常是你的性命。”",
        "flavor_en": "\"Greatness, at any cost.\"",
        "flavor_author": "——鲍勃·马哈尔",
        "pt": "2/1",
        "set_code": "RAV #81"
    },
    {
        "name_zh": "恶魔导师",
        "name_en": "Demonic Tutor",
        "mana_cost": "{1}{B}",
        "type_line": "法术",
        "oracle_text": "从你的牌库搜寻一张牌并置入手牌，然后洗牌。",
        "flavor_text": "“契约早已立下，只需你用灵魂签上名字。”",
        "flavor_en": "\"Every wish carries an invoice written in brimstone and blood.\"",
        "flavor_author": "——地狱盟约",
        "pt": "",
        "set_code": "LEA #104"
    },
    {
        "name_zh": "毒气阻遏",
        "name_en": "Toxic Deluge",
        "mana_cost": "{2}{B}",
        "type_line": "法术",
        "oracle_text": "作为施放此咒语的额外费用，支付X点生命。\n所有生物得-X/-X直到回合结束。",
        "flavor_text": "“酸雨倾盆，冲刷掉一切骄傲与皮肉。”",
        "flavor_en": "\"Pestilence knows no allegiance; it dissolves flesh without distinction.\"",
        "flavor_author": "——指挥官典籍",
        "pt": "",
        "set_code": "C13 #96"
    },
    {
        "name_zh": "约格莫夫的意志",
        "name_en": "Yawgmoth's Will",
        "mana_cost": "{2}{B}",
        "type_line": "法术",
        "oracle_text": "直到回合结束，你可以从你的坟墓场施放牌。本回合置入你坟墓场的牌改为放逐。",
        "flavor_text": "“死亡并非安息，而是重归大宗师锻炉的熔渣。”",
        "flavor_en": "\"The dead do not rest; they fuel the glorious machine of Phyrexia.\"",
        "flavor_author": "——非瑞克西亚圣约",
        "pt": "",
        "set_code": "USG #171"
    },
    {
        "name_zh": "墓场泰坦",
        "name_en": "Grave Titan",
        "mana_cost": "{4}{B}{B}",
        "type_line": "生物 ～ 巨人",
        "oracle_text": "死触\n每当墓场泰坦进战场或攻击时，派出两个2/2黑色丧尸衍生生物。",
        "flavor_text": "“在死者的浪潮前，孤军奋战不过是徒增枯骨。”",
        "flavor_en": "\"Death does not arrive alone; it marches with an endless legion.\"",
        "flavor_author": "——冥府战鼓",
        "pt": "6/6",
        "set_code": "M11 #97"
    },
    {
        "name_zh": "严酷导师",
        "name_en": "Grim Tutor",
        "mana_cost": "{1}{B}{B}",
        "type_line": "法术",
        "oracle_text": "从你的牌库搜寻一张牌并置入手牌，然后洗牌。你失去3点生命。",
        "flavor_text": "“痛苦是最耐心的老师，它的每一堂课都刻骨铭心。”",
        "flavor_en": "\"Pain teaches lessons that prosperity can never impart.\"",
        "flavor_author": "——暗黑箴言",
        "pt": "",
        "set_code": "S99 #75"
    },
    {
        "name_zh": "终结",
        "name_en": "Terminate",
        "mana_cost": "{B}{R}",
        "type_line": "瞬间",
        "oracle_text": "消灭目标生物。它不能重生。",
        "flavor_text": "“像掐灭蜡烛一样，简单，彻底。”",
        "flavor_en": "\"Like extinguishing a flicker in an icy draft—swift, silent, complete.\"",
        "flavor_author": "——散漫客信条",
        "pt": "",
        "set_code": "PLS #128"
    },
    {
        "name_zh": "活尸法",
        "name_en": "Animate Dead",
        "mana_cost": "{1}{B}",
        "type_line": "结界 ～ 灵气",
        "oracle_text": "将目标生物牌从坟墓场移入战场，并结附于其上。该生物得-1/-0。",
        "flavor_text": "“哪怕灵魂早已消散，骨骸亦需听令起舞。”",
        "flavor_en": "\"Though the spirit flies, the hollow bone must dance to dark command.\"",
        "flavor_author": "——通灵术秘典",
        "pt": "",
        "set_code": "LEA #92"
    },
    {
        "name_zh": "贪婪暴君棘泽边",
        "name_en": "Griselbrand",
        "mana_cost": "{4}{B}{B}{B}{B}",
        "type_line": "传奇生物 ～ 恶魔",
        "oracle_text": "飞行，系命\n支付7点生命：抓七张牌。",
        "flavor_text": "“地狱深渊中的帝王，七即是其力量与诅咒的烙印。”",
        "flavor_en": "\"A demon emperor who bargains only with blood and absolute ruin.\"",
        "flavor_author": "——依尼翠末日预言",
        "pt": "7/7",
        "set_code": "AVR #106"
    },
    {
        "name_zh": "非瑞克西亚抹煞兽",
        "name_en": "Phyrexian Obliterator",
        "mana_cost": "{B}{B}{B}{B}",
        "type_line": "生物 ～ 恐怖兽",
        "oracle_text": "践踏\n每当任一来源对非瑞克西亚抹煞兽造成伤害时，该来源的操控者牺牲等量的永久物。",
        "flavor_text": "“反抗只是加速毁灭的借口。”",
        "flavor_en": "\"Resistance merely hastens the inevitable disintegration of all you love.\"",
        "flavor_author": "——新非瑞克西亚",
        "pt": "5/5",
        "set_code": "NPH #68"
    },
    {
        "name_zh": "希欧朵拉",
        "name_en": "Sheoldred, the Apocalypse",
        "mana_cost": "{2}{B}{B}",
        "type_line": "传奇生物 ～ 魔判官",
        "oracle_text": "死触\n每当你抓一张牌时，你获得2点生命。\n每当任一对手抓一张牌时，该牌手失去2点生命。",
        "flavor_text": "“每一次呼吸都在向终局倒数，无处可逃。”",
        "flavor_en": "\"Every breath drawn brings you closer to the glorious silence of the void.\"",
        "flavor_author": "——启示录卷轴",
        "pt": "4/5",
        "set_code": "DMU #107"
    },
    {
        "name_zh": "恐怖",
        "name_en": "Terror",
        "mana_cost": "{1}{B}",
        "type_line": "瞬间",
        "oracle_text": "消灭目标非神器且非黑色的生物。它不能重生。",
        "flavor_text": "“黑暗不仅存在于阴影中，它潜伏在你的每一次心跳里。”",
        "flavor_en": "\"Darkness lives not in the shadows, but inside every quickening pulse.\"",
        "flavor_author": "——狂乱低语",
        "pt": "",
        "set_code": "LEA #130"
    },
    {
        "name_zh": "约格莫夫的坟墓乌尔博格",
        "name_en": "Urborg, Tomb of Yawgmoth",
        "mana_cost": "",
        "type_line": "传奇地",
        "oracle_text": "每个地均额外是沼泽类别。",
        "flavor_text": "“邪秽之源，纵使死者沉沦，其毒依旧蔓延四方。”",
        "flavor_en": "\"Where Yawgmoth fell, darkness festers and black bile drowns the fertile soil.\"",
        "flavor_author": "——克撒研究录",
        "pt": "",
        "set_code": "PLC #165"
    },
    {
        "name_zh": "西瓦巨龙",
        "name_en": "Shivan Dragon",
        "mana_cost": "{4}{R}{R}",
        "type_line": "生物 ～ 龙",
        "oracle_text": "飞行\n{R}：西瓦巨龙得+1/+0直到回合结束。",
        "flavor_text": "“统治希瓦山脉的霸主，它的怒火融化磐石。”",
        "flavor_en": "\"The undisputed master of the Shivan mountains, whose breath melts basalt.\"",
        "flavor_author": "——多明纳里亚史诗",
        "pt": "5/5",
        "set_code": "LEA #173"
    },
    {
        "name_zh": "反抗烈炬茜卓",
        "name_en": "Chandra, Torch of Defiance",
        "mana_cost": "{2}{R}{R}",
        "type_line": "传奇旅法师 ～ 茜卓",
        "oracle_text": "+1：放逐牌库顶牌，你可以施放它；否则对每位对手造成2点伤害。\n+1：加{R}{R}。\n-3：对目标生物造成4点伤害。\n-7：获得每当你施放咒语造成5点伤害的徽记。",
        "flavor_text": "“规则是用来打破的，如同火焰吞噬一切桎梏。”",
        "flavor_en": "\"Rules are dry kindling waiting for the glorious sparks of rebellion.\"",
        "flavor_author": "——茜卓·纳拉",
        "pt": "[4]",
        "set_code": "KLD #110"
    },
    {
        "name_zh": "鬼怪向导",
        "name_en": "Goblin Guide",
        "mana_cost": "{R}",
        "type_line": "生物 ～ 鬼怪／斥候",
        "oracle_text": "敏捷\n每当鬼怪向导攻击时，防御牌手展示其牌库顶牌。若为地牌，则置入手牌。",
        "flavor_text": "“他认得路，只是不知通往宝藏还是通往陷阱。”",
        "flavor_en": "\"He knows the territory well—though neither he nor you know where it leads.\"",
        "flavor_author": "——阿库姆探险队",
        "pt": "2/2",
        "set_code": "ZEN #126"
    },
    {
        "name_zh": "盛欢幻灵",
        "name_en": "Eidolon of the Great Revel",
        "mana_cost": "{R}{R}",
        "type_line": "结界生物 ～ 灵气",
        "oracle_text": "每当任一牌手施放法术力值小于或等于3的咒语时，盛欢幻灵对该牌手造成2点伤害。",
        "flavor_text": "“狂欢之火灼烧灵魂，沉醉者不知皮肉已焦。”",
        "flavor_en": "\"In Xenagos's feast, the dancers revel until their very marrow is ash.\"",
        "flavor_author": "——塞洛斯狂宴",
        "pt": "2/2",
        "set_code": "JOU #94"
    },
    {
        "name_zh": "鲜血之月",
        "name_en": "Blood Moon",
        "mana_cost": "{2}{R}",
        "type_line": "结界",
        "oracle_text": "非基本地均为山脉。",
        "flavor_text": "“血色苍穹之下，一切奇迹归于原始的荒芜。”",
        "flavor_en": "\"Under the crimson haze, towering citadels crumble into raw brimstone.\"",
        "flavor_author": "——暗黑时代",
        "pt": "",
        "set_code": "DRK #59"
    },
    {
        "name_zh": "炼狱泰坦",
        "name_en": "Inferno Titan",
        "mana_cost": "{4}{R}{R}",
        "type_line": "生物 ～ 巨人",
        "oracle_text": "{R}：炼狱泰坦得+1/+0直到回合结束。\n进战场或攻击时，造成共3点伤害，分配给任意目标。",
        "flavor_text": "“当大地开裂，熔岩铸成的身躯践踏一切防御。”",
        "flavor_en": "\"Molten rock heeds no armor, and wildfire takes no prisoners.\"",
        "flavor_author": "——火山口铭文",
        "pt": "6/6",
        "set_code": "M11 #146"
    },
    {
        "name_zh": "混沌扭曲",
        "name_en": "Chaos Warp",
        "mana_cost": "{2}{R}",
        "type_line": "瞬间",
        "oracle_text": "目标永久物的拥有者将其洗入其牌库，然后展示其牌库顶牌。若为永久物牌，则置入战场。",
        "flavor_text": "“在狂乱的虚空中，哪怕最精密的秩序也不过是一掷骰子。”",
        "flavor_en": "\"Into the maelstrom of entropy goes your champion; who knows what emerges?\"",
        "flavor_author": "——乱术师格言",
        "pt": "",
        "set_code": "CMD #120"
    },
    {
        "name_zh": "碎颅击",
        "name_en": "Skullcrack",
        "mana_cost": "{1}{R}",
        "type_line": "瞬间",
        "oracle_text": "本回合中，牌手不能获得生命，且伤害不能被防止。碎颅击对目标牌手造成3点伤害。",
        "flavor_text": "“你的祷告或许崇高，但我的拳头更快。”",
        "flavor_en": "\"Your prayers are poetic, but my hammer reaches the target first.\"",
        "flavor_author": "——古鲁战吼",
        "pt": "",
        "set_code": "GTC #106"
    },
    {
        "name_zh": "烈火断层",
        "name_en": "Fireblast",
        "mana_cost": "{4}{R}{R}",
        "type_line": "瞬间",
        "oracle_text": "你可以牺牲两个山脉，而不用支付此咒语的法术力费用。\n烈火断层对任意一个目标造成4点伤害。",
        "flavor_text": "“大地燃烧，山脉化为愤怒的熔火长河。”",
        "flavor_en": "\"Sacrifice the bedrock today to incinerate the enemy tomorrow.\"",
        "flavor_author": "——维苏瓦火山史",
        "pt": "",
        "set_code": "VIS #79"
    },
    {
        "name_zh": "敏捷得胜鬼怪拉卡凡",
        "name_en": "Ragavan, Nimble Pilferer",
        "mana_cost": "{R}",
        "type_line": "传奇生物 ～ 猴子／浪客",
        "oracle_text": "敏捷\n对牌手造成战斗伤害时派出珍宝并放逐其牌库顶牌，你可以施放该牌。疾冲{1}{R}。",
        "flavor_text": "“只要你一眨眼，不仅钱袋空了，连压箱底的咒语也易主了。”",
        "flavor_en": "\"A flash of orange fur, a missing ring, and your prized artifact is gone.\"",
        "flavor_author": "——卡拉德许市集传闻",
        "pt": "2/1",
        "set_code": "MH2 #138"
    },
    {
        "name_zh": "雷喉残虐者",
        "name_en": "Thundermaw Hellkite",
        "mana_cost": "{3}{R}{R}",
        "type_line": "生物 ～ 龙",
        "oracle_text": "飞行，敏捷\n当雷喉残虐者进战场时，对由对手操控的每个具飞行异能的生物造成1点伤害并横置之。",
        "flavor_text": "“雷暴咆哮之处，苍穹之上的飞鸟唯有坠落。”",
        "flavor_en": "\"When thunderstorm wears wings of brass, lesser fliers tumble like rain.\"",
        "flavor_author": "——龙谷风暴志",
        "pt": "5/5",
        "set_code": "M13 #150"
    },
    {
        "name_zh": "惹事暴民",
        "name_en": "Bedlam Reveler",
        "mana_cost": "{6}{R}{R}",
        "type_line": "生物 ～ 恶魔",
        "oracle_text": "坟墓场中每有一张瞬间或法术牌，便减少{1}施放费用。\n进战场时弃掉手牌并抓三张牌。",
        "flavor_text": "“如果世界已然陷入混乱，何不与火共舞？”",
        "flavor_en": "\"When madness reigns supreme, the only sane choice is to fuel the bonfire.\"",
        "flavor_author": "——依尼翠邪宴",
        "pt": "3/4",
        "set_code": "EMN #118"
    },
    {
        "name_zh": "熔岩钉",
        "name_en": "Lava Spike",
        "mana_cost": "{R}",
        "type_line": "法术 ～ 古咒",
        "oracle_text": "熔岩钉对目标牌手造成3点伤害。",
        "flavor_text": "“地心深处的怒火凝固成尖锐的刺，直贯咽喉。”",
        "flavor_en": "\"A spear of pure geological wrath that strikes straight at the soul.\"",
        "flavor_author": "——神河物语",
        "pt": "",
        "set_code": "CHK #178"
    },
    {
        "name_zh": "炽烈火球",
        "name_en": "Fireball",
        "mana_cost": "{X}{R}",
        "type_line": "法术",
        "oracle_text": "火球对任意数量的目标造成共X点伤害，每个额外目标需多支付{1}。",
        "flavor_text": "“一道弧光划破天际，之后唯有寂静与灰烬。”",
        "flavor_en": "\"A flash of light across the twilight, followed by ash and burning silence.\"",
        "flavor_author": "——古法术讲义",
        "pt": "",
        "set_code": "LEA #153"
    },
    {
        "name_zh": "愤怒巨灵",
        "name_en": "Anger",
        "mana_cost": "{3}{R}",
        "type_line": "生物 ～ 化身",
        "oracle_text": "敏捷\n只要愤怒巨灵在你的坟墓场中且你操控山脉，由你操控的生物便具有敏捷异能。",
        "flavor_text": "“即便肉身已化为尘埃，他的狂怒仍在血脉中奔流。”",
        "flavor_en": "\"Though flesh returns to dirt, raw indignation burns in the marrow forever.\"",
        "flavor_author": "——审判预言",
        "pt": "2/2",
        "set_code": "JUD #85"
    },
    {
        "name_zh": "巨木吞噬怪",
        "name_en": "Gigantosaurus",
        "mana_cost": "{G}{G}{G}{G}{G}",
        "type_line": "生物 ～ 恐龙",
        "oracle_text": "（它拥有无与伦比的庞大身躯。）",
        "flavor_text": "“每一脚步皆是地震，每一呼吸皆是风暴。”",
        "flavor_en": "\"Each step is an earthquake, each breath a hurricane across the canopy.\"",
        "flavor_author": "——烈阳帝国传记",
        "pt": "10/10",
        "set_code": "M19 #185"
    },
    {
        "name_zh": "巨力成长",
        "name_en": "Giant Growth",
        "mana_cost": "{G}",
        "type_line": "瞬间",
        "oracle_text": "目标生物得+3/+3直到回合结束。",
        "flavor_text": "“即使是最矮小的杂草，亦能遮天蔽日。”",
        "flavor_en": "\"Even the humblest blade of grass can blot out the midday sun.\"",
        "flavor_author": "——林间古谚",
        "pt": "",
        "set_code": "LEA #195"
    },
    {
        "name_zh": "极乐鸟",
        "name_en": "Birds of Paradise",
        "mana_cost": "{G}",
        "type_line": "生物 ～ 鸟",
        "oracle_text": "飞行\n{T}：加一点任意颜色的法术力。",
        "flavor_text": "“带上微风的翅膀，将色彩洒向苍穹。”",
        "flavor_en": "\"Borne on gentle breezes, their plumage paints the dawn in vibrant hue.\"",
        "flavor_author": "——多明纳里亚自然史",
        "pt": "0/1",
        "set_code": "LEA #186"
    },
    {
        "name_zh": "撼地妮莎",
        "name_en": "Nissa, Who Shakes the World",
        "mana_cost": "{3}{G}{G}",
        "type_line": "传奇旅法师 ～ 妮莎",
        "oracle_text": "每当你横置一个森林产生法术力时，额外产生{G}。\n+1：将目标地变为3/3警戒生物并重置。\n-8：从牌库搜寻所有基本地置入战场且具不灭。",
        "flavor_text": "“大地的脉动在我脚下觉醒，山河皆为战旗。”",
        "flavor_en": "\"The earth whispers of forgotten might, and I lend it my thundering voice.\"",
        "flavor_author": "——妮莎·瑞文",
        "pt": "[5]",
        "set_code": "WAR #169"
    },
    {
        "name_zh": "植林泰坦",
        "name_en": "Primeval Titan",
        "mana_cost": "{4}{G}{G}",
        "type_line": "生物 ～ 巨人",
        "oracle_text": "践踏\n每当植林泰坦进战场或攻击时，你可以从你的牌库搜寻最多两张地牌并横置置入战场。",
        "flavor_text": "“当巨神行走在荒野，山峦随之升起，河流应声改道。”",
        "flavor_en": "\"When giants stride the untamed wild, peaks arise and rivers bow.\"",
        "flavor_author": "——赞迪卡神话",
        "pt": "6/6",
        "set_code": "M11 #192"
    },
    {
        "name_zh": "破土巨角兽",
        "name_en": "Craterhoof Behemoth",
        "mana_cost": "{5}{G}{G}{G}",
        "type_line": "生物 ～ 兽",
        "oracle_text": "敏捷\n当破土巨角兽进战场时，由你操控的生物获得践踏与+X/+X直到回合结束，X为你操控的生物数量。",
        "flavor_text": "“兽群冲锋之处，大地为之哀鸣。”",
        "flavor_en": "\"Where the thundering herd charges, the very mantle of the plane fractures.\"",
        "flavor_author": "——加渥尼狩猎录",
        "pt": "5/5",
        "set_code": "AVR #172"
    },
    {
        "name_zh": "活旺力",
        "name_en": "Vigor",
        "mana_cost": "{3}{G}{G}{G}",
        "type_line": "生物 ～ 化身",
        "oracle_text": "践踏\n若将对由你操控的其他生物造成伤害，改为在上面放置等量的+1/+1指示物。",
        "flavor_text": "“生命在每一次击打中愈发繁茂强韧。”",
        "flavor_en": "\"Every wound feeds the roots of life, blooming stronger with every blow.\"",
        "flavor_author": "——罗堰祭司",
        "pt": "6/6",
        "set_code": "LRW #249"
    },
    {
        "name_zh": "黑莲花",
        "name_en": "Black Lotus",
        "mana_cost": "{0}",
        "type_line": "神器",
        "oracle_text": "{T}，牺牲黑莲花：加三点任意单色法术力。",
        "flavor_text": "“其香清绝，其力倾国。”",
        "flavor_en": "\"Its scent outshines the sun, its power shapes the destiny of the planes.\"",
        "flavor_author": "——旅法师日志",
        "pt": "",
        "set_code": "LEA #232"
    },
    {
        "name_zh": "索蓝发电机",
        "name_en": "Thran Dynamo",
        "mana_cost": "{4}",
        "type_line": "神器",
        "oracle_text": "{T}：加{C}{C}{C}。",
        "flavor_text": "“远古的轰鸣跨越万年，仍跳动如初生之心。”",
        "flavor_en": "\"Power that outlasted its creators, humming with primal vigor.\"",
        "flavor_author": "——机械纪元",
        "pt": "",
        "set_code": "UDS #139"
    },
    {
        "name_zh": "乙太精瓶",
        "name_en": "Aether Vial",
        "mana_cost": "{1}",
        "type_line": "神器",
        "oracle_text": "在你的维持开始时，可以在其上放置一个蓄能指示物。\n{T}：从你手牌将一张法术力值等同于蓄能指示物数量的生物牌置入战场。",
        "flavor_text": "“浓缩的乙太在玻璃瓶中激荡，静待打破宁静的契机。”",
        "flavor_en": "\"Within the fragile glass coils pure raw potential, waiting to explode.\"",
        "flavor_author": "——秘罗地工坊",
        "pt": "",
        "set_code": "DST #91"
    },
    {
        "name_zh": "扑翼机",
        "name_en": "Ornithopter",
        "mana_cost": "{0}",
        "type_line": "神器生物 ～ 振翼机",
        "oracle_text": "飞行",
        "flavor_text": "“凭借木材与黄铜的轻巧骨架，凡人亦可触摸云霄。”",
        "flavor_en": "\"On fragile wings of brass and linen, mortals chase the soaring hawk.\"",
        "flavor_author": "——古发明天才",
        "pt": "0/2",
        "set_code": "ATQ #59"
    },
    {
        "name_zh": "穿髓金针",
        "name_en": "Pithing Needle",
        "mana_cost": "{1}",
        "type_line": "神器",
        "oracle_text": "当穿髓金针进战场时，选择一个牌名。\n具有该名称的来源之起动异能都不能起动，法术力异能除外。",
        "flavor_text": "“一根细微的金针，足以封锁最狂暴的巨力。”",
        "flavor_en": "\"A thread of polished silver silences the roaring titan.\"",
        "flavor_author": "——十会盟卷宗",
        "pt": "",
        "set_code": "SOK #158"
    },
    {
        "name_zh": "祖神兽",
        "name_en": "Progenitus",
        "mana_cost": "{W}{W}{U}{U}{B}{B}{R}{R}{G}{G}",
        "type_line": "传奇生物 ～ 九头蛇／头像",
        "oracle_text": "反一切保护\n如果祖神兽将从任何区域置入坟墓场，改为展示它并洗入其拥有者的牌库。",
        "flavor_text": "“世界之魂，万物起源，目光所至神魔退避。”",
        "flavor_en": "\"The soul of the world has awakened, and gods bow in silence.\"",
        "flavor_author": "——阿拉若启示录",
        "pt": "10/10",
        "set_code": "CON #121"
    },
    {
        "name_zh": "不休饥渴钨拉莫",
        "name_en": "Ulamog, the Ceaseless Hunger",
        "mana_cost": "{10}",
        "type_line": "传奇生物 ～ 奥札奇",
        "oracle_text": "当你施放此咒语时，放逐目标两个永久物。\n不灭\n每当钨拉莫攻击时，防御牌手放逐其牌库顶二十张牌。",
        "flavor_text": "“虚空之胃永不餍足，凡触及者化为白垩。”",
        "flavor_en": "\"A ceaseless maw consuming the foundations of memory and stone.\"",
        "flavor_author": "——奥札奇纪元",
        "pt": "10/10",
        "set_code": "BFZ #15"
    },
    {
        "name_zh": "绝望终局伊莫库",
        "name_en": "Emrakul, the Promised End",
        "mana_cost": "{13}",
        "type_line": "传奇生物 ～ 奥札奇",
        "oracle_text": "你坟墓场中每有一种牌类别，便减少{1}费用。\n施放时操控目标对手下一个回合。\n飞行，践踏，反瞬间保护。",
        "flavor_text": "“疯狂乃是唯一的真理，我们皆是提线木偶。”",
        "flavor_en": "\"Even madness is a mercy against the incomprehensible abyss.\"",
        "flavor_author": "——依尼翠残卷",
        "pt": "13/13",
        "set_code": "EMN #6"
    },
    {
        "name_zh": "解放的卡恩",
        "name_en": "Karn Liberated",
        "mana_cost": "{7}",
        "type_line": "传奇旅法师 ～ 卡恩",
        "oracle_text": "+4：目标牌手放逐一张手牌。\n-3：放逐目标永久物。\n-14：重开一局游戏，将所有以此法放逐的非灵气永久物置入你的战场。",
        "flavor_text": "“时间会抚平创伤，但我将抚平时间。”",
        "flavor_en": "\"I can remake this world from iron and silver, and I will.\"",
        "flavor_author": "——卡恩",
        "pt": "[6]",
        "set_code": "NPH #1"
    },
    {
        "name_zh": "多明纳里亚英雄泰菲力",
        "name_en": "Teferi, Hero of Dominaria",
        "mana_cost": "{3}{W}{U}",
        "type_line": "传奇旅法师 ～ 泰菲力",
        "oracle_text": "+1：抓一张牌，在你的结束步骤重置两个地。\n-3：将目标非地永久物置入其拥有者牌库顶数来第三张。\n-8：获得每当你抓牌放逐目标永久物的徽记。",
        "flavor_text": "“慢下来，时光在此刻听我号令。”",
        "flavor_en": "\"Time is a tapestry, and I hold the needle that mends history.\"",
        "flavor_author": "——泰菲力",
        "pt": "[4]",
        "set_code": "DOM #207"
    },
    {
        "name_zh": "原初饥渴札卡玛",
        "name_en": "Zacama, Primal Calamity",
        "mana_cost": "{6}{R}{G}{W}",
        "type_line": "传奇生物 ～ 恐龙",
        "oracle_text": "警戒，延势，践踏\n施放进战场时重置所有你操控的地。\n{2}{R}：对生物打3点。\n{2}{G}：消灭结界或神器。\n{2}{W}：获得3点生命。",
        "flavor_text": "“三首齐啸，天地重回混沌初开之刻。”",
        "flavor_en": "\"Three heads roar, shaking the world to its primordial foundations.\"",
        "flavor_author": "——烈阳帝国赞美诗",
        "pt": "9/9",
        "set_code": "RIX #174"
    },
    {
        "name_zh": "龙王安塔卡",
        "name_en": "Dragonlord Atarka",
        "mana_cost": "{5}{R}{G}",
        "type_line": "传奇生物 ～ 龙长老",
        "oracle_text": "飞行，践踏\n当龙王安塔卡进战场时，对由你对手操控的生物或旅法师造成共5点伤害。",
        "flavor_text": "“饥荒是她的冠冕，烈火是她的言辞。”",
        "flavor_en": "\"Her hunger is the only law the frozen tundra respects.\"",
        "flavor_author": "——鞑契龙史",
        "pt": "8/8",
        "set_code": "DTK #216"
    },
    {
        "name_zh": "残酷通牒",
        "name_en": "Cruel Ultimatum",
        "mana_cost": "{U}{U}{B}{B}{B}{R}{R}",
        "type_line": "法术",
        "oracle_text": "目标对手牺牲生物，弃三张牌，失去5点生命；你移回坟墓场生物牌，抓三张牌，获得5点生命。",
        "flavor_text": "“献出你的力量，献出你的仆从，献出你的呼吸。”",
        "flavor_en": "\"There is no cruelty too great for those who stand beneath the dragon's shadow.\"",
        "flavor_author": "——尼可波拉斯",
        "pt": "",
        "set_code": "ALA #164"
    },
    {
        "name_zh": "闪电螺旋",
        "name_en": "Lightning Helix",
        "mana_cost": "{R}{W}",
        "type_line": "瞬间",
        "oracle_text": "闪电螺旋对任意一个目标造成3点伤害，你获得3点生命。",
        "flavor_text": "“怒火与虔信交织成纯白电光，诛灭敌寇，抚平创伤。”",
        "flavor_en": "\"Wrath strikes like iron; grace restores like rain.\"",
        "flavor_author": "——波洛斯信条",
        "pt": "",
        "set_code": "RAV #213"
    },
    {
        "name_zh": "灰烬同盟",
        "name_en": "Vindicate",
        "mana_cost": "{1}{W}{B}",
        "type_line": "法术",
        "oracle_text": "消灭目标永久物。",
        "flavor_text": "“不要否认结局，接受你的命运。”",
        "flavor_en": "\"Some problems cannot be reasoned with; they must be cleanly erased.\"",
        "flavor_author": "——葛拉西斯",
        "pt": "",
        "set_code": "APC #126"
    },
    {
        "name_zh": "血辫妖精",
        "name_en": "Bloodbraid Elf",
        "mana_cost": "{2}{R}{G}",
        "type_line": "生物 ～ 妖精／狂战士",
        "oracle_text": "敏捷，倾曳（施放此咒语时，放逐牌库顶牌直到放逐一张法术力值更低的非地牌，你可以免费施放它。）",
        "flavor_text": "“冲锋在前，混乱在后，旋风随她呼啸。”",
        "flavor_en": "\"She charges with storm winds at her back and wild fortune in her blades.\"",
        "flavor_author": "——勇德战歌",
        "pt": "3/2",
        "set_code": "ARB #50"
    },
    {
        "name_zh": "平原",
        "name_en": "Plains",
        "mana_cost": "",
        "type_line": "基本地 ～ 平原",
        "oracle_text": "{T}：加{W}。",
        "flavor_text": "“无垠日光洒遍沃野，晨风带来希望之息。”",
        "flavor_en": "\"Endless light warms the soil and brings renewed dawn across the steppe.\"",
        "flavor_author": "——班特日出",
        "pt": "",
        "set_code": "DOM #250"
    },
    {
        "name_zh": "海岛",
        "name_en": "Island",
        "mana_cost": "",
        "type_line": "基本地 ～ 海岛",
        "oracle_text": "{T}：加{U}。",
        "flavor_text": "“潮汐起伏，藏尽千古深邃与变幻莫测。”",
        "flavor_en": "\"The tide ebbs and flows, carrying centuries of mystery in every crest.\"",
        "flavor_author": "——拉尼卡水系",
        "pt": "",
        "set_code": "DOM #254"
    },
    {
        "name_zh": "沼泽",
        "name_en": "Swamp",
        "mana_cost": "",
        "type_line": "基本地 ～ 沼泽",
        "oracle_text": "{T}：加{B}。",
        "flavor_text": "“一切生灵终将归于泥泞，重归宁静。”",
        "flavor_en": "\"All that breathes returns to mire, and in the quiet rests forever.\"",
        "flavor_author": "——非瑞克西亚信条",
        "pt": "",
        "set_code": "DOM #258"
    },
    {
        "name_zh": "山脉",
        "name_en": "Mountain",
        "mana_cost": "",
        "type_line": "基本地 ～ 山脉",
        "oracle_text": "{T}：加{R}。",
        "flavor_text": "“岩浆奔涌之处，怒火永不熄灭。”",
        "flavor_en": "\"Where magma flows unbridled, the fury never sleeps.\"",
        "flavor_author": "——希瓦火山咏叹",
        "pt": "",
        "set_code": "DOM #263"
    },
    {
        "name_zh": "森林",
        "name_en": "Forest",
        "mana_cost": "",
        "type_line": "基本地 ～ 森林",
        "oracle_text": "{T}：加{G}。",
        "flavor_text": "“古木参天，生命在宁静中繁衍不息。”",
        "flavor_en": "\"Ancient trunks support the sky, nurturing life in eternal serenity.\"",
        "flavor_author": "——多明纳里亚林海",
        "pt": "",
        "set_code": "DOM #267"
    },
    {
        "name_zh": "荒原",
        "name_en": "Wasteland",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "{T}：加{C}。\n{T}，牺牲荒原：消灭目标非基本地。",
        "flavor_text": "“曾经繁华的城邦，终沦为风蚀黄沙。”",
        "flavor_en": "\"The grandest marble palaces inevitably crumble into dust under patient winds.\"",
        "flavor_author": "——暴风雨志",
        "pt": "",
        "set_code": "TMP #311"
    },
    {
        "name_zh": "雾漫雨林",
        "name_en": "Misty Rainforest",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "{T}，支付1点生命，牺牲雾漫雨林：从你的牌库搜寻一张森林或海岛牌并置入战场，然后洗牌。",
        "flavor_text": "“浓雾缭绕的树冠下，水流与枝蔓亲密无间。”",
        "flavor_en": "\"Where humid vapors meet leafy shadows, life carves new channels through stone.\"",
        "flavor_author": "——赞迪卡测绘者",
        "pt": "",
        "set_code": "ZEN #220"
    },
    {
        "name_zh": "幽静",
        "name_en": "Solitude",
        "mana_cost": "{3}{W}{W}",
        "type_line": "生物 ～ 化身",
        "oracle_text": "闪现，系命\n当幽静进战场时，放逐最多一个目标生物。其操控者获得等同于其力量的生命。转化：放逐一张白色手牌。",
        "flavor_text": "“在至纯的寂静之中，所有喧嚣与恶意归于空无。”",
        "flavor_en": "\"In the purest quiet, all strife and malice dissolve into the infinite expanse.\"",
        "flavor_author": "——元素圣歌",
        "pt": "3/2",
        "set_code": "MH2 #32"
    },
    {
        "name_zh": "盲目顺从",
        "name_en": "Blind Obedience",
        "mana_cost": "{1}{W}",
        "type_line": "结界",
        "oracle_text": "敲诈\n由对手操控的神器与生物横置进战场。",
        "flavor_text": "“规矩不仅写在纸上，更烙印在每一位信徒的膝盖骨里。”",
        "flavor_en": "\"The law is not just written on stone, but etched into every compliant heart.\"",
        "flavor_author": "——欧佐夫教令",
        "pt": "",
        "set_code": "GTC #6"
    },
    {
        "name_zh": "圣洁之日",
        "name_en": "Holy Day",
        "mana_cost": "{W}",
        "type_line": "瞬间",
        "oracle_text": "防止本回合中将造成的所有战斗伤害。",
        "flavor_text": "“利剑归鞘，即便仇敌亦在神圣之光下同享安息。”",
        "flavor_en": "\"Steel returns to sheath, and even bitter enemies share silent grace.\"",
        "flavor_author": "——和平日祷词",
        "pt": "",
        "set_code": "LEG #190"
    },
    {
        "name_zh": "救赎天使",
        "name_en": "Angel of Grace",
        "mana_cost": "{3}{W}{W}",
        "type_line": "生物 ～ 天使",
        "oracle_text": "闪现，飞行\n当救赎天使进战场时，直到回合结束，伤害不会使你的生命值降至1以下。",
        "flavor_text": "“只要希望尚存一线，绝望便无法夺走你的呼吸。”",
        "flavor_en": "\"As long as one ember of hope glimmers, the dark cannot swallow your soul.\"",
        "flavor_author": "——拉尼卡献礼",
        "pt": "5/4",
        "set_code": "RNA #1"
    },
    {
        "name_zh": "神圣葬送",
        "name_en": "Hallowed Burial",
        "mana_cost": "{3}{W}{W}",
        "type_line": "法术",
        "oracle_text": "所有生物的拥有者将它们置于其牌库底。",
        "flavor_text": "“没有鲜血，没有尸骸，只有漫漫长路的寂静回音。”",
        "flavor_en": "\"No spilled blood, no weeping bones; only the soft rustle of returning journeys.\"",
        "flavor_author": "——晨光诗篇",
        "pt": "",
        "set_code": "EVE #7"
    },
    {
        "name_zh": "迅咒法师",
        "name_en": "Snapcaster Mage",
        "mana_cost": "{1}{U}",
        "type_line": "生物 ～ 人类／法术师",
        "oracle_text": "闪现\n当迅咒法师进战场时，目标你坟墓场中的瞬间或法术牌获得返照异能直到回合结束。",
        "flavor_text": "“刹那的回忆，在手中化为致命的第二道闪电。”",
        "flavor_en": "\"An echo of magic is often more lethal than the original song.\"",
        "flavor_author": "——蒂亚戈·陈",
        "pt": "2/1",
        "set_code": "ISD #78"
    },
    {
        "name_zh": "精神误判",
        "name_en": "Mental Misstep",
        "mana_cost": "{U/P}",
        "type_line": "瞬间",
        "oracle_text": "（{U/P}可用{U}或2点生命支付。）\n反击目标法术力值为1的咒语。",
        "flavor_text": "“仅仅一个细微念头的偏差，便倾覆了宏伟的谋划。”",
        "flavor_en": "\"A single misstep of the mind brings down the greatest architecture of thought.\"",
        "flavor_author": "——金吉厦泰",
        "pt": "",
        "set_code": "NPH #38"
    },
    {
        "name_zh": "严苛窒息",
        "name_en": "Stifle",
        "mana_cost": "{U}",
        "type_line": "瞬间",
        "oracle_text": "反击目标触发异能或起动异能。",
        "flavor_text": "“如果计划在开始前就夭折，那么过程便毫无意义。”",
        "flavor_en": "\"If the seed never sprouts, the forest need never fear the axe.\"",
        "flavor_author": "——逆转咒师",
        "pt": "",
        "set_code": "SCG #52"
    },
    {
        "name_zh": "大法师的护符",
        "name_en": "Archmage's Charm",
        "mana_cost": "{U}{U}{U}",
        "type_line": "瞬间",
        "oracle_text": "选择一项：\n• 反击目标非地咒语。\n• 目标牌手抓两张牌。\n• 获得目标法术力值小于等于1的非地永久物之操控权。",
        "flavor_text": "“真正的法术大师从不拘泥于一种解答。”",
        "flavor_en": "\"True mastery lies in seeing three solutions where others find only deadlock.\"",
        "flavor_author": "——托拉利雅信条",
        "pt": "",
        "set_code": "MH1 #40"
    },
    {
        "name_zh": "索术师",
        "name_en": "Spellseeker",
        "mana_cost": "{2}{U}",
        "type_line": "生物 ～ 人类／法术师",
        "oracle_text": "当索术师进战场时，你可以从你的牌库搜寻一张法术力值小于等于2的瞬间或法术牌并展示置入手牌。",
        "flavor_text": "“在千万行咒文的海洋中，她总能捞起最锋芒毕露的一句。”",
        "flavor_en": "\"In an ocean of forgotten script, she finds the solitary phrase that cuts deepest.\"",
        "flavor_author": "——秘术学院",
        "pt": "1/2",
        "set_code": "BBD #41"
    },
    {
        "name_zh": "悲恸",
        "name_en": "Grief",
        "mana_cost": "{2}{B}{B}",
        "type_line": "生物 ～ 化身",
        "oracle_text": "威慑\n当悲恸进战场时，目标对手展示手牌，你从中选择一张非地牌弃掉。转化：放逐一张黑色手牌。",
        "flavor_text": "“悲伤不是眼泪，而是心口一块慢慢死去的坚冰。”",
        "flavor_en": "\"Grief is not a flood of tears, but cold stone settling quietly into the chest.\"",
        "flavor_author": "——暗夜低鸣",
        "pt": "3/2",
        "set_code": "MH2 #87"
    },
    {
        "name_zh": "恶魔契约",
        "name_en": "Demonic Pact",
        "mana_cost": "{2}{B}{B}",
        "type_line": "结界",
        "oracle_text": "在你的维持开始时，从未被选择的项目中选择一项：打4点伤害；抓两张牌；弃牌；输掉这局游戏。",
        "flavor_text": "“最后一行条款总是用最纤细的红墨水写成。”",
        "flavor_en": "\"The final clause is always inked in red, signed before you even read the rest.\"",
        "flavor_author": "——莉莲娜之锁",
        "pt": "",
        "set_code": "ORI #92"
    },
    {
        "name_zh": "黑日当空",
        "name_en": "Black Sun's Zenith",
        "mana_cost": "{X}{B}{B}",
        "type_line": "法术",
        "oracle_text": "在每个生物上各放置X个-1/-1指示物。将黑日当空洗入其拥有者的牌库。",
        "flavor_text": "“黑曜石之光照耀之地，万物枯萎消瘦。”",
        "flavor_en": "\"Beneath the obsidian sun, flesh shrivels and strength wanes into shadow.\"",
        "flavor_author": "——新非瑞克西亚纪元",
        "pt": "",
        "set_code": "MBS #40"
    },
    {
        "name_zh": "虚空地脉",
        "name_en": "Leyline of the Void",
        "mana_cost": "{2}{B}{B}",
        "type_line": "结界",
        "oracle_text": "起手手牌时可免费进战场。\n如果对手的牌将置入其坟墓场，改为放逐之。",
        "flavor_text": "“在这里，连死亡也是奢侈的幻想，唯有虚无。”",
        "flavor_en": "\"Here, even the peace of the tomb is denied; all things fall into timeless nothingness.\"",
        "flavor_author": "——暗黑地脉",
        "pt": "",
        "set_code": "GPT #52"
    },
    {
        "name_zh": "阴谋团仪式",
        "name_en": "Cabal Ritual",
        "mana_cost": "{1}{B}",
        "type_line": "瞬间",
        "oracle_text": "加{B}{B}{B}。门限——如果你坟墓场有七张或更多牌，改为加{B}{B}{B}{B}{B}。",
        "flavor_text": "“当死者呼喊时，暗影将赐予翻倍的力量。”",
        "flavor_en": "\"When the dead sing together, the shadows swell with unstoppable might.\"",
        "flavor_author": "——阴谋团祭司",
        "pt": "",
        "set_code": "TOR #51"
    },
    {
        "name_zh": "狂怒",
        "name_en": "Fury",
        "mana_cost": "{3}{R}{R}",
        "type_line": "生物 ～ 化身",
        "oracle_text": "连击\n当狂怒进战场时，对任意数量的目标造成共4点伤害。转化：放逐一张红色手牌。",
        "flavor_text": "“狂暴无需理由，烈火所向即是正义。”",
        "flavor_en": "\"Rage needs no justification; where wildfire treads, ash is the only verdict.\"",
        "flavor_author": "——火山化身",
        "pt": "3/3",
        "set_code": "MH2 #126"
    },
    {
        "name_zh": "连环闪电",
        "name_en": "Chain Lightning",
        "mana_cost": "{R}",
        "type_line": "法术",
        "oracle_text": "连环闪电对任意一个目标造成3点伤害。该牌手或生物操控者可以支付{R}{R}复制此咒语。",
        "flavor_text": "“电弧跳跃于大地，将对手的挑衅化为下一击。”",
        "flavor_en": "\"Lightning jumps across the ridge, bouncing from foe to friend and back again.\"",
        "flavor_author": "——传奇时代",
        "pt": "",
        "set_code": "LEG #137"
    },
    {
        "name_zh": "炎灵巨龙尼米捷",
        "name_en": "Niv-Mizzet, Parun",
        "mana_cost": "{U}{U}{U}{R}{R}{R}",
        "type_line": "传奇生物 ～ 龙／法术师",
        "oracle_text": "不能被反击，飞行\n每当任一牌手施放瞬间或法术时，你抓一张牌。\n每当你抓一张牌时，尼米捷对任意目标打1点伤害。",
        "flavor_text": "“好奇心加上无穷智慧，等于一场绚丽的烈火实验。”",
        "flavor_en": "\"Curiosity plus boundless intellect equals a devastatingly beautiful explosion.\"",
        "flavor_author": "——伊捷公会引言",
        "pt": "5/5",
        "set_code": "GRN #192"
    },
    {
        "name_zh": "放逐之火",
        "name_en": "Banefire",
        "mana_cost": "{X}{R}",
        "type_line": "法术",
        "oracle_text": "放逐之火对任意目标造成X点伤害。若X为5或更大，则不能被反击且伤害不能被防止。",
        "flavor_text": "“当火焰超越凡尘极限，哪怕神明亦无法阻挡热浪。”",
        "flavor_en": "\"When dragonfire burns pure enough, prayers cannot cool the molten stone.\"",
        "flavor_author": "——勇德熔炉",
        "pt": "",
        "set_code": "ALA #91"
    },
    {
        "name_zh": "熔岩飞镖",
        "name_en": "Lava Dart",
        "mana_cost": "{R}",
        "type_line": "瞬间",
        "oracle_text": "熔岩飞镖对任意目标造成1点伤害。返照——牺牲一个山脉。",
        "flavor_text": "“哪怕是熔岩中的细微余烬，也足以点燃整座山林。”",
        "flavor_en": "\"Even a dying spark in the gravel is enough to set the mountain ablaze.\"",
        "flavor_author": "——审判预言",
        "pt": "",
        "set_code": "JUD #94"
    },
    {
        "name_zh": "坚忍",
        "name_en": "Endurance",
        "mana_cost": "{1}{G}{G}",
        "type_line": "生物 ～ 化身",
        "oracle_text": "闪现，延势\n进战场时，目标牌手将其坟墓场所有牌洗入其牌库底。转化：放逐一张绿色手牌。",
        "flavor_text": "“古树承载千秋风雨，沉稳自若，万物终将复归其根。”",
        "flavor_en": "\"The ancient trunk stands firm against all tempests, patient and eternal.\"",
        "flavor_author": "——大地灵歌",
        "pt": "3/4",
        "set_code": "MH2 #157"
    },
    {
        "name_zh": "庞巨血口龙",
        "name_en": "Colossal Dreadmaw",
        "mana_cost": "{4}{G}{G}",
        "type_line": "生物 ～ 恐龙",
        "oracle_text": "践踏",
        "flavor_text": "“若你感到地动，快跑；若你听见咆哮，躲藏；若你看见利齿，为时已晚。”",
        "flavor_en": "\"If you feel the ground quake, run. If you hear its roar, flee. If you see its teeth, it's too late.\"",
        "flavor_author": "——依夏兰探险警句",
        "pt": "6/6",
        "set_code": "XLN #180"
    },
    {
        "name_zh": "罗堰妖精",
        "name_en": "Llanowar Elves",
        "mana_cost": "{G}",
        "type_line": "生物 ～ 妖精／德鲁伊",
        "oracle_text": "{T}：加{G}。",
        "flavor_text": "“一树生一念，万叶聚成海。”",
        "flavor_en": "\"One bone broken for every twig snapped under trespasser boots.\"",
        "flavor_author": "——罗堰古训",
        "pt": "1/1",
        "set_code": "LEA #210"
    },
    {
        "name_zh": "庞巨生长",
        "name_en": "Overrun",
        "mana_cost": "{2}{G}{G}{G}",
        "type_line": "法术",
        "oracle_text": "由你操控的生物得+3/+3并获得践踏异能直到回合结束。",
        "flavor_text": "“当荒野涌向文明，城墙如同干枯的树枝般破碎。”",
        "flavor_en": "\"When the wilderness surges forward, the high walls of man snap like twigs.\"",
        "flavor_author": "——古鲁战歌",
        "pt": "",
        "set_code": "TMP #245"
    },
    {
        "name_zh": "森林图书馆",
        "name_en": "Sylvan Library",
        "mana_cost": "{1}{G}",
        "type_line": "结界",
        "oracle_text": "在你的抓牌步骤开始时，你可以额外抓两张牌。若如此做，选择手牌中两张牌，为每张支付4点生命或放回牌库顶。",
        "flavor_text": "“大自然记录一切，只是它用树轮而非墨水书写。”",
        "flavor_en": "\"Nature preserves all memory, written not in ink, but in the rings of ancient oak.\"",
        "flavor_author": "——传奇典范",
        "pt": "",
        "set_code": "LEG #207"
    },
    {
        "name_zh": "赞迪卡之复仇",
        "name_en": "Avenger of Zendikar",
        "mana_cost": "{5}{G}{G}",
        "type_line": "生物 ～ 元素",
        "oracle_text": "进战场时，为你操控的每个地派出一个0/1植物衍生生物。\n地落——在每个植物上放一个+1/+1指示物。",
        "flavor_text": "“被践踏的大地在此刻还击，每一粒种子皆是战士。”",
        "flavor_en": "\"The trampled soil strikes back, and every tiny seed bears armor and blade.\"",
        "flavor_author": "——赞迪卡怒潮",
        "pt": "5/5",
        "set_code": "WWK #96"
    },
    {
        "name_zh": "始祖卡诺索斯",
        "name_en": "Carnage Tyrant",
        "mana_cost": "{4}{G}{G}",
        "type_line": "生物 ～ 恐龙",
        "oracle_text": "不能被反击，践踏，辟邪",
        "flavor_text": "“它是不可阻挡的纯粹力量，任何魔法在它面前皆如细丝。”",
        "flavor_en": "\"A crushing manifestation of raw prehistoric fury, deaf to all incantations.\"",
        "flavor_author": "——依夏兰传记",
        "pt": "7/6",
        "set_code": "XLN #179"
    },
    {
        "name_zh": "探索",
        "name_en": "Explore",
        "mana_cost": "{1}{G}",
        "type_line": "法术",
        "oracle_text": "你本回合可以额外下一个地。抓一张牌。",
        "flavor_text": "“地图的边缘，正是奇迹的开端。”",
        "flavor_en": "\"The edge of the parchment is where true adventure begins.\"",
        "flavor_author": "——赞迪卡向导",
        "pt": "",
        "set_code": "WWK #99"
    },
    {
        "name_zh": "莫克斯钻石",
        "name_en": "Mox Diamond",
        "mana_cost": "{0}",
        "type_line": "神器",
        "oracle_text": "如果莫克斯钻石将进战场，改为弃掉一张地牌；若你不弃，则将其置入坟墓场。\n{T}：加一点任意颜色的法术力。",
        "flavor_text": "“将整片山河凝缩成一点璀璨星光。”",
        "flavor_en": "\"An entire realm condensed into a pinpoint of dazzling brilliance.\"",
        "flavor_author": "——强恩遗物",
        "pt": "",
        "set_code": "STH #138"
    },
    {
        "name_zh": "莲花瓣",
        "name_en": "Lotus Petal",
        "mana_cost": "{0}",
        "type_line": "神器",
        "oracle_text": "{T}，牺牲莲花瓣：加一点任意颜色的法术力。",
        "flavor_text": "“转瞬即逝的美丽，蕴含着改变战局的伟力。”",
        "flavor_en": "\"A single petal falls, and a realm's fate pivots upon the wind.\"",
        "flavor_author": "——暴风雨日记",
        "pt": "",
        "set_code": "TMP #294"
    },
    {
        "name_zh": "催眠宝珠",
        "name_en": "Mesmeric Orb",
        "mana_cost": "{2}",
        "type_line": "神器",
        "oracle_text": "每当一个永久物重置时，其操控者将其牌库顶牌置入其坟墓场。",
        "flavor_text": "“每转动一次，思绪便剥落一片，直到只剩空壳。”",
        "flavor_en": "\"With each hypnotic rotation, memories flake away like dried leaves.\"",
        "flavor_author": "——秘罗地暗黑卷轴",
        "pt": "",
        "set_code": "MRD #204"
    },
    {
        "name_zh": "探险地图",
        "name_en": "Expedition Map",
        "mana_cost": "{1}",
        "type_line": "神器",
        "oracle_text": "{2}，{T}，牺牲探险地图：从你的牌库搜寻一张地牌，展示并置入手牌，然后洗牌。",
        "flavor_text": "“跟随古老的羊皮纸线条，去往凡人不可及之境。”",
        "flavor_en": "\"Follow lines faded by time to places no sovereign has claimed.\"",
        "flavor_author": "——赞迪卡测绘图",
        "pt": "",
        "set_code": "ZEN #201"
    },
    {
        "name_zh": "闪电天使",
        "name_en": "Lightning Angel",
        "mana_cost": "{1}{U}{R}{W}",
        "type_line": "生物 ～ 天使",
        "oracle_text": "飞行，警戒，敏捷",
        "flavor_text": "“她拥有晨曦之纯洁，烈焰之迅捷，以及风暴之愤怒。”",
        "flavor_en": "\"Pure as the morning sun, swift as wildfire, and fierce as thunder.\"",
        "flavor_author": "——启示录典籍",
        "pt": "3/4",
        "set_code": "APC #117"
    },
    {
        "name_zh": "执政官阿崔夏",
        "name_en": "Atraxa, Praetors' Voice",
        "mana_cost": "{G}{W}{U}{B}",
        "type_line": "传奇生物 ～ 天使／恐怖兽",
        "oracle_text": "飞行，警戒，死触，系命\n在你的结束步骤开始时，增殖。",
        "flavor_text": "“四色意志熔铸成唯一纯净的金属，宣告终局之统合。”",
        "flavor_en": "\"Four visions fused into porcelain and oil, herald to the grand completion.\"",
        "flavor_author": "——新非瑞克西亚圣谕",
        "pt": "4/4",
        "set_code": "C16 #28"
    },
    {
        "name_zh": "迷宫终点",
        "name_en": "Maze's End",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "迷宫终点横置进战场。\n{T}：加{C}。\n{3}，{T}，将迷宫终点移回手牌：搜寻公会门置入战场。若你操控十个具不同名称的门，你赢得游戏。",
        "flavor_text": "“穿越十门考验者，方能见证拉尼卡的真正荣耀。”",
        "flavor_en": "\"Those who solve the implicit maze claim the mantle of the living guildpact.\"",
        "flavor_author": "——十会盟铭文",
        "pt": "",
        "set_code": "DGM #152"
    },
    {
        "name_zh": "血斑泥沼",
        "name_en": "Bloodstained Mire",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "{T}，支付1点生命，牺牲血斑泥沼：从你的牌库搜寻一张沼泽或山脉牌并置入战场，然后洗牌。",
        "flavor_text": "“远古的战场沉没在泥泞中，地表仍渗出暗红色的热血。”",
        "flavor_en": "\"Where ancient armies bled, the soil still weeps in crimson mud.\"",
        "flavor_author": "——石破天惊地志",
        "pt": "",
        "set_code": "ONS #313"
    },
    {
        "name_zh": "被淹没的地下墓穴",
        "name_en": "Drowned Catacomb",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "除非你操控海岛或沼泽，否则横置进战场。\n{T}：加{U}或{B}。",
        "flavor_text": "“潮水退去时，亡者的石棺露出水面，静默如谜。”",
        "flavor_en": "\"When tides recede, sarcophagi gleam wet and silent beneath pale moonlight.\"",
        "flavor_author": "——深渊遗迹",
        "pt": "",
        "set_code": "M10 #224"
    },
    {
        "name_zh": "无影虚空",
        "name_en": "Ghost Quarter",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "{T}：加{C}。\n{T}，牺牲无影虚空：消灭目标地。其操控者可以搜寻一张基本地置入战场。",
        "flavor_text": "“荒废的街道空无一人，唯有风在倾颓的石拱间徘徊。”",
        "flavor_en": "\"Empty alleys whisper forgotten names under crumbling arches.\"",
        "flavor_author": "——拉尼卡旧城记",
        "pt": "",
        "set_code": "DIS #173"
    },
    {
        "name_zh": "乌金之眼",
        "name_en": "Eye of Ugin",
        "mana_cost": "",
        "type_line": "传奇地",
        "oracle_text": "你施放的无色奥札奇咒语减少{2}费用。\n{7}，{T}：搜寻无色生物牌置入手牌。",
        "flavor_text": "“沉睡的巨兽在此凝视虚空，封印渐行松动。”",
        "flavor_en": "\"An ancient eye gazes into timeless depths as crystal seals begin to crack.\"",
        "flavor_author": "——赞迪卡秘史",
        "pt": "",
        "set_code": "WWK #136"
    },
    {
        "name_zh": "卡拉德许发明家发明展",
        "name_en": "Inventors' Fair",
        "mana_cost": "",
        "type_line": "传奇地",
        "oracle_text": "在你的维持开始时，若你操控三个或更多神器，获得1点生命。\n{4}，{T}，牺牲此地：搜寻神器牌置入手牌。",
        "flavor_text": "“巧夺天工的机械在阳光下熠熠生辉，宣告天才的胜利。”",
        "flavor_en": "\"Gleaming brass and shimmering filigree proclaim the triumph of mortal intellect.\"",
        "flavor_author": "——发明家博览会",
        "pt": "",
        "set_code": "KLD #247"
    },
    {
        "name_zh": "乌尔萨的工坊",
        "name_en": "Urza's Workshop",
        "mana_cost": "",
        "type_line": "地 ～ 乌尔萨的",
        "oracle_text": "{T}：加{C}。金属术——如果你操控三个或更多神器，改为加等同于你操控乌尔萨地数量的{C}。",
        "flavor_text": "“凡乌尔萨双手触及之物，皆化为永恒运转的机械奇迹。”",
        "flavor_en": "\"Whatever Urza's hands shaped turned into an enduring testament of iron.\"",
        "flavor_author": "——多明纳里亚古卷",
        "pt": "",
        "set_code": "BRC #41"
    },
    {
        "name_zh": "乌尔萨的动力炉",
        "name_en": "Urza's Power Plant",
        "mana_cost": "",
        "type_line": "地 ～ 乌尔萨的动力炉",
        "oracle_text": "{T}：加{C}。若你操控矿脉与塔，改为加{C}{C}。",
        "flavor_text": "“机械的轰鸣震颤地壳，提供源源不绝的动力。”",
        "flavor_en": "\"The rhythmic pounding of pistons shakes the mantle, feeding endless industry.\"",
        "flavor_author": "——神器之战",
        "pt": "",
        "set_code": "CHR #115"
    },
    {
        "name_zh": "乌尔萨的矿脉",
        "name_en": "Urza's Mine",
        "mana_cost": "",
        "type_line": "地 ～ 乌尔萨的矿脉",
        "oracle_text": "{T}：加{C}。若你操控动力炉与塔，改为加{C}{C}。",
        "flavor_text": "“深入大地腹地，挖掘足以重塑世界的矿石。”",
        "flavor_en": "\"Deep into the subterranean dark, veins of raw potential are stripped bare.\"",
        "flavor_author": "——神器之战",
        "pt": "",
        "set_code": "CHR #114"
    },
    {
        "name_zh": "乌尔萨的高塔",
        "name_en": "Urza's Tower",
        "mana_cost": "",
        "type_line": "地 ～ 乌尔萨的高塔",
        "oracle_text": "{T}：加{C}。若你操控矿脉与动力炉，改为加{C}{C}{C}。",
        "flavor_text": "“高耸入云的尖塔汇聚苍穹雷霆，驱动创世神器。”",
        "flavor_en": "\"Towering needles channel the celestial storm to fuel machines of creation.\"",
        "flavor_author": "——神器之战",
        "pt": "",
        "set_code": "CHR #116"
    },
    {
        "name_zh": "古鲁地盘",
        "name_en": "Gruul Turf",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "横置进战场。进战场时移回一个地。\n{T}：加{R}{G}。",
        "flavor_text": "“这不是花园，这是捕猎场；留下头骨，或者滚出去。”",
        "flavor_en": "\"Not a park, not a garden—a hunting reserve where civilization is devoured.\"",
        "flavor_author": "——古鲁族训",
        "pt": "",
        "set_code": "GPT #158"
    },
    {
        "name_zh": "底密尔藏匿处",
        "name_en": "Dimir Aqueduct",
        "mana_cost": "",
        "type_line": "地",
        "oracle_text": "横置进战场。进战场时移回一个地。\n{T}：加{U}{B}。",
        "flavor_text": "“污水之下流动的是秘密，而秘密比金币更有分量。”",
        "flavor_en": "\"Beneath the filth flows intelligence, and intelligence outweighs gold.\"",
        "flavor_author": "——底密尔信条",
        "pt": "",
        "set_code": "RAV #271"
    },
    {
        "name_zh": "塔莫耶夫",
        "name_en": "Tarmogoyf",
        "mana_cost": "{1}{G}",
        "type_line": "生物 ～ 勒孚",
        "oracle_text": "塔莫耶夫的力量等同于所有坟墓场中牌的类别数量，其防御力等同于该数量加1。",
        "flavor_text": "“死者越多，它的爪牙越锋利。”",
        "flavor_en": "\"What feeds on the fallen grows mightier with each casualty left behind.\"",
        "flavor_author": "——预知将来",
        "pt": "*/*+1",
        "set_code": "FUT #153"
    },
    {
        "name_zh": "脑冻结",
        "name_en": "Brain Freeze",
        "mana_cost": "{1}{U}",
        "type_line": "瞬间",
        "oracle_text": "目标牌手将其牌库顶三张牌置入其坟墓场。风暴（施放此咒语时，本回合此前每施放过一个咒语，便复制一个此咒语。）",
        "flavor_text": "“当脑海充斥过多思绪，思考本身化为严霜。”",
        "flavor_en": "\"A flurry of thought that leaves the mind freezing cold.\"",
        "flavor_author": "——智者遗训",
        "pt": "",
        "set_code": "SCG #29"
    }
]

def generate_c_file(output_path):
    print(f"Generating {len(CARDS)} cards into {output_path}...")
    
    lines = []
    lines.append('#include "mtg_cards.h"')
    lines.append('#include <stdlib.h>')
    lines.append('')
    lines.append('#define DECK_MAX 256')
    lines.append('')
    lines.append('static uint16_t s_deck[DECK_MAX];')
    lines.append('static size_t s_deck_pos = 0;')
    lines.append('static uint32_t s_rng_state = 123456789;')
    lines.append('static uint16_t s_last_card_idx = 0xFFFF;')
    lines.append('')
    lines.append('static uint32_t xorshift32(void)')
    lines.append('{')
    lines.append('    uint32_t x = s_rng_state;')
    lines.append('    x ^= x << 13;')
    lines.append('    x ^= x >> 17;')
    lines.append('    x ^= x << 5;')
    lines.append('    s_rng_state = x ? x : 1;')
    lines.append('    return s_rng_state;')
    lines.append('}')
    lines.append('')
    lines.append('static const mtg_card_t s_cards[] = {')

    for i, c in enumerate(CARDS):
        # escape double quotes if needed
        def esc(s):
            return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

        lines.append('    {')
        lines.append(f'        .name_zh = "{esc(c["name_zh"])}",')
        lines.append(f'        .name_en = "{esc(c["name_en"])}",')
        lines.append(f'        .mana_cost = "{esc(c["mana_cost"])}",')
        lines.append(f'        .type_line = "{esc(c["type_line"])}",')
        lines.append(f'        .oracle_text = "{esc(c["oracle_text"])}",')
        lines.append(f'        .flavor_text = "{esc(c["flavor_text"])}",')
        lines.append(f'        .flavor_en = "{esc(c["flavor_en"])}",')
        lines.append(f'        .flavor_author = "{esc(c["flavor_author"])}",')
        lines.append(f'        .pt = "{esc(c["pt"])}",')
        lines.append(f'        .set_code = "{esc(c["set_code"])}",')
        lines.append('    }' + (',' if i < len(CARDS) - 1 else ''))

    lines.append('};')
    lines.append('')
    lines.append('static const size_t s_cards_count = sizeof(s_cards) / sizeof(s_cards[0]);')
    lines.append('')
    lines.append('/* Fisher-Yates 洗牌整副牌库（保证整副牌抽完前 0 重复） */')
    lines.append('static void shuffle_deck(void)')
    lines.append('{')
    lines.append('    size_t count = s_cards_count;')
    lines.append('    if (count == 0) return;')
    lines.append('    if (count > DECK_MAX) count = DECK_MAX;')
    lines.append('')
    lines.append('    for (size_t i = 0; i < count; i++) {')
    lines.append('        s_deck[i] = (uint16_t)i;')
    lines.append('    }')
    lines.append('')
    lines.append('    for (size_t i = count - 1; i > 0; i--) {')
    lines.append('        size_t j = (size_t)(xorshift32() % (uint32_t)(i + 1));')
    lines.append('        uint16_t tmp = s_deck[i];')
    lines.append('        s_deck[i] = s_deck[j];')
    lines.append('        s_deck[j] = tmp;')
    lines.append('    }')
    lines.append('')
    lines.append('    /* 若重洗后第一张等于上一轮最后一张，调换至第二张，杜绝相邻重复 */')
    lines.append('    if (count > 1 && s_deck[0] == s_last_card_idx) {')
    lines.append('        uint16_t tmp = s_deck[0];')
    lines.append('        s_deck[0] = s_deck[1];')
    lines.append('        s_deck[1] = tmp;')
    lines.append('    }')
    lines.append('')
    lines.append('    s_deck_pos = 0;')
    lines.append('}')
    lines.append('')
    lines.append('void mtg_cards_init(uint32_t seed)')
    lines.append('{')
    lines.append('    s_rng_state = seed ? seed : 123456789;')
    lines.append('    s_last_card_idx = 0xFFFF;')
    lines.append('    shuffle_deck();')
    lines.append('}')
    lines.append('')
    lines.append('size_t mtg_cards_count(void)')
    lines.append('{')
    lines.append('    return s_cards_count;')
    lines.append('}')
    lines.append('')
    lines.append('const mtg_card_t *mtg_cards_get_by_index(size_t index)')
    lines.append('{')
    lines.append('    if (s_cards_count == 0) return NULL;')
    lines.append('    return &s_cards[index % s_cards_count];')
    lines.append('}')
    lines.append('')
    lines.append('const mtg_card_t *mtg_cards_get_random(void)')
    lines.append('{')
    lines.append('    if (s_cards_count == 0) return NULL;')
    lines.append('    if (s_deck_pos >= s_cards_count) {')
    lines.append('        shuffle_deck();')
    lines.append('    }')
    lines.append('    uint16_t card_idx = s_deck[s_deck_pos++];')
    lines.append('    s_last_card_idx = card_idx;')
    lines.append('    return &s_cards[card_idx];')
    lines.append('}')
    lines.append('')

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    print(f"Successfully generated {output_path} with {len(CARDS)} cards.")

if __name__ == '__main__':
    out = os.path.join(os.path.dirname(__file__), '../main/mtg_cards.c')
    generate_c_file(out)
