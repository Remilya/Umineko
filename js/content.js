export const STORAGE_KEY = "umineko-tr-locale";
export const SUPPORTED_LOCALES = ["tr", "en"];

export const RELEASE_INFO = {
  version: "v1.0.0",
  installerUrl:
    "https://github.com/Remilya/Umineko/releases/latest/download/Umineko_TR_Kurulum.exe",
  questionZipUrl:
    "https://github.com/Remilya/Umineko/releases/latest/download/Umineko_Question_TR.zip",
  answerZipUrl:
    "https://github.com/Remilya/Umineko/releases/latest/download/Umineko_Answer_TR.zip",
  releasePageUrl: "https://github.com/Remilya/Umineko/releases/latest",
  repoUrl: "https://github.com/Remilya/Umineko",
  modInstallerUrl:
    "https://wiki.07th-mod.com/Umineko/Umineko-Part-3a-Cross-Platform-Installer/",
  vtUrl:
    "https://www.virustotal.com/gui/file/5480e6ff78934950bd74738686204befe4aaaf61dda4ca94c09894726da73ca5/detection",
  mangaEp1Url:
    "https://manga-tr.com/manga-umineko-no-naku-koro-ni-episode-1-legend-of-the-golden-witch.html",
  mangaEp2Url:
    "https://manga-tr.com/manga-umineko-no-naku-koro-ni-episode-2-turn-of-the-golden-witch.html",
  remilyaMangaUrl: "https://remilya.dev/#translations",
  hashes: {
    installer:
      "5480e6ff78934950bd74738686204befe4aaaf61dda4ca94c09894726da73ca5",
    question:
      "d570db894500e027ef6e2b7446911bcaa1710eb6f68c56f592d665539f3dd990",
    answer:
      "63607af7f0efcf5089eb22c2c836ff3852476f0cfd169749942e2deb1004e5f5"
  }
};

export const truthQuotes = [
  {
    id: "red",
    tone: "red",
    bust: "assets/bust-bea.webp",
    badge: {
      tr: "KIRMIZI GERÇEK",
      en: "RED TRUTH",
      ja: "赤き真実"
    },
    speaker: {
      tr: "BEATRICE · ALTIN CADI",
      en: "BEATRICE · THE GOLDEN WITCH",
      ja: "ベアトリーチェ · 黄金の魔女"
    },
    lines: {
      tr: "Dinle ve hatırla! Kırmızıyla söylediğim her şey mutlak gerçektir! Kanıta ya da tartışmaya asla ihtiyaç duymaz!",
      en: "Listen closely! Everything I speak in red is the absolute truth! There is zero need for proof or debate!",
      ja: "よく聞け。妾が赤で語ることは全て真実だ！　証拠も議論も一切不要！"
    }
  },
  {
    id: "blue",
    tone: "blue",
    bust: "assets/bust-but.webp",
    badge: {
      tr: "MAVİ GERÇEK",
      en: "BLUE TRUTH",
      ja: "青き真実"
    },
    speaker: {
      tr: "USHIROMIYA BATTLER · RAKİP",
      en: "BATTLER USHIROMIYA · OPPONENT",
      ja: "右代宮 戦人 · 対局者"
    },
    lines: {
      tr: "Satranç tahtasını tersine çeviriyorum! Cadı illüzyonunu paramparça edecek teorimi mavi kamayla saplayacağım!",
      en: "Let me flip the chessboard over! I'll drive my theory in with a blue wedge and shatter the witch's illusion!",
      ja: "チェス盤をひっくり返すぜ！　魔女の幻想を打ち砕く仮説を、青き楔で突き立ててやる！"
    }
  },
  {
    id: "gold",
    tone: "gold",
    bust: "assets/bust-dlanor.webp",
    badge: {
      tr: "ALTIN GERÇEK",
      en: "GOLDEN TRUTH",
      ja: "黄金の真実"
    },
    speaker: {
      tr: "DLANOR A. KNOX · BAŞ SORUŞTURMACI",
      en: "DLANOR A. KNOX · CHIEF INQUISITOR",
      ja: "ドラノール・A・ノックス · 異端審問官"
    },
    lines: {
      tr: "Yalnızca oyun tahtasının tüm kurallarını kavrayanların ulaşabildiği, kimi zaman kırmızıdan bile üstün hakikat.",
      en: "A truth forged only by one who grasps every rule of the game board, at times surpassing even the red.",
      ja: "ゲーム盤の全ての理を理解した者だけが紡げる、時に赤をも凌駕する黄金の真実です。"
    }
  },
  {
    id: "purple",
    tone: "purple",
    bust: "assets/bust-ber.webp",
    badge: {
      tr: "MOR BEYAN",
      en: "PURPLE DECLARATION",
      ja: "紫の発言"
    },
    speaker: {
      tr: "BERNKASTEL · MUCİZELER CADISI",
      en: "BERNKASTEL · WITCH OF MIRACLES",
      ja: "ベルンカステル · 奇跡の魔女"
    },
    lines: {
      tr: "Mor beyanlar kırmızı gerçek kadar kesindir, tek bir istisnayla: Yalnızca suçlu morla yalan söyleyebilir.",
      en: "Purple declarations carry the certainty of the red truth, with one exception: only the culprit may lie in purple.",
      ja: "紫の発言は赤き真実と同等の効力を持つわ。ただ一つの例外――『犯人だけは嘘をつける』を除いてね。"
    }
  }
];

export const galleryItems = [
  {
    id: "kanji",
    thumb: "assets/shot-kanji-tr.webp",
    hd: "assets/shot-kanji-tr-hd.webp",
    tab: { tr: "Kanji Uyumu", en: "Kanji Support" },
    caption: {
      tr: "1. Bölüm · İsim açıklamaları ve kelime oyunlarındaki Japonca Kanji karakterleri (右代宮 戦人) Türkçe satır içinde bozulmadan görünür.",
      en: "Episode 1 · Japanese Kanji characters used in name explanations and wordplay render cleanly inline alongside Turkish text."
    }
  },
  {
    id: "red",
    thumb: "assets/shot-red-tr.webp",
    hd: "assets/shot-red-tr-hd.webp",
    tab: { tr: "Kırmızı Gerçek", en: "Red Truth" },
    caption: {
      tr: "2. Bölüm · Kırmızı Gerçek sahneleri ve Türkçe ADV diyalog penceresi.",
      en: "Episode 2 · Red Truth logic clashes inside the Turkish ADV dialogue window."
    }
  },
  {
    id: "puzzle",
    thumb: "assets/shot-puzzle-tr.webp",
    hd: "assets/shot-puzzle-tr-hd.webp",
    tab: { tr: "8. Bölüm Bilmece", en: "Ep.8 Riddles" },
    caption: {
      tr: "8. Bölüm · Cadılar Bayramı partisindeki görsel bilmece kartları ve Bernkastel'in gizem tahtası.",
      en: "Episode 8 · Visual Halloween party riddle cards and Bernkastel's mystery board in Turkish."
    }
  },
  {
    id: "scroll",
    thumb: "assets/shot-scroll-a-tr.webp",
    hd: "assets/shot-scroll-a-tr-hd.webp",
    tab: { tr: "Kapanış Ekranı", en: "Ending Scroll" },
    caption: {
      tr: "Bölüm sonu parşömenleri ve Chiru ('Hakikat ve İllüzyonların Noktürnü') için yeniden çizilen Türkçe logolar.",
      en: "Redrawn episode ending scrolls and Turkish title logos for Question & Answer Arcs."
    }
  }
];

export const remilyaRoutes = [
  { label: "remilya.dev", href: "https://remilya.dev/" },
  { label: "/Higurashi", href: "https://remilya.dev/Higurashi/" },
  { label: "/Gensokyo", href: "https://remilya.dev/Gensokyo/" },
  { label: "/Raidou", href: "https://remilya.dev/Raidou/" },
  { label: "/ScarletSavior", href: "https://remilya.dev/ScarletSavior/" },
  { label: "/Voicevox", href: "https://remilya.dev/Voicevox/" }
];

export const content = {
  tr: {
    meta: {
      title: "Umineko: Martılar Ağladığında | Question & Answer Arcs Türkçe Yama",
      description:
        "Umineko When They Cry Question ve Answer Arcs (1-8. Bölümler) için 07th-Mod uyumlu Türkçe yama. PS3 seslendirme, 1080p grafik desteği ve tek tıkla kurulum aracı.",
      ogLocale: "tr_TR"
    },
    aria: {
      skipLink: "Ana içeriğe geç",
      brandLink: "En üste dön",
      primaryNav: "Ana gezinme",
      dotsNav: "Sayfa bölümleri",
      localeSwitcher: "Site dili",
      languageChanged: "Dil Türkçe olarak değiştirildi.",
      closeModal: "Görseli kapat",
      mangaVol1: "Umineko Episode 1 Türkçe Manga Oku",
      mangaVol2: "Umineko Episode 2 Türkçe Manga Oku"
    },
    brandSubtitle: "Question & Answer Arcs · Türkçe Yama",
    nav: {
      top: "Giriş",
      hakikat: "Özellikler",
      bolumler: "Bölümler",
      kurulum: "Kurulum",
      hakkinda: "Hakkında"
    },
    panelLabels: {
      top: "Giriş",
      hakikat: "Özellikler",
      bolumler: "Bölümler",
      kurulum: "Kurulum",
      hakkinda: "Hakkında"
    },
    hero: {
      kicker: "QUESTION & ANSWER ARCS · 1-8. BÖLÜMLER",
      cap1: "U",
      word1: "mineko",
      sep: ":",
      cap2: "M",
      word2: "artılar Ağladığında",
      lead: "Rokkenjima'daki sekiz oyun tahtasını PS3 seslendirmesi ve 1080p grafiklerle Türkçeye taşıyan yama.",
      sub: "“Aşk olmadan gerçeği göremezsin.” Umineko'nun 1. ve 2. Bölüm mangalarıyla Higurashi serisini çeviren Remilya'dan, görsel romanın Question ve Answer Arcs bölümleri.",
      ctaPrimary: "Kurulum Aracını İndir (v1.0.0)",
      ctaSecondary: "Kurulum Rehberi",
      quickLinks: [
        { label: "Ep. 1 & 2 Manga Çevirisi", href: "#manga-slice", external: false },
        { label: "07th-Mod Ayar Tablosu", href: "#kurulum", external: false },
        {
          label: "VirusTotal Raporu",
          href: RELEASE_INFO.vtUrl,
          external: true
        }
      ],
      scrollHint: "Aşağı kaydır veya ↓ tuşuna bas",
      badges: [
        { code: "EP. 1-8", label: "SORU VE CEVAP" },
        { code: "07TH-MOD", label: "PS3 SES · 1080P" },
        { code: "MANGA + VN", label: "REMILYA ÇEVİRİSİ" }
      ]
    },
    slices: {
      srTitle: "Yamanın Özellikleri",
      compBanner: "DÖRT BAŞLIK · BİRİNİ SEÇ",
      compLaunch: "İncele ↗",
      compLaunchPrefix: "Aç:",
      truthModeLabel: "Hakikat Türü",
      truthLangLabel: "Metin Dili",
      truthTabs: {
        red: "Kırmızı",
        blue: "Mavi",
        gold: "Altın",
        purple: "Mor"
      },
      zoomHint: "Tam ekran görmek için görsele tıkla",
      items: [
        {
          k: 1,
          eyebrow: "I · MANTIK VE KAPALI ODA",
          cap: "K",
          restTitle: "ırmızı, Mavi, Altın ve Mor Gerçekler",
          shortLabel: "Renkli Gerçekler",
          compSub: "Mantık ve Kapalı Oda",
          primaryLabel: "Bölümleri İncele ↓",
          primaryHref: "#bolumler",
          primaryExternal: false,
          secondaryLabel: "Kuruluma Geç ↓",
          secondaryHref: "#kurulum",
          secondaryExternal: false
        },
        {
          k: 2,
          eyebrow: "II · QUESTION & ANSWER ARCS",
          cap: "Q",
          restTitle: "uestion ve Answer Arcs Bir Arada",
          pills: [
            "Question Arcs (Ep. 1-4)",
            "Answer Arcs / Chiru (Ep. 5-8)",
            "Çay Partileri ve ????",
            "Steam · GOG"
          ],
          shortLabel: "Question & Answer",
          compSub: "1-8. Bölümler",
          primaryLabel: "8 Bölümü Gör ↓",
          primaryHref: "#bolumler",
          primaryExternal: false,
          secondaryLabel: "Kurulum Rehberi ↓",
          secondaryHref: "#kurulum",
          secondaryExternal: false
        },
        {
          k: 3,
          eyebrow: "III · GÖRSEL UYARLAMA VE TİPOGRAFİ",
          cap: "O",
          restTitle: "yun İçi Görseller ve Kanji Uyumu",
          shortLabel: "Görsel ve Arayüz",
          compSub: "Menüler ve Bilmeceler",
          primaryLabel: "Görseli Büyüt",
          primaryHref: "#zoom",
          primaryExternal: false,
          secondaryLabel: "İndirme Merkezi ↓",
          secondaryHref: "#kurulum",
          secondaryExternal: false
        },
        {
          k: 4,
          eyebrow: "IV · MANGA UYARLAMALARI",
          cap: "E",
          restTitle: "p. 1 ve Ep. 2 Mangalarının Çevirmeninden",
          badge: "Türkçe Çeviri ve Balon Dizgisi: Remilya",
          shortLabel: "Manga Çevirisi",
          compSub: "Ep. 1 ve Ep. 2 Türkçe Manga",
          primaryLabel: "Ep. 1 Mangayı Oku ↗",
          primaryHref: RELEASE_INFO.mangaEp1Url,
          primaryExternal: true,
          secondaryLabel: "Ep. 2 Mangayı Oku ↗",
          secondaryHref: RELEASE_INFO.mangaEp2Url,
          secondaryExternal: true
        }
      ]
    },
    episodes: {
      eyebrow: "ROKKENJIMA · 4 EKİM 1986",
      cap: "B",
      restTitle: "ölüm Rehberi (Ep. 1-8)",
      prevAria: "Önceki bölüm",
      nextAria: "Sonraki bölüm",
      statusLabel: "Question & Answer Arcs · Çay Partisi ve ???? Dahil",
      ctaLabel: "Yamayı İndir ↓",
      items: [
        {
          ep: "1. Bölüm · QUESTION ARCS",
          shortTab: "Ep.1",
          title: "Altın Cadı Efsanesi",
          enSub: "Legend of the Golden Witch · 黄金の魔女の伝説",
          stats: "4 Ekim 1986 · Açılış Tahtası",
          truths: ["Kitabe Bilmecesi", "Kapalı Oda", "TR Manga Çevirisi"],
          note: "Ushiromiya ailesi yıllık konferans için Rokkenjima'da toplanır. Tayfun adayı dış dünyadan koparınca Altın Cadı Beatrice'in mektubu okunur ve ilk kapalı oda cinayetleri işlenir."
        },
        {
          ep: "2. Bölüm · QUESTION ARCS",
          shortTab: "Ep.2",
          title: "Altın Cadının Sırası",
          enSub: "Turn of the Golden Witch · 黄金の魔女の順番",
          stats: "Cadı ve İnsan Düellosu",
          truths: ["Kırmızı Gerçek", "Üst Düzlem", "TR Manga Çevirisi"],
          note: "Beatrice artık gizlenmez, satranç tahtasının ötesinde Battler'ın karşısına dikilir. Kanıta gerek duymayan Kırmızı Gerçek bu bölümde oyuna girer."
        },
        {
          ep: "3. Bölüm · QUESTION ARCS",
          shortTab: "Ep.3",
          title: "Altın Cadının Şöleni",
          enSub: "Banquet of the Golden Witch · 黄金の魔女の宴",
          stats: "Altın Diyarın Kapısı",
          truths: ["Virgilia ve Ronove", "Şeytanın Kanıtı"],
          note: "Yetişkinler kitabenin sırrını çözmeye yaklaşırken Beatrice'in eski hocası Virgilia ve kâhyası Ronove tahtaya iner. Kırmızı gerçek çemberi daraldıkça Battler savunmasını baştan kurar."
        },
        {
          ep: "4. Bölüm · QUESTION ARCS",
          shortTab: "Ep.4",
          title: "Altın Cadının İttifakı",
          enSub: "Alliance of the Golden Witch · 黄金の魔女の同盟",
          stats: "1998 · Soru Arkları Finali",
          truths: ["Mavi Gerçek", "Ange Ushiromiya"],
          note: "1998'de hayatta kalan tek aile üyesi Ange Ushiromiya, on iki yıl önceki günün peşine düşer. Battler ise cadı illüzyonlarını insan açıklamalarıyla delmek için Mavi Gerçek kamalarını kullanır."
        },
        {
          ep: "5. Bölüm · ANSWER ARCS (CHIRU)",
          shortTab: "Ep.5",
          title: "Altın Cadının Sonu",
          enSub: "End of the Golden Witch · 黄金の魔女の終焉",
          stats: "Cevap Arkları Başlangıcı",
          truths: ["Furudo Erika", "Knox'un On Emri", "Altın Gerçek"],
          note: "Cevap Arkları başlar. Bernkastel'in piyonu dedektif Furudo Erika ile Cennetsel Mahkeme'den Dlanor A. Knox, Knox'un On Emri'yle tahtaya dahil olur."
        },
        {
          ep: "6. Bölüm · ANSWER ARCS (CHIRU)",
          shortTab: "Ep.6",
          title: "Altın Cadının Şafağı",
          enSub: "Dawn of the Golden Witch · 黄金の魔女の夜明け",
          stats: "Yeni Oyun Kurucunun Sınavı",
          truths: ["Mühürlü Oda", "Mantık Hatası"],
          note: "Yeni Oyun Kurucu kendi tahtasını kurar. Ancak zincirli kapalı oda tuzağı ve Erika'nın mantık kuşatması, oyunu geri dönüşü olmayan bir düelloya sürükler."
        },
        {
          ep: "7. Bölüm · ANSWER ARCS (CHIRU)",
          shortTab: "Ep.7",
          title: "Altın Cadının Ağıtı",
          enSub: "Requiem of the Golden Witch · 黄金の魔女の鎮魂歌",
          stats: "Rokkenjima'nın Saklı Tarihi",
          truths: ["Willard ve Lion", "Gizem Tahtası"],
          note: "Willard H. Wright ve Ushiromiya Lion, Beatrice'in geçmişini ve ailenin saklı tarihini gün yüzüne çıkarır. Bölüm sonunda Bernkastel kendi etkileşimli gizem oyununu başlatır."
        },
        {
          ep: "8. Bölüm · ANSWER ARCS (CHIRU)",
          shortTab: "Ep.8",
          title: "Altın Cadının Alacakaranlığı",
          enSub: "Twilight of the Golden Witch · 黄金の魔女の黄昏",
          stats: "Rokkenjima'nın Son Perdesi",
          truths: ["Mor Beyan", "Cadılar Bayramı Bilmeceleri", "Çift Son"],
          note: "Ange için hazırlanan son Cadılar Bayramı partisinde bilmece kartları ve Mor Beyan oyunu açılır. Rokkenjima'nın son perdesi senin vereceğin kararla kapanır."
        }
      ]
    },
    install: {
      eyebrow: "SÜRÜM v1.0.0 · 07TH-MOD UYUMLU",
      cap: "Ü",
      restTitle: "ç Adımda Kurulum",
      copy: "Yamayı kurmadan önce Steam veya GOG sürümünün üzerine 07th-Mod (PS3 seslendirme ve 1080p grafik paketi) kurulu olmalı.",
      dlTitle: "1 · Kurulum Aracı ve Paketler",
      exeTitle: "Umineko_TR_Kurulum.exe",
      exeMeta: "Önerilen · Windows (64-bit) · Otomatik Yedekli",
      exeDesc: "Steam ve GOG klasörlerini otomatik bulur, 07th-Mod kurulumunu doğrular ve dosyaları yedekleyerek yamayı kurar.",
      exeBtn: "Kurulum Aracını İndir",
      llmNote: "Çeviride LLM (Argon 4, Carbon 4 ve Gemini) kullanılmıştır. Bununla probleminiz varsa lütfen yamayı kullanmayınız.",
      vtBadge: "VirusTotal Temiz ↗",
      copyHash: "SHA-256 Kopyala",
      copiedHash: "Kopyalandı ✓",
      manualTitle: "Manuel ZIP Paketleri (İsteğe Bağlı · LINUX & STEAM DECK)",
      qZipLabel: "Question Arcs (Ep. 1-4) Manuel Kurulum ZIP",
      aZipLabel: "Answer Arcs / Chiru (Ep. 5-8) Manuel Kurulum ZIP",
      matrixTitle: "2 · Gerekli 07th-Mod Ayarları",
      matrixNote: "07th-Mod Kurulum Aracında şu seçenekleri işaretle:",
      modBtn: "07th-Mod İndir ↗",
      colSetting: "AYAR",
      colQuestion: "QUESTION (1-4)",
      colAnswer: "CHIRU (5-8)",
      rows: [
        {
          setting: "Choose Patch",
          q: "full Patch",
          a: "adv-mode (Zorunlu!)",
          highlightA: true
        },
        {
          setting: "Translations",
          q: "English / Japanese",
          a: "English / Japanese",
          highlightA: false
        },
        {
          setting: "Experimental Script",
          q: "No Change",
          a: "No Change",
          highlightA: false
        },
        {
          setting: "Sprites / BGM",
          q: "PS3 Sprites (Önerilir)",
          a: "PS3 Sprites (Önerilir)",
          highlightA: false
        }
      ],
      step3: "3 · Umineko_TR_Kurulum.exe aracını açıp 'Yamayı Kur' düğmesine bas. İstediğin zaman 'Yedeğe Dön' ile İngilizce 07th-Mod dosyalarına geri dönebilirsin."
    },
    about: {
      eyebrow: "REMILYA.DEV · BAĞIMSIZ ÇEVİRİ",
      cap: "H",
      restTitle: "akkında ve Diğer Çeviriler",
      copy: "Kâr amacı gütmeyen hayran çevirisi, orijinal oyun dosyalarını içermez. Çeviride LLM (Argon 4, Carbon 4 ve Gemini) kullanılmıştır. Bununla probleminiz varsa lütfen yamayı kullanmayınız.",
      creditsTitle: "EMEK VE TEŞEKKÜR",
      creditsText: "Orijinal Eser: 07th Expansion / Ryukishi07 · PS3 Uyarlaması: Alchemist · Motor ve Grafik Altyapısı: 07th-Mod Topluluğu · Türkçe Çeviri, Görsel Uyarlama ve Kurulum Aracı: Remilya.",
      cards: [
        {
          icon: "i-book",
          label: "Umineko Ep. 1 & Ep. 2 Türkçe Manga",
          handle: "remilya.dev/#translations",
          desc: "1. ve 2. Bölüm mangalarının Türkçe çevirisi",
          href: RELEASE_INFO.remilyaMangaUrl
        },
        {
          icon: "i-quill",
          label: "Higurashi When They Cry TR",
          handle: "remilya.dev/Higurashi",
          desc: "Ana ve ek bölümler dahil Higurashi Türkçe yaması",
          href: "https://remilya.dev/Higurashi/"
        },
        {
          icon: "i-gh",
          label: "GitHub Deposu ve Sürümler",
          handle: "Remilya/Umineko",
          desc: "Kurulum aracı kaynak kodu, sürüm paketleri ve hata bildirimi",
          href: RELEASE_INFO.repoUrl
        },
        {
          icon: "i-torii",
          label: "Remilya Ana Sayfa",
          handle: "remilya.dev",
          desc: "RAIDOU TR, Gensokyo 3D Atlas, diğer çeviriler ve açık kaynak araçlar",
          href: "https://remilya.dev/"
        }
      ],
      routesTitle: "REMILYA.DEV KISA BAĞLANTILAR",
      footerNote: "UMINEKO QUESTION & ANSWER ARCS TÜRKÇE YAMA · REMILYA.DEV/UMINEKO",
      backTop: "Başa Dön ↑"
    }
  },

  en: {
    meta: {
      title: "Umineko When They Cry | Question & Answer Arcs Turkish Patch",
      description:
        "07th-Mod compatible Turkish patch for Umineko When They Cry Question & Answer Arcs (Episodes 1-8), from the Turkish translator of the Ep. 1 & Ep. 2 manga.",
      ogLocale: "en_US"
    },
    aria: {
      skipLink: "Skip to main content",
      brandLink: "Back to top",
      primaryNav: "Main navigation",
      dotsNav: "Page sections",
      localeSwitcher: "Site language",
      languageChanged: "Language switched to English.",
      closeModal: "Close image preview",
      mangaVol1: "Read Umineko Episode 1 Manga in Turkish",
      mangaVol2: "Read Umineko Episode 2 Manga in Turkish"
    },
    brandSubtitle: "Question & Answer Arcs · Turkish Patch",
    nav: {
      top: "Home",
      hakikat: "Features",
      bolumler: "Episodes",
      kurulum: "Install",
      hakkinda: "Credits"
    },
    panelLabels: {
      top: "Home",
      hakikat: "Features",
      bolumler: "Episodes",
      kurulum: "Install",
      hakkinda: "Credits"
    },
    hero: {
      kicker: "QUESTION & ANSWER ARCS · EPISODES 1-8",
      cap1: "U",
      word1: "mineko",
      sep: ":",
      cap2: "W",
      word2: "hen They Cry TR",
      lead: "Turkish patch for the eight game boards on Rokkenjima, built for 07th-Mod with PS3 voices and 1080p graphics.",
      sub: "“Without love, it cannot be seen.” From Remilya, Turkish translator of the Umineko Ep. 1 & Ep. 2 manga and the Higurashi series.",
      ctaPrimary: "Download Installer (v1.0.0)",
      ctaSecondary: "Installation Guide",
      quickLinks: [
        { label: "Ep. 1 & 2 Manga Translation", href: "#manga-slice", external: false },
        { label: "07th-Mod Settings Matrix", href: "#kurulum", external: false },
        {
          label: "VirusTotal Report",
          href: RELEASE_INFO.vtUrl,
          external: true
        }
      ],
      scrollHint: "Scroll down or press ↓",
      badges: [
        { code: "EP. 1-8", label: "QUESTION & ANSWER" },
        { code: "07TH-MOD", label: "PS3 VOICES · 1080P" },
        { code: "MANGA + VN", label: "BY REMILYA" }
      ]
    },
    slices: {
      srTitle: "Patch Features",
      compBanner: "FOUR FEATURES · PICK ONE",
      compLaunch: "Explore ↗",
      compLaunchPrefix: "Open:",
      truthModeLabel: "Truth Type",
      truthLangLabel: "Script Language",
      truthTabs: {
        red: "Red",
        blue: "Blue",
        gold: "Golden",
        purple: "Purple"
      },
      zoomHint: "Click image to view full-screen",
      items: [
        {
          k: 1,
          eyebrow: "I · LOGIC & CLOSED ROOMS",
          cap: "R",
          restTitle: "ed, Blue, Golden & Purple Truths",
          shortLabel: "Colored Truths",
          compSub: "Logic & Closed Rooms",
          primaryLabel: "Browse Episodes ↓",
          primaryHref: "#bolumler",
          primaryExternal: false,
          secondaryLabel: "Install Guide ↓",
          secondaryHref: "#kurulum",
          secondaryExternal: false
        },
        {
          k: 2,
          eyebrow: "II · QUESTION & ANSWER ARCS",
          cap: "Q",
          restTitle: "uestion & Answer Arcs Included",
          pills: [
            "Question Arcs (Ep. 1-4)",
            "Answer Arcs / Chiru (Ep. 5-8)",
            "Tea Parties & ????",
            "Steam · GOG"
          ],
          shortLabel: "Question & Answer",
          compSub: "Episodes 1-8",
          primaryLabel: "View Episode Guide ↓",
          primaryHref: "#bolumler",
          primaryExternal: false,
          secondaryLabel: "Install Guide ↓",
          secondaryHref: "#kurulum",
          secondaryExternal: false
        },
        {
          k: 3,
          eyebrow: "III · UI, RIDDLES & KANJI",
          cap: "L",
          restTitle: "ocalized UI, Riddles & Kanji Support",
          shortLabel: "UI & Riddles",
          compSub: "Menus & Ending Scrolls",
          primaryLabel: "Enlarge Preview",
          primaryHref: "#zoom",
          primaryExternal: false,
          secondaryLabel: "Download Center ↓",
          secondaryHref: "#kurulum",
          secondaryExternal: false
        },
        {
          k: 4,
          eyebrow: "IV · MANGA ADAPTATIONS",
          cap: "F",
          restTitle: "rom the Ep. 1 & Ep. 2 Manga Translator",
          badge: "Turkish Translation & Typesetting: Remilya",
          shortLabel: "Manga Translation",
          compSub: "Ep. 1 & Ep. 2 Turkish Manga",
          primaryLabel: "Read Ep. 1 Manga (TR) ↗",
          primaryHref: RELEASE_INFO.mangaEp1Url,
          primaryExternal: true,
          secondaryLabel: "Read Ep. 2 Manga (TR) ↗",
          secondaryHref: RELEASE_INFO.mangaEp2Url,
          secondaryExternal: true
        }
      ]
    },
    episodes: {
      eyebrow: "ROKKENJIMA · OCTOBER 4, 1986",
      cap: "E",
      restTitle: "pisode Guide (Ep. 1-8)",
      prevAria: "Previous episode",
      nextAria: "Next episode",
      statusLabel: "Question & Answer Arcs · Tea Party & ???? Included",
      ctaLabel: "Get the Patch ↓",
      items: [
        {
          ep: "Episode 1 · Question Arcs",
          shortTab: "Ep.1",
          title: "Legend of the Golden Witch",
          enSub: "Altın Cadı Efsanesi · 黄金の魔女の伝説",
          stats: "October 4, 1986 · Opening Board",
          truths: ["Epitaph Riddle", "Closed Rooms", "TR Manga Translation"],
          note: "The Ushiromiya family gathers on Rokkenjima for their annual conference. Once a typhoon cuts the island off, Beatrice's letter is read aloud and the first closed-room murders follow."
        },
        {
          ep: "Episode 2 · Question Arcs",
          shortTab: "Ep.2",
          title: "Turn of the Golden Witch",
          enSub: "Altın Cadının Sırası · 黄金の魔女の順番",
          stats: "Witch vs. Human Clash",
          truths: ["Red Truth", "Meta-World", "TR Manga Translation"],
          note: "Beatrice steps out of the shadows to confront Battler across the chessboard, bringing the Red Truth into play for the first time."
        },
        {
          ep: "Episode 3 · Question Arcs",
          shortTab: "Ep.3",
          title: "Banquet of the Golden Witch",
          enSub: "Altın Cadının Şöleni · 黄金の魔女の宴",
          stats: "Door to the Golden Land",
          truths: ["Virgilia & Ronove", "Devil's Proof"],
          note: "As the adults close in on the epitaph's secret, Beatrice's former teacher Virgilia and butler Ronove enter the stage while webs of red truth tighten around Battler."
        },
        {
          ep: "Episode 4 · Question Arcs",
          shortTab: "Ep.4",
          title: "Alliance of the Golden Witch",
          enSub: "Altın Cadının İttifakı · 黄金の魔女の同盟",
          stats: "1998 · Question Arcs Finale",
          truths: ["Blue Truth", "Ange Ushiromiya"],
          note: "In 1998, sole survivor Ange Ushiromiya searches for what happened twelve years prior. Meanwhile, Battler counters the witch's illusions using wedges of Blue Truth."
        },
        {
          ep: "Episode 5 · Answer Arcs (Chiru)",
          shortTab: "Ep.5",
          title: "End of the Golden Witch",
          enSub: "Altın Cadının Sonu · 黄金の魔女の終焉",
          stats: "Answer Arcs Opening",
          truths: ["Furudo Erika", "Knox's Decalogue", "Golden Truth"],
          note: "The Answer Arcs begin. Bernkastel's piece Furudo Erika and Chief Inquisitor Dlanor A. Knox step onto the board armed with Knox's Decalogue."
        },
        {
          ep: "Episode 6 · Answer Arcs (Chiru)",
          shortTab: "Ep.6",
          title: "Dawn of the Golden Witch",
          enSub: "Altın Cadının Şafağı · 黄金の魔女の夜明け",
          stats: "Trial of the New Game Master",
          truths: ["Chain-Locked Room", "Logic Error"],
          note: "A new Game Master builds their own board. Soon, a chain-locked room trap and Erika's logic siege push the game into a point-of-no-return duel."
        },
        {
          ep: "Episode 7 · Answer Arcs (Chiru)",
          shortTab: "Ep.7",
          title: "Requiem of the Golden Witch",
          enSub: "Altın Cadının Ağıtı · 黄金の魔女の鎮魂歌",
          stats: "Hidden History of Rokkenjima",
          truths: ["Willard & Lion", "Mystery Board"],
          note: "Willard H. Wright and Ushiromiya Lion bring Beatrice's past and the family's hidden history to light before Bernkastel opens her interactive mystery game."
        },
        {
          ep: "Episode 8 · Answer Arcs (Chiru)",
          shortTab: "Ep.8",
          title: "Twilight of the Golden Witch",
          enSub: "Altın Cadının Alacakaranlığı · 黄金の魔女の黄昏",
          stats: "Curtain Call of Rokkenjima",
          truths: ["Purple Declarations", "Halloween Riddles", "Dual Endings"],
          note: "At a final Halloween party held for Ange, riddle cards and Bernkastel's Purple Declarations set the stage for your final choice on Rokkenjima."
        }
      ]
    },
    install: {
      eyebrow: "RELEASE v1.0.0 · 07TH-MOD READY",
      cap: "T",
      restTitle: "hree-Step Installation",
      copy: "Install 07th-Mod (PS3 voices and 1080p graphics) on your Steam or GOG copy before running this patch.",
      dlTitle: "1 · Standalone Installer & Archives",
      exeTitle: "Umineko_TR_Kurulum.exe",
      exeMeta: "Recommended · Windows (64-bit) · Auto-Backup",
      exeDesc: "Finds Steam and GOG folders automatically, verifies 07th-Mod, and backs up files before applying the patch.",
      exeBtn: "Download Installer",
      llmNote: "LLMs (Argon 4, Carbon 4, and Gemini) were used in this translation. If you have an issue with that, please do not use this patch.",
      vtBadge: "VirusTotal Clean ↗",
      copyHash: "Copy SHA-256",
      copiedHash: "Copied ✓",
      manualTitle: "Manual ZIP Packages (Optional · Linux & Steam Deck)",
      qZipLabel: "Question Arcs (Ep. 1-4) Manual Patch ZIP",
      aZipLabel: "Answer Arcs / Chiru (Ep. 5-8) Manual Patch ZIP",
      matrixTitle: "2 · Required 07th-Mod Settings",
      matrixNote: "Pick these options inside the 07th-Mod Installer:",
      modBtn: "Get 07th-Mod ↗",
      colSetting: "Option",
      colQuestion: "Question (1-4)",
      colAnswer: "Chiru (5-8)",
      rows: [
        {
          setting: "Choose Patch",
          q: "full Patch",
          a: "adv-mode (Required!)",
          highlightA: true
        },
        {
          setting: "Translations",
          q: "English / Japanese",
          a: "English / Japanese",
          highlightA: false
        },
        {
          setting: "Experimental Script",
          q: "No Change",
          a: "No Change",
          highlightA: false
        },
        {
          setting: "Sprites / BGM",
          q: "PS3 Sprites (Recommended)",
          a: "PS3 Sprites (Recommended)",
          highlightA: false
        }
      ],
      step3: "3 · Open Umineko_TR_Kurulum.exe and click 'Yamayı Kur'. You can revert to English 07th-Mod anytime via 'Yedeğe Dön'."
    },
    about: {
      eyebrow: "REMILYA.DEV · FAN TRANSLATION",
      cap: "A",
      restTitle: "bout & Other Translations",
      copy: "Free, non-commercial fan patch containing no original game files. LLMs (Argon 4, Carbon 4, and Gemini) were used in this translation. If you have an issue with that, please do not use this patch.",
      creditsTitle: "CREDITS & ACKNOWLEDGEMENTS",
      creditsText: "Original Work: 07th Expansion / Ryukishi07 · PS3 Release: Alchemist · Engine & HD Modding Framework: 07th-Mod Team · Turkish Translation, Image Editing & Installer: Remilya.",
      cards: [
        {
          icon: "i-book",
          label: "Umineko Ep. 1 & Ep. 2 Turkish Manga",
          handle: "remilya.dev/#translations",
          desc: "Episode 1 & 2 manga series translated and typeset into Turkish",
          href: RELEASE_INFO.remilyaMangaUrl
        },
        {
          icon: "i-quill",
          label: "Higurashi When They Cry TR",
          handle: "remilya.dev/Higurashi",
          desc: "Main and bonus arcs Turkish patch for Higurashi When They Cry",
          href: "https://remilya.dev/Higurashi/"
        },
        {
          icon: "i-gh",
          label: "GitHub Repository & Releases",
          handle: "Remilya/Umineko",
          desc: "Installer source code, release packages, and issue reports",
          href: RELEASE_INFO.repoUrl
        },
        {
          icon: "i-torii",
          label: "Remilya Home",
          handle: "remilya.dev",
          desc: "RAIDOU TR, Gensokyo 3D Atlas, other translations, and open-source tools",
          href: "https://remilya.dev/"
        }
      ],
      routesTitle: "REMILYA.DEV SHORT LINKS",
      footerNote: "UMINEKO QUESTION & ANSWER ARCS TURKISH PATCH · REMILYA.DEV/UMINEKO",
      backTop: "Back to Top ↑"
    }
  }
};
