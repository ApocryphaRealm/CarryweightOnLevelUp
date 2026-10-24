# -*- coding: utf-8 -*-
"""gen-translations.py - builds the eleven CarryweightOnLevelUp_<language>.txt files.

The English key list is extracted from the PATCHED source/UI.cpp by regex on
strings::TR("KEY", "text") so it can never drift from the code. The other ten languages are
this project's own translations of that list, held below as parallel dictionaries.

Writes REPO/dist/Interface/Translations/CarryweightOnLevelUp_<language>.txt for english + the
owner's ten languages (UTF-16LE with a BOM, one "$key<TAB>text" per line, literal "\\n" for an
embedded line break, CRLF records - the SKSE/SkyUI shape AMF's own Strings.cpp reads).

Run: `python tools/gen-translations.py` from the repo root or anywhere (paths are relative to
this script's grandparent directory).
"""
import io
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LANGS = ["english", "japanese", "korean", "chinese", "russian", "german", "french", "spanish", "italian", "polish", "czech"]

TR_RE = re.compile(r'strings::TR\(\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)')


def unescape(s):
    return s.encode("latin-1", "backslashreplace").decode("unicode_escape") if "\\" in s else s


KEY_ARRAY_RE = re.compile(r'constexpr const char\* kLogLevelKeys\[\]\s*=\s*\{([^}]*)\};', re.S)
NAME_ARRAY_RE = re.compile(r'constexpr const char\* kLogLevelNames\[\]\s*=\s*\{([^}]*)\};', re.S)
STR_LIT_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def read_keys():
    path = os.path.join(REPO, "source", "UI.cpp")
    src = io.open(path, "r", encoding="utf-8").read()
    keys = {}
    order = []
    for m in TR_RE.finditer(src):
        key, text = unescape(m.group(1)), unescape(m.group(2))
        if key in keys and keys[key] != text:
            raise RuntimeError(f"duplicate key {key!r} with two different English texts: {keys[key]!r} vs {text!r}")
        if key not in keys:
            order.append(key)
        keys[key] = text

    # The log-level Combo's option labels are looked up by a parallel key array
    # (kLogLevelKeys[i] -> kLogLevelNames[i]) rather than a literal strings::TR(...) call, since
    # the option text is rebuilt into a std::vector per frame. Pair the two arrays positionally.
    km = KEY_ARRAY_RE.search(src)
    nm = NAME_ARRAY_RE.search(src)
    if not km or not nm:
        raise RuntimeError("could not find kLogLevelKeys/kLogLevelNames arrays in source/UI.cpp")
    array_keys = [unescape(s) for s in STR_LIT_RE.findall(km.group(1))]
    array_names = [unescape(s) for s in STR_LIT_RE.findall(nm.group(1))]
    if len(array_keys) != len(array_names):
        raise RuntimeError(f"kLogLevelKeys ({len(array_keys)}) and kLogLevelNames ({len(array_names)}) length mismatch")
    for key, text in zip(array_keys, array_names):
        if key in keys and keys[key] != text:
            raise RuntimeError(f"duplicate key {key!r} with two different English texts: {keys[key]!r} vs {text!r}")
        if key not in keys:
            order.append(key)
        keys[key] = text

    return keys, order


# ------------------------------------------------------------------------------------------------
# Translations for every key found in source/UI.cpp. Plain, literal renderings of the UI text;
# every printf specifier is kept exactly; product names (Skyrim, Apocrypha Menu Framework,
# Carryweight on Level Up) stay untranslated; file names (CarryweightOnLevelUp.log, the INI)
# stay untranslated inside the translated sentence.
# ------------------------------------------------------------------------------------------------
TRANSLATIONS = {
    "COLU_HelpMark": {
        "japanese": "(?)", "korean": "(?)", "chinese": "(?)", "russian": "(?)", "german": "(?)",
        "french": "(?)", "spanish": "(?)", "italian": "(?)", "polish": "(?)", "czech": "(?)",
    },
    "COLU_NudgeArrows": {
        "japanese": "<-->", "korean": "<-->", "chinese": "<-->", "russian": "<-->", "german": "<-->",
        "french": "<-->", "spanish": "<-->", "italian": "<-->", "polish": "<-->", "czech": "<-->",
    },
    "COLU_CarryWeight": {
        "japanese": "運搬重量", "korean": "소지 무게", "chinese": "负重", "russian": "Переносимый вес",
        "german": "Traggewicht", "french": "Capacité de transport", "spanish": "Peso transportable",
        "italian": "Peso trasportabile", "polish": "Udźwig", "czech": "Nosnost",
    },
    "COLU_StartingWeight": {
        "japanese": "初期運搬重量", "korean": "시작 소지 무게", "chinese": "初始负重", "russian": "Начальный вес",
        "german": "Anfangstraggewicht", "french": "Capacité de départ", "spanish": "Peso inicial",
        "italian": "Peso iniziale", "polish": "Udźwig początkowy", "czech": "Počáteční nosnost",
    },
    "COLU_HelpStartingWeight": {
        "japanese": "レベル1での運搬重量。バニラのSkyrimは300から始まります。",
        "korean": "레벨 1의 소지 무게입니다. 바닐라 Skyrim은 300에서 시작합니다.",
        "chinese": "1 级时的负重。原版 Skyrim 从 300 开始。",
        "russian": "Переносимый вес на 1 уровне. В оригинальном Skyrim он начинается с 300.",
        "german": "Traggewicht auf Stufe 1. Das unmodifizierte Skyrim beginnt bei 300.",
        "french": "Capacité de transport au niveau 1. Skyrim vanilla démarre à 300.",
        "spanish": "Peso transportable en el nivel 1. El Skyrim vanilla empieza en 300.",
        "italian": "Peso trasportabile al livello 1. Lo Skyrim vanilla parte da 300.",
        "polish": "Udźwig na 1. poziomie. Podstawowy Skyrim zaczyna od 300.",
        "czech": "Nosnost na 1. úrovni. Vanilla Skyrim začíná na 300.",
    },
    "COLU_PerLevel": {
        "japanese": "レベルごとの増加量", "korean": "레벨당 증가량", "chinese": "每级增加",
        "russian": "За уровень", "german": "Pro Stufe", "french": "Par niveau", "spanish": "Por nivel",
        "italian": "Per livello", "polish": "Za poziom", "czech": "Za úroveň",
    },
    "COLU_HelpPerLevel": {
        "japanese": "1レベルを超えるごとに追加される運搬重量です。",
        "korean": "1레벨을 초과하는 모든 레벨마다 추가되는 소지 무게입니다.",
        "chinese": "每高于 1 级的每一级所增加的负重。",
        "russian": "Переносимый вес, добавляемый за каждый уровень выше первого.",
        "german": "Traggewicht, das für jede Stufe über Stufe 1 hinzugefügt wird.",
        "french": "Capacité de transport ajoutée pour chaque niveau au-delà du niveau 1.",
        "spanish": "Peso transportable añadido por cada nivel por encima del 1.",
        "italian": "Peso trasportabile aggiunto per ogni livello superiore al 1.",
        "polish": "Udźwig dodawany za każdy poziom powyżej 1.",
        "czech": "Nosnost přidaná za každou úroveň nad 1.",
    },
    "COLU_ApplyNowBtn": {
        "japanese": "今すぐ適用", "korean": "지금 적용", "chinese": "立即应用", "russian": "Применить сейчас",
        "german": "Jetzt anwenden", "french": "Appliquer maintenant", "spanish": "Aplicar ahora",
        "italian": "Applica ora", "polish": "Zastosuj teraz", "czech": "Použít nyní",
    },
    "COLU_StatusApplied": {
        "japanese": "現在のレベルに適用しました。", "korean": "현재 레벨에 적용했습니다.", "chinese": "已应用于你当前的等级。",
        "russian": "Применено к вашему текущему уровню.", "german": "Auf deine aktuelle Stufe angewendet.",
        "french": "Appliqué à votre niveau actuel.", "spanish": "Aplicado a tu nivel actual.",
        "italian": "Applicato al tuo livello attuale.", "polish": "Zastosowano do twojego obecnego poziomu.",
        "czech": "Použito na vaši aktuální úroveň.",
    },
    "COLU_HelpApplyNow": {
        "japanese": "上記の値で現在のレベルの数式を再発行します。セーブのロード時、レベルアップ時、スライダーを動かした時にも自動的に実行されます - バックグラウンドでは何も動作しません。",
        "korean": "위 값으로 현재 레벨의 수식을 다시 발행합니다. 세이브를 불러올 때, 레벨업할 때, 슬라이더를 움직일 때도 자동으로 실행됩니다 - 백그라운드에서는 아무것도 실행되지 않습니다.",
        "chinese": "使用上面的值为你当前的等级重新发布公式。它也会在读取存档、升级或移动滑块时自行运行 - 后台不会运行任何东西。",
        "russian": "Повторно применяет формулу для вашего текущего уровня со значениями выше. Также срабатывает сама при загрузке сохранения, повышении уровня и изменении ползунка - в фоне ничего не выполняется.",
        "german": "Wendet die Formel für deine aktuelle Stufe mit den obigen Werten erneut an. Läuft außerdem selbstständig beim Laden eines Spielstands, bei einem Stufenaufstieg oder wenn du einen Schieberegler bewegst - im Hintergrund läuft nichts.",
        "french": "Réémet la formule pour votre niveau actuel avec les valeurs ci-dessus. S'exécute aussi seule au chargement d'une sauvegarde, à la montée de niveau ou quand vous déplacez un curseur - rien ne tourne en arrière-plan.",
        "spanish": "Reemite la fórmula para tu nivel actual con los valores anteriores. También se ejecuta por sí sola al cargar una partida, al subir de nivel o al mover un control deslizante - nada se ejecuta en segundo plano.",
        "italian": "Riemette la formula per il tuo livello attuale con i valori sopra. Viene eseguita anche da sola al caricamento di un salvataggio, al passaggio di livello o quando muovi uno slider - nulla è in esecuzione in background.",
        "polish": "Ponownie wydaje formułę dla twojego obecnego poziomu z wartościami powyżej. Działa też samoczynnie przy wczytaniu zapisu, awansie poziomu lub przesunięciu suwaka - w tle nic nie działa.",
        "czech": "Znovu vydá formuli pro vaši aktuální úroveň s hodnotami výše. Také se spustí sama při načtení uložené hry, postupu na úroveň nebo posunutí posuvníku - na pozadí nic neběží.",
    },
    "COLU_CpcStandsDown": {
        "japanese": 'Character Progression Control も読み込まれています: このMODが読み込まれている間はその「Carry Weight」タブは手を引くので、所持重量を設定するのはこのページだけです。',
        "korean": 'Character Progression Control도 불러와져 있습니다: 이 모드가 있는 동안 그 “Carry Weight” 탭은 물러나므로 소지 무게는 이 페이지만 설정합니다.',
        "chinese": 'Character Progression Control 也已加载：本模组加载期间它的“Carry Weight”标签页会让位，因此只有本页设置负重。',
        "russian": 'Character Progression Control тоже загружен: пока загружен этот мод, его вкладка «Carry Weight» уступает, и переносимый вес задаёт только эта страница.',
        "german": 'Character Progression Control ist ebenfalls geladen: Sein Reiter „Carry Weight“ hält sich zurück, solange diese Mod geladen ist, also setzt nur diese Seite die Tragkraft.',
        "french": 'Character Progression Control est aussi chargé : son onglet « Carry Weight » se retire tant que ce mod est chargé, donc seule cette page fixe la charge.',
        "spanish": 'Character Progression Control también está cargado: su pestaña «Carry Weight» se retira mientras este mod esté cargado, así que solo esta página fija la carga.',
        "italian": 'Anche Character Progression Control è caricato: la sua scheda «Carry Weight» si fa da parte finché questa mod è caricata, quindi solo questa pagina imposta il peso trasportabile.',
        "polish": 'Character Progression Control też jest wczytany: jego zakładka „Carry Weight” ustępuje, dopóki ten mod jest wczytany, więc udźwig ustawia tylko ta strona.',
        "czech": 'Character Progression Control je také načten: jeho karta „Carry Weight“ ustupuje, dokud je tento mod načten, takže nosnost nastavuje jen tato stránka.',
    },
    "COLU_StartingValues": {
        "japanese": '開始時の値を設定',
        "korean": '시작 값 설정',
        "chinese": '设置初始值',
        "russian": 'Задавать начальные значения',
        "german": 'Startwerte festlegen',
        "french": 'Définir les valeurs de départ',
        "spanish": 'Establecer los valores iniciales',
        "italian": 'Imposta i valori iniziali',
        "polish": 'Ustawiaj wartości początkowe',
        "czech": 'Nastavit počáteční hodnoty',
    },
    "COLU_HelpStartingValues": {
        "japanese": '既定ではオン。オフ: 開始時の所持重量は種族や他のMODが設定したままになり、下のレベルごとの増加分だけが加算されます。',
        "korean": '기본값은 켜짐. 끄면: 시작 소지 무게는 종족과 다른 모드가 정한 그대로이고, 아래의 레벨당 증가분만 더해집니다.',
        "chinese": '默认开启。关闭：初始负重保持种族及其他模组的设定，只加上下面的每级增量。',
        "russian": 'По умолчанию включено. Выключено: начальный переносимый вес остаётся таким, как его задали раса и другие моды, добавляется только прибавка за уровень ниже.',
        "german": 'Standardmäßig an. Aus: Deine Start-Tragkraft bleibt, wie Rasse und andere Mods sie setzen, und nur der Zuwachs pro Stufe unten kommt hinzu.',
        "french": "Activé par défaut. Désactivé : votre charge de départ reste telle que votre race et les autres mods la fixent, et seul le gain par niveau ci-dessous s'ajoute.",
        "spanish": 'Activado por defecto. Desactivado: tu carga inicial queda como la fijan tu raza y otros mods, y solo se suma la ganancia por nivel de abajo.',
        "italian": "Attivo per impostazione predefinita. Spento: il peso iniziale resta come lo impostano la razza e le altre mod, e si aggiunge solo l'aumento per livello qui sotto.",
        "polish": 'Domyślnie włączone. Wyłączone: początkowy udźwig zostaje taki, jak ustawiła go rasa i inne mody, a dodawany jest tylko przyrost na poziom poniżej.',
        "czech": 'Ve výchozím stavu zapnuto. Vypnuto: počáteční nosnost zůstává tak, jak ji nastavila rasa a jiné mody, a přičítá se jen přírůstek za úroveň níže.',
    },
    "COLU_OwnBase": {
        "japanese": '種族の値ではなくこのMODの基本値を使う',
        "korean": '종족 값 대신 이 모드의 기본값 사용',
        "chinese": '使用本模组的基础值代替种族的数值',
        "russian": 'Использовать наши базовые значения вместо расовых',
        "german": 'Unsere Grundwerte statt der Rassenwerte verwenden',
        "french": 'Utiliser nos valeurs de base au lieu de celles de la race',
        "spanish": 'Usar nuestros valores base en lugar de los de la raza',
        "italian": 'Usa i nostri valori base invece di quelli della razza',
        "polish": 'Używaj naszych wartości bazowych zamiast rasowych',
        "czech": 'Použít naše základní hodnoty místo hodnot rasy',
    },
    "COLU_HelpOwnBase": {
        "japanese": '既定ではオフ: このMODは種族(および種族MOD)が設定した値の上に所持重量を加算します。オン: 所持重量は下の式の合計値になり、種族や他のMODの設定に置き換わります。オフに戻すと種族の値が正確に戻ります。',
        "korean": '기본값은 꺼짐: 이 모드는 종족(및 종족 모드)이 정한 값 위에 소지 무게를 더합니다. 켜면: 소지 무게가 아래 공식의 합계가 되어 종족과 다른 모드의 설정을 대신합니다. 다시 끄면 종족의 값이 정확히 돌아옵니다.',
        "chinese": '默认关闭：本模组把负重叠加在种族（以及种族模组）设定的数值之上。开启：负重设为下面公式的总值，取代种族和其他模组的设定。再次关闭即可精确恢复种族的数值。',
        "russian": 'По умолчанию выключено: мод добавляет переносимый вес поверх того, что задаёт раса (и расовый мод). Включено: переносимый вес задаётся итогом формулы ниже, заменяя то, что задали раса и другие моды. Если снова выключить, расовое значение вернётся точно.',
        "german": 'Standardmäßig aus: Diese Mod legt ihre Tragkraft auf das, was deine Rasse (und eine Rassen-Mod) setzt. An: Deine Tragkraft wird auf die Summe der Formel unten gesetzt und ersetzt, was Rasse und andere Mods setzen. Wieder ausgeschaltet, ist der Wert der Rasse genau zurück.',
        "french": 'Désactivé par défaut : ce mod ajoute sa charge à ce que fixe votre race (et tout mod de race). Activé : votre charge est fixée au total de la formule ci-dessous, à la place de ce que fixent la race et les autres mods. Le désactiver rend exactement la valeur de la race.',
        "spanish": 'Desactivado por defecto: este mod suma su carga a lo que fija tu raza (y cualquier mod de razas). Activado: tu carga se fija en el total de la fórmula de abajo, en lugar de lo que fijan la raza y otros mods. Al desactivarlo vuelve exactamente el valor de la raza.',
        "italian": 'Spento per impostazione predefinita: questa mod aggiunge il suo peso a quello impostato dalla razza (e da una mod delle razze). Acceso: il peso trasportabile è impostato al totale della formula qui sotto, al posto di ciò che impostano la razza e le altre mod. Spegnendolo torna esattamente il valore della razza.',
        "polish": 'Domyślnie wyłączone: mod dodaje swój udźwig do tego, co ustawia rasa (i mod ras). Włączone: udźwig jest ustawiany na sumę ze wzoru poniżej, zamiast tego, co ustawiają rasa i inne mody. Po ponownym wyłączeniu wartość rasy wraca dokładnie.',
        "czech": 'Ve výchozím stavu vypnuto: mod přidává svou nosnost k tomu, co nastavuje rasa (a mod ras). Zapnuto: nosnost se nastaví na součet ze vzorce níže, místo toho, co nastavují rasa a jiné mody. Po vypnutí se hodnota rasy vrátí přesně.',
    },
    "COLU_StartingOff": {
        "japanese": '開始時の値はオフです: 種族の開始時の所持重量はそのままで、レベルごとの増加分だけが適用されます。',
        "korean": '시작 값이 꺼져 있습니다: 종족의 시작 소지 무게는 그대로 두고 레벨당 증가분만 적용됩니다.',
        "chinese": '初始值已关闭：种族的初始负重保持不变，只应用每级增量。',
        "russian": 'Начальные значения выключены: расовый начальный вес не трогается, применяется только прибавка за уровень.',
        "german": 'Startwerte sind aus: Die Start-Tragkraft deiner Rasse bleibt unberührt, nur der Zuwachs pro Stufe gilt.',
        "french": "Les valeurs de départ sont désactivées : la charge de départ de votre race reste intacte et seul le gain par niveau s'applique.",
        "spanish": 'Los valores iniciales están desactivados: la carga inicial de tu raza no se toca y solo se aplica la ganancia por nivel.',
        "italian": "I valori iniziali sono spenti: il peso iniziale della tua razza resta invariato e si applica solo l'aumento per livello.",
        "polish": 'Wartości początkowe są wyłączone: początkowy udźwig rasy zostaje bez zmian, działa tylko przyrost na poziom.',
        "czech": 'Počáteční hodnoty jsou vypnuté: počáteční nosnost rasy zůstává beze změny a platí jen přírůstek za úroveň.',
    },
    "COLU_Offset": {
        "japanese": '種族の開始時の所持重量からのオフセット',
        "korean": '종족 시작 소지 무게에서의 오프셋',
        "chinese": '相对种族初始负重的偏移量',
        "russian": 'Смещение от расового начального веса',
        "german": 'Versatz zur Start-Tragkraft deiner Rasse',
        "french": 'Décalage par rapport à la charge de départ de votre race',
        "spanish": 'Desplazamiento respecto a la carga inicial de tu raza',
        "italian": 'Scostamento dal peso iniziale della tua razza',
        "polish": 'Przesunięcie względem początkowego udźwigu rasy',
        "czech": 'Posun od počáteční nosnosti rasy',
    },
    "COLU_HelpOffset": {
        "japanese": 'レベル1時点で種族の所持重量に加算されます。0なら種族自身の値のままです。Imperiousのアルトマーは250から始まるので、+20で270になります。',
        "korean": '레벨 1에서 종족의 소지 무게에 더해집니다. 0이면 종족 고유의 값 그대로입니다. Imperious의 알트머는 250에서 시작하므로 +20이면 270이 됩니다.',
        "chinese": '在1级时加到种族负重之上；0 保持种族自身的数值。Imperious 中的高精灵从 250 开始，+20 即为 270。',
        "russian": 'Добавляется к расовому весу на 1 уровне; 0 оставляет собственное расовое значение. Альтмер с Imperious начинает с 250, так что +20 даёт 270.',
        "german": 'Wird auf Stufe 1 zur Tragkraft deiner Rasse addiert; 0 behält den eigenen Wert der Rasse. Ein Altmer mit Imperious beginnt bei 250, +20 ergibt also 270.',
        "french": 'Ajouté à la charge de votre race au niveau 1 ; 0 garde la valeur propre à la race. Un Altmer sous Imperious commence à 250, donc +20 donne 270.',
        "spanish": 'Se suma a la carga de tu raza en el nivel 1; 0 mantiene el valor propio de la raza. Un altmer con Imperious empieza en 250, así que +20 da 270.',
        "italian": 'Aggiunto al peso della tua razza al livello 1; 0 mantiene il valore proprio della razza. Un Altmer con Imperious parte da 250, quindi +20 dà 270.',
        "polish": 'Dodawane do udźwigu rasy na 1. poziomie; 0 zachowuje własną wartość rasy. Altmer z Imperious zaczyna od 250, więc +20 daje 270.',
        "czech": 'Přičte se k nosnosti rasy na 1. úrovni; 0 ponechá vlastní hodnotu rasy. Altmer s Imperious začíná na 250, takže +20 dává 270.',
    },
    "COLU_RaceLine": {
        "japanese": '種族の開始値 %.0f、現在の基本所持重量 %.0f',
        "korean": '종족 시작값 %.0f, 현재 기본 소지 무게 %.0f',
        "chinese": '种族初始值 %.0f；当前基础负重 %.0f',
        "russian": 'Раса начинает с %.0f; базовый переносимый вес сейчас %.0f',
        "german": 'Deine Rasse beginnt bei %.0f; deine Grund-Tragkraft jetzt %.0f',
        "french": 'Votre race commence à %.0f ; votre charge de base actuelle %.0f',
        "spanish": 'Tu raza empieza en %.0f; tu carga base ahora %.0f',
        "italian": 'La tua razza parte da %.0f; il tuo peso base ora %.0f',
        "polish": 'Twoja rasa zaczyna od %.0f; twój bazowy udźwig teraz %.0f',
        "czech": 'Vaše rasa začíná na %.0f; vaše základní nosnost nyní %.0f',
    },
    "COLU_LevelFormulaGrowth": {
        "japanese": 'レベル %u: 種族と他のMOD + %.1f x %u = %.0f 運搬重量',
        "korean": '레벨 %u: 종족과 다른 모드 + %.1f x %u = %.0f 소지 무게',
        "chinese": '等级 %u: 种族与其他模组 + %.1f x %u = %.0f 负重',
        "russian": 'Уровень %u: раса и другие моды + %.1f x %u = %.0f переносимого веса',
        "german": 'Stufe %u: Rasse und andere Mods + %.1f x %u = %.0f Traggewicht',
        "french": 'Niveau %u : race et autres mods + %.1f x %u = %.0f de capacité de transport',
        "spanish": 'Nivel %u: raza y otros mods + %.1f x %u = %.0f de peso transportable',
        "italian": 'Livello %u: razza e altre mod + %.1f x %u = %.0f di peso trasportabile',
        "polish": 'Poziom %u: rasa i inne mody + %.1f x %u = %.0f udźwigu',
        "czech": 'Úroveň %u: rasa a jiné mody + %.1f x %u = %.0f nosnosti',
    },
    "COLU_LevelFormulaAdd": {
        "japanese": 'レベル %u: 種族と他のMOD %+.0f + %.1f x %u = %.0f 運搬重量',
        "korean": '레벨 %u: 종족과 다른 모드 %+.0f + %.1f x %u = %.0f 소지 무게',
        "chinese": '等级 %u: 种族与其他模组 %+.0f + %.1f x %u = %.0f 负重',
        "russian": 'Уровень %u: раса и другие моды %+.0f + %.1f x %u = %.0f переносимого веса',
        "german": 'Stufe %u: Rasse und andere Mods %+.0f + %.1f x %u = %.0f Traggewicht',
        "french": 'Niveau %u : race et autres mods %+.0f + %.1f x %u = %.0f de capacité de transport',
        "spanish": 'Nivel %u: raza y otros mods %+.0f + %.1f x %u = %.0f de peso transportable',
        "italian": 'Livello %u: razza e altre mod %+.0f + %.1f x %u = %.0f di peso trasportabile',
        "polish": 'Poziom %u: rasa i inne mody %+.0f + %.1f x %u = %.0f udźwigu',
        "czech": 'Úroveň %u: rasa a jiné mody %+.0f + %.1f x %u = %.0f nosnosti',
    },
    "COLU_LevelFormula": {
        "japanese": "レベル %u: %.0f + %.1f x %u = %.0f 運搬重量",
        "korean": "레벨 %u: %.0f + %.1f x %u = %.0f 소지 무게",
        "chinese": "等级 %u: %.0f + %.1f x %u = %.0f 负重",
        "russian": "Уровень %u: %.0f + %.1f x %u = %.0f переносимого веса",
        "german": "Stufe %u: %.0f + %.1f x %u = %.0f Traggewicht",
        "french": "Niveau %u : %.0f + %.1f x %u = %.0f de capacité de transport",
        "spanish": "Nivel %u: %.0f + %.1f x %u = %.0f de peso transportable",
        "italian": "Livello %u: %.0f + %.1f x %u = %.0f di peso trasportabile",
        "polish": "Poziom %u: %.0f + %.1f x %u = %.0f udźwigu",
        "czech": "Úroveň %u: %.0f + %.1f x %u = %.0f nosnosti",
    },
    "COLU_Debug": {
        "japanese": "デバッグ", "korean": "디버그", "chinese": "调试", "russian": "Отладка", "german": "Debug",
        "french": "Débogage", "spanish": "Depuración", "italian": "Debug", "polish": "Debugowanie", "czech": "Ladění",
    },
    "COLU_LogLevel": {
        "japanese": "ログレベル", "korean": "로그 레벨", "chinese": "日志级别", "russian": "Уровень журнала",
        "german": "Protokollstufe", "french": "Niveau de journal", "spanish": "Nivel de registro",
        "italian": "Livello di log", "polish": "Poziom logowania", "czech": "Úroveň logování",
    },
    "COLU_LogLevel_Trace": {
        "japanese": "トレース", "korean": "추적", "chinese": "跟踪", "russian": "Трассировка", "german": "Trace",
        "french": "Trace", "spanish": "Trace", "italian": "Trace", "polish": "Trace", "czech": "Trace",
    },
    "COLU_LogLevel_Debug": {
        "japanese": "デバッグ", "korean": "디버그", "chinese": "调试", "russian": "Отладка", "german": "Debug",
        "french": "Débogage", "spanish": "Depuración", "italian": "Debug", "polish": "Debugowanie", "czech": "Ladění",
    },
    "COLU_LogLevel_Info": {
        "japanese": "情報", "korean": "정보", "chinese": "信息", "russian": "Информация", "german": "Info",
        "french": "Infos", "spanish": "Información", "italian": "Informazioni", "polish": "Informacje", "czech": "Informace",
    },
    "COLU_LogLevel_Warning": {
        "japanese": "警告", "korean": "경고", "chinese": "警告", "russian": "Предупреждение", "german": "Warnung",
        "french": "Avertissement", "spanish": "Advertencia", "italian": "Avviso", "polish": "Ostrzeżenie", "czech": "Varování",
    },
    "COLU_LogLevel_Error": {
        "japanese": "エラー", "korean": "오류", "chinese": "错误", "russian": "Ошибка", "german": "Fehler",
        "french": "Erreur", "spanish": "Error", "italian": "Errore", "polish": "Błąd", "czech": "Chyba",
    },
    "COLU_LogLevel_Critical": {
        "japanese": "重大", "korean": "치명적", "chinese": "严重", "russian": "Критическая",
        "german": "Kritisch", "french": "Critique", "spanish": "Crítico", "italian": "Critico",
        "polish": "Krytyczny", "czech": "Kritická",
    },
    "COLU_LogLevel_Off": {
        "japanese": "オフ", "korean": "끄기", "chinese": "关闭", "russian": "Отключено", "german": "Aus",
        "french": "Désactivé", "spanish": "Desactivado", "italian": "Disattivato", "polish": "Wyłączone", "czech": "Vypnuto",
    },
    "COLU_HelpLogLevel": {
        "japanese": "即座に適用されます。ログは Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log にあります。",
        "korean": "즉시 적용됩니다. 로그는 Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log 에 있습니다.",
        "chinese": "立即生效。日志位于 Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log。",
        "russian": "Применяется немедленно. Журнал находится здесь: Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
        "german": "Wird sofort angewendet. Das Log liegt unter Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
        "french": "S'applique immédiatement. Le journal se trouve dans Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
        "spanish": "Se aplica de inmediato. El registro está en Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
        "italian": "Si applica immediatamente. Il log si trova in Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
        "polish": "Stosowane natychmiast. Log znajduje się w Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
        "czech": "Použije se okamžitě. Log je v Documents\\My Games\\Skyrim Special Edition\\SKSE\\CarryweightOnLevelUp.log.",
    },
    "COLU_SaveBtn": {
        "japanese": "保存", "korean": "저장", "chinese": "保存", "russian": "Сохранить", "german": "Speichern",
        "french": "Enregistrer", "spanish": "Guardar", "italian": "Salva", "polish": "Zapisz", "czech": "Uložit",
    },
    "COLU_StatusSaving": {
        "japanese": "保存中...", "korean": "저장 중...", "chinese": "正在保存...", "russian": "Сохранение...",
        "german": "Wird gespeichert...", "french": "Enregistrement...", "spanish": "Guardando...",
        "italian": "Salvataggio...", "polish": "Zapisywanie...", "czech": "Ukládání...",
    },
    "COLU_StatusSaved": {
        "japanese": "設定を保存しました。", "korean": "설정을 저장했습니다.", "chinese": "设置已保存。",
        "russian": "Настройки сохранены.", "german": "Einstellungen gespeichert.", "french": "Paramètres enregistrés.",
        "spanish": "Ajustes guardados.", "italian": "Impostazioni salvate.", "polish": "Ustawienia zapisane.",
        "czech": "Nastavení uložena.",
    },
    "COLU_StatusSaveFail": {
        "japanese": "INIの書き込みに失敗しました。理由はログを確認してください。",
        "korean": "INI를 쓸 수 없습니다. 이유는 로그를 확인하세요.",
        "chinese": "无法写入 INI。请查看日志了解原因。",
        "russian": "Не удалось записать INI. Причина — в журнале.",
        "german": "Die INI konnte nicht geschrieben werden. Der Grund steht im Log.",
        "french": "Impossible d'écrire l'INI. Voyez le journal pour la raison.",
        "spanish": "No se pudo escribir el INI. Consulta el registro para saber por qué.",
        "italian": "Impossibile scrivere l'INI. Consulta il log per il motivo.",
        "polish": "Nie można zapisać INI. Sprawdź log, aby dowiedzieć się dlaczego.",
        "czech": "Nelze zapsat INI. Důvod najdete v logu.",
    },
    "COLU_HelpSave": {
        "japanese": "このページのすべての設定をプラグインのINIに書き込み、再起動後も残します。",
        "korean": "이 페이지의 모든 설정을 플러그인의 INI에 기록하여 재시작 후에도 유지되게 합니다.",
        "chinese": "将此页面的所有设置写入插件的 INI,使其在重启后仍然保留。",
        "russian": "Записывает каждую настройку этой страницы в INI плагина, чтобы она сохранилась после перезапуска.",
        "german": "Schreibt jede Einstellung dieser Seite in die INI des Plugins, damit sie einen Neustart überlebt.",
        "french": "Écrit chaque paramètre de cette page dans le fichier INI du plugin afin qu'il survive à un redémarrage.",
        "spanish": "Escribe cada ajuste de esta página en el INI del plugin para que sobreviva a un reinicio.",
        "italian": "Scrive ogni impostazione di questa pagina nell'INI del plugin, così sopravvive a un riavvio.",
        "polish": "Zapisuje każde ustawienie tej strony do pliku INI wtyczki, dzięki czemu przetrwa restart.",
        "czech": "Zapíše každé nastavení této stránky do INI pluginu, aby přežilo restart.",
    },
    "COLU_ReloadBtn": {
        "japanese": "INIから再読み込み", "korean": "INI에서 다시 불러오기", "chinese": "从 INI 重新加载",
        "russian": "Перезагрузить из INI", "german": "Aus INI neu laden", "french": "Recharger depuis l'INI",
        "spanish": "Recargar desde el INI", "italian": "Ricarica dall'INI", "polish": "Wczytaj ponownie z INI",
        "czech": "Znovu načíst z INI",
    },
    "COLU_StatusReloading": {
        "japanese": "再読み込み中...", "korean": "다시 불러오는 중...", "chinese": "正在重新加载...",
        "russian": "Перезагрузка...", "german": "Wird neu geladen...", "french": "Rechargement...",
        "spanish": "Recargando...", "italian": "Ricaricamento...", "polish": "Wczytywanie ponowne...",
        "czech": "Znovu se načítá...",
    },
    "COLU_StatusReloaded": {
        "japanese": "INIから設定を再読み込みしました。", "korean": "INI에서 설정을 다시 불러왔습니다.",
        "chinese": "已从 INI 重新加载设置。", "russian": "Настройки перезагружены из INI.",
        "german": "Einstellungen aus der INI neu geladen.", "french": "Paramètres rechargés depuis l'INI.",
        "spanish": "Ajustes recargados desde el INI.", "italian": "Impostazioni ricaricate dall'INI.",
        "polish": "Ustawienia wczytane ponownie z INI.", "czech": "Nastavení znovu načtena z INI.",
    },
    "COLU_StatusReloadFail": {
        "japanese": "INIの読み込みに失敗しました。理由はログを確認してください。",
        "korean": "INI를 읽을 수 없습니다. 이유는 로그를 확인하세요.",
        "chinese": "无法读取 INI。请查看日志了解原因。",
        "russian": "Не удалось прочитать INI. Причина — в журнале.",
        "german": "Die INI konnte nicht gelesen werden. Der Grund steht im Log.",
        "french": "Impossible de lire l'INI. Voyez le journal pour la raison.",
        "spanish": "No se pudo leer el INI. Consulta el registro para saber por qué.",
        "italian": "Impossibile leggere l'INI. Consulta il log per il motivo.",
        "polish": "Nie można odczytać INI. Sprawdź log, aby dowiedzieć się dlaczego.",
        "czech": "Nelze přečíst INI. Důvod najdete v logu.",
    },
    "COLU_HelpReload": {
        "japanese": "最後の保存以降にここで行った変更をすべて捨て、INIをディスクから再読み込みします。",
        "korean": "마지막 저장 이후 여기서 만든 변경 사항을 모두 버리고 INI를 디스크에서 다시 읽습니다.",
        "chinese": "放弃自上次保存以来在此处所做的任何更改,并从磁盘重新读取 INI。",
        "russian": "Отбрасывает все изменения, сделанные здесь с последнего сохранения, и заново считывает INI с диска.",
        "german": "Verwirft jede hier seit dem letzten Speichern vorgenommene Änderung und liest die INI erneut von der Festplatte.",
        "french": "Annule tout changement effectué ici depuis le dernier enregistrement et relit l'INI depuis le disque.",
        "spanish": "Descarta cualquier cambio hecho aquí desde el último guardado y vuelve a leer el INI desde el disco.",
        "italian": "Scarta ogni modifica fatta qui dall'ultimo salvataggio e rilegge l'INI dal disco.",
        "polish": "Odrzuca wszelkie zmiany wprowadzone tutaj od ostatniego zapisu i ponownie odczytuje INI z dysku.",
        "czech": "Zahodí všechny změny provedené zde od posledního uložení a znovu načte INI z disku.",
    },
    "COLU_RestoreBtn": {
        "japanese": "既定値に戻す", "korean": "기본값으로 복원", "chinese": "恢复默认值", "russian": "Восстановить умолч.",
        "german": "Standard wiederherstellen", "french": "Restaurer les valeurs par défaut",
        "spanish": "Restaurar valores predeterminados", "italian": "Ripristina i valori predefiniti",
        "polish": "Przywróć wartości domyślne", "czech": "Obnovit výchozí",
    },
    "COLU_StatusRestored": {
        "japanese": "既定値に戻しました。保存を押して確定してください。",
        "korean": "기본값으로 복원했습니다. 유지하려면 저장을 누르세요.",
        "chinese": "已恢复默认值。按保存以保留它们。",
        "russian": "Значения по умолчанию восстановлены. Нажмите «Сохранить», чтобы закрепить их.",
        "german": "Standardwerte wiederherstellt. Drücke Speichern, um sie zu behalten.",
        "french": "Valeurs par défaut restaurées. Appuyez sur Enregistrer pour les conserver.",
        "spanish": "Valores predeterminados restaurados. Pulsa Guardar para conservarlos.",
        "italian": "Valori predefiniti ripristinati. Premi Salva per conservarli.",
        "polish": "Przywrócono wartości domyślne. Naciśnij Zapisz, aby je zachować.",
        "czech": "Výchozí hodnoty obnoveny. Stiskněte Uložit, abyste je zachovali.",
    },
    "COLU_HelpRestore": {
        "japanese": "新規インストール時の値にすべての設定を戻します。保存ボタンを押すまで何も書き込まれません。",
        "korean": "새로 설치했을 때의 값으로 모든 설정을 되돌립니다. 저장을 누르기 전까지는 아무것도 기록되지 않습니다.",
        "chinese": "将每个设置恢复为全新安装时的值。在你按下保存之前,不会写入任何内容。",
        "russian": "Возвращает каждую настройку к значению, которое было бы при свежей установке. Ничего не записывается, пока вы не нажмёте «Сохранить».",
        "german": "Setzt jede Einstellung auf den Wert zurück, den sie bei einer frischen Installation hätte. Nichts wird geschrieben, bis du auf Speichern drückst.",
        "french": "Remet chaque paramètre à sa valeur d'une installation neuve. Rien n'est écrit avant que vous n'appuyiez sur Enregistrer.",
        "spanish": "Devuelve cada ajuste al valor que tendría en una instalación nueva. No se escribe nada hasta que pulses Guardar.",
        "italian": "Riporta ogni impostazione al valore che avrebbe in un'installazione nuova. Non viene scritto nulla finché non premi Salva.",
        "polish": "Przywraca każde ustawienie do wartości z nowej instalacji. Nic nie zostaje zapisane, dopóki nie naciśniesz Zapisz.",
        "czech": "Vrátí každé nastavení na hodnotu, jakou by mělo při čerstvé instalaci. Nic se nezapíše, dokud nestisknete Uložit.",
    },
    "COLU_Intro": {
        "japanese": "変更はすぐに適用されます。次回プレイ時にも残すには保存を押してください。",
        "korean": "변경 사항은 즉시 적용됩니다. 다음에 플레이할 때도 유지하려면 저장을 누르세요.",
        "chinese": "更改会立即生效。按保存可在下次游玩时保留它们。",
        "russian": "Изменения применяются сразу же. Нажмите «Сохранить», чтобы они остались и в следующий раз.",
        "german": "Änderungen wirken sofort. Drücke Speichern, um sie für das nächste Mal zu behalten.",
        "french": "Les changements s'appliquent dès que vous les faites. Appuyez sur Enregistrer pour les garder la prochaine fois.",
        "spanish": "Los cambios se aplican en cuanto los haces. Pulsa Guardar para conservarlos la próxima vez que juegues.",
        "italian": "Le modifiche si applicano non appena le fai. Premi Salva per conservarle per la prossima partita.",
        "polish": "Zmiany obowiązują natychmiast po ich wprowadzeniu. Naciśnij Zapisz, aby zachować je na następną rozgrywkę.",
        "czech": "Změny se použijí okamžitě, jak je provedete. Stiskněte Uložit, abyste je zachovali pro příští hraní.",
    },
}


def write_translation_file(path, entries):
    lines = []
    for key, text in entries.items():
        escaped = text.replace("\r\n", "\n").replace("\n", "\\n")
        lines.append(f"${key}\t{escaped}")
    body = "\r\n".join(lines) + "\r\n"
    data = b"\xff\xfe" + body.encode("utf-16-le")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def main():
    keys, order = read_keys()
    missing_translation_keys = [k for k in order if k not in TRANSLATIONS]
    if missing_translation_keys:
        raise RuntimeError(f"no translations held for keys found in source: {missing_translation_keys}")
    extra_translation_keys = [k for k in TRANSLATIONS if k not in keys]
    if extra_translation_keys:
        raise RuntimeError(f"translations held for keys no longer in source: {extra_translation_keys}")

    out_dir = os.path.join(REPO, "dist", "Interface", "Translations")
    english = {k: keys[k] for k in order}
    write_translation_file(os.path.join(out_dir, "CarryweightOnLevelUp_english.txt"), english)
    print(f"english: {len(english)} keys")

    for lang in LANGS[1:]:
        translated = {k: TRANSLATIONS[k][lang] for k in order}
        write_translation_file(os.path.join(out_dir, f"CarryweightOnLevelUp_{lang}.txt"), translated)
        print(f"{lang}: {len(translated)} keys written")


if __name__ == "__main__":
    main()
