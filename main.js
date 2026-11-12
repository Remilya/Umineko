import {
  STORAGE_KEY,
  SUPPORTED_LOCALES,
  content,
  remilyaRoutes
} from "./js/content.js";
import { initDeck } from "./js/deck.js";
import { initSlices } from "./js/slices.js";
import {
  initEpisodeGrimoire,
  initAtmosphere
} from "./js/showcase.js";
import { initRemiliaCursor } from "./js/cursor.js";

const esc = (s) =>
  String(s).replace(/[&<>"']/g, (c) =>
    ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      '"': "&quot;",
      "'": "&#39;"
    })[c]
  );

function renderLinkAttributes(link) {
  return link.external === false ? "" : ' target="_blank" rel="noopener noreferrer"';
}

function detectInitialLocale() {
  try {
    const params = new URLSearchParams(window.location.search);
    const fromQuery = (params.get("lang") || "").toLowerCase();
    if (SUPPORTED_LOCALES.includes(fromQuery)) {
      return fromQuery;
    }
  } catch {
    /* ignore query parse issues */
  }

  try {
    const stored = window.localStorage.getItem(STORAGE_KEY);
    if (stored && SUPPORTED_LOCALES.includes(stored)) {
      return stored;
    }
  } catch {
    /* ignore storage errors */
  }

  const htmlLang = (document.documentElement.lang || "").toLowerCase();
  if (SUPPORTED_LOCALES.includes(htmlLang)) {
    return htmlLang;
  }

  return "tr";
}

const $ = (sel) => document.querySelector(sel);
const setText = (sel, val) => {
  const el = typeof sel === "string" ? $(sel) : sel;
  if (el && val !== undefined) el.textContent = val;
};

let currentLocale = detectInitialLocale();
let deckApi = null;
let slicesApi = null;
let episodesApi = null;

function applyLink(el, label, href, external, ariaLabel) {
  if (!el) return;
  el.textContent = label;
  el.href = href;
  if (ariaLabel) {
    el.setAttribute("aria-label", ariaLabel);
  }
  if (external === false) {
    el.removeAttribute("target");
    el.removeAttribute("rel");
  } else {
    el.target = "_blank";
    el.rel = "noopener noreferrer";
  }
}

function renderHero(dict) {
  setText("#hero-kicker", dict.hero.kicker);
  const h1 = $("#hero-title");
  if (h1) {
    h1.innerHTML = `<span class="cap">${esc(dict.hero.cap1)}</span>${esc(dict.hero.word1)}<span class="title-dot">${esc(dict.hero.sep)}</span> <span class="cap cap--crimson">${esc(dict.hero.cap2)}</span>${esc(dict.hero.word2)}`;
  }
  setText("#hero-lead", dict.hero.lead);
  setText("#hero-sub", dict.hero.sub);
  setText("#hero-cta-primary-text", dict.hero.ctaPrimary);
  setText("#hero-cta-secondary-text", dict.hero.ctaSecondary);
  setText("#hero-scroll-hint", dict.hero.scrollHint);

  const quickEl = $("#hero-quicklinks");
  if (quickEl) {
    quickEl.innerHTML = dict.hero.quickLinks
      .map(
        (link) =>
          `<a class="hero-qlink" href="${esc(link.href)}"${renderLinkAttributes(link)}><span>${esc(link.label)}</span><svg aria-hidden="true"><use href="#i-arrow"/></svg></a>`
      )
      .join("");
  }

  const badgesEl = $("#hero-badges");
  if (badgesEl) {
    badgesEl.innerHTML = dict.hero.badges
      .map(
        (b) =>
          `<div class="mascot-chip"><strong>${esc(b.code)}</strong><span>${esc(b.label)}</span></div>`
      )
      .join("");
  }
}

function renderSlicesCopy(dict) {
  setText("#slices-sr-title", dict.slices.srTitle);
  setText("#slices-comp-banner", dict.slices.compBanner);
  setText("#truth-mode-label", dict.slices.truthModeLabel);
  setText("#truth-lang-label", dict.slices.truthLangLabel);
  setText("#gal-zoom-hint", dict.slices.zoomHint);

  dict.slices.items.forEach((item) => {
    const k = item.k;
    setText(`#sl${k}-eyebrow`, item.eyebrow);
    const titleEl = $(`#sl${k}-t`);
    if (titleEl) {
      titleEl.innerHTML = `<b class="cap">${esc(item.cap)}</b>${esc(item.restTitle)}`;
    }
    if (item.badge) {
      setText(`#sl${k}-badge`, item.badge);
    }

    applyLink(
      $(`#sl${k}-primary`),
      item.primaryLabel,
      item.primaryHref,
      item.primaryExternal
    );
    applyLink(
      $(`#sl${k}-secondary`),
      item.secondaryLabel,
      item.secondaryHref,
      item.secondaryExternal
    );

    if (item.pills) {
      const pillsEl = $(`#sl${k}-pills`);
      if (pillsEl) {
        pillsEl.innerHTML = item.pills
          .map((p) => `<li class="sl-pill">${esc(p)}</li>`)
          .join("");
      }
    }

    setText(`#sl${k}-sliv`, item.shortLabel);
    setText(`#sl${k}-lab-title`, item.shortLabel);
    setText(`#sl${k}-lab-sub`, item.compSub);
    const labHref = item.primaryHref === "#zoom" ? "#kurulum" : item.primaryHref;
    const labExt = item.primaryHref === "#zoom" ? false : item.primaryExternal;
    applyLink(
      $(`#sl${k}-lab-cta`),
      dict.slices.compLaunch,
      labHref,
      labExt,
      `${dict.slices.compLaunchPrefix} ${item.shortLabel}`
    );
  });
}

function renderEpisodesHeader(dict) {
  setText("#ep-eyebrow", dict.episodes.eyebrow);
  const titleEl = $("#ep-section-title");
  if (titleEl) {
    titleEl.innerHTML = `<span class="cap cap--crimson">${esc(dict.episodes.cap)}</span>${esc(dict.episodes.restTitle)}`;
  }
}

function renderInstallSection(dict) {
  setText("#inst-eyebrow", dict.install.eyebrow);
  const titleEl = $("#inst-title");
  if (titleEl) {
    titleEl.innerHTML = `<span class="cap">${esc(dict.install.cap)}</span>${esc(dict.install.restTitle)}`;
  }
  setText("#inst-copy", dict.install.copy);
  setText("#inst-dl-title", dict.install.dlTitle);
  setText("#inst-exe-title", dict.install.exeTitle);
  setText("#inst-exe-meta", dict.install.exeMeta);
  setText("#inst-exe-desc", dict.install.exeDesc);
  setText("#inst-exe-btn-text", dict.install.exeBtn);
  setText("#inst-llm-note", dict.install.llmNote);
  setText("#inst-vt-badge", dict.install.vtBadge);
  setText("#inst-manual-title", dict.install.manualTitle);
  setText("#inst-qzip-label", dict.install.qZipLabel);
  setText("#inst-azip-label", dict.install.aZipLabel);
  setText("#inst-matrix-title", dict.install.matrixTitle);
  setText("#inst-matrix-note", dict.install.matrixNote);
  setText("#inst-mod-btn", dict.install.modBtn);
  setText("#inst-col-setting", dict.install.colSetting);
  setText("#inst-col-q", dict.install.colQuestion);
  setText("#inst-col-a", dict.install.colAnswer);
  setText("#inst-step3", dict.install.step3);

  const tbody = $("#inst-matrix-body");
  if (tbody) {
    tbody.innerHTML = dict.install.rows
      .map(
        (r) => `
          <tr>
            <th scope="row">${esc(r.setting)}</th>
            <td><span class="matrix-val">${esc(r.q)}</span></td>
            <td><span class="matrix-val ${r.highlightA ? "matrix-val--alert" : ""}">${esc(r.a)}</span></td>
          </tr>
        `
      )
      .join("");
  }
}

function renderAboutSection(dict) {
  setText("#about-eyebrow", dict.about.eyebrow);
  const titleEl = $("#about-title");
  if (titleEl) {
    titleEl.innerHTML = `<span class="cap">${esc(dict.about.cap)}</span>${esc(dict.about.restTitle)}`;
  }
  setText("#about-copy", dict.about.copy);
  setText("#credits-title", dict.about.creditsTitle);
  setText("#credits-text", dict.about.creditsText);
  setText("#routes-title", dict.about.routesTitle);
  setText("#footer-note", dict.about.footerNote);
  setText("#footer-top-link", dict.about.backTop);

  const socialsEl = $("#about-cards");
  if (socialsEl) {
    socialsEl.innerHTML = dict.about.cards
      .map(
        (soc) => `
          <a class="social-card" href="${esc(soc.href)}" target="_blank" rel="noopener noreferrer">
            <span class="social-card__ico"><svg aria-hidden="true"><use href="#${esc(soc.icon)}"/></svg></span>
            <span class="social-card__body">
              <strong>${esc(soc.label)}</strong>
              <span class="social-card__handle">${esc(soc.handle)}</span>
              <span class="social-card__desc">${esc(soc.desc)}</span>
            </span>
            <svg class="social-card__arr" aria-hidden="true"><use href="#i-arrow"/></svg>
          </a>
        `
      )
      .join("");
  }

  const routesEl = $("#short-routes");
  if (routesEl && !routesEl.dataset.hydrated) {
    routesEl.innerHTML = remilyaRoutes
      .map(
        (r) =>
          `<a class="route-pill" href="${esc(r.href)}">${esc(r.label)}</a>`
      )
      .join("");
    routesEl.dataset.hydrated = "true";
  }
}

function applyLocale(locale, { announce = false, syncQuoteLang = false } = {}) {
  const next = SUPPORTED_LOCALES.includes(locale) ? locale : "tr";
  currentLocale = next;
  const dict = content[next];

  document.documentElement.lang = next;
  document.title = dict.meta.title;

  const setMeta = (sel, val) => {
    const el = $(sel);
    if (el) el.setAttribute("content", val);
  };
  setMeta("#meta-description", dict.meta.description);
  setMeta("#og-locale", dict.meta.ogLocale);
  setMeta("#og-title", dict.meta.title);
  setMeta("#og-description", dict.meta.description);
  setMeta("#twitter-title", dict.meta.title);
  setMeta("#twitter-description", dict.meta.description);

  const skipEl = $("#skip-link");
  if (skipEl) skipEl.textContent = dict.aria.skipLink;
  const brandEl = $("#brand-link");
  if (brandEl) brandEl.setAttribute("aria-label", dict.aria.brandLink);
  const navEl = $("#primary-nav");
  if (navEl) navEl.setAttribute("aria-label", dict.aria.primaryNav);
  const dotsEl = $("#dots");
  if (dotsEl) dotsEl.setAttribute("aria-label", dict.aria.dotsNav);
  const switcherEl = $("#locale-switcher");
  if (switcherEl) switcherEl.setAttribute("aria-label", dict.aria.localeSwitcher);
  const mv1 = $("#sl4-manga-1");
  if (mv1 && dict.aria.mangaVol1) mv1.setAttribute("aria-label", dict.aria.mangaVol1);
  const mv2 = $("#sl4-manga-2");
  if (mv2 && dict.aria.mangaVol2) mv2.setAttribute("aria-label", dict.aria.mangaVol2);

  setText("#brand-subtitle", dict.brandSubtitle);
  setText("#nav-top", dict.nav.top);
  setText("#nav-hakikat", dict.nav.hakikat);
  setText("#nav-bolumler", dict.nav.bolumler);
  setText("#nav-kurulum", dict.nav.kurulum);
  setText("#nav-hakkinda", dict.nav.hakkinda);

  document.querySelectorAll(".locale-button").forEach((btn) => {
    const active = btn.dataset.locale === next;
    btn.classList.toggle("is-active", active);
    btn.setAttribute("aria-pressed", String(active));
  });

  renderHero(dict);
  renderSlicesCopy(dict);
  renderEpisodesHeader(dict);
  renderInstallSection(dict);
  renderAboutSection(dict);

  Object.entries(dict.panelLabels).forEach(([id, label]) => {
    const panel = document.getElementById(id);
    if (panel) panel.dataset.label = label;
  });

  if (deckApi) {
    deckApi.updateDotLabels(dict.panelLabels);
    deckApi.refresh();
  }
  if (slicesApi) {
    slicesApi.setLocale(next, syncQuoteLang);
  }
  if (episodesApi) {
    episodesApi.setLocale(next);
  }

  if (announce) {
    setText("#locale-status", dict.aria.languageChanged);
    try {
      window.localStorage.setItem(STORAGE_KEY, next);
    } catch {
      /* ignore storage restrictions */
    }
  }
}

function bootstrap() {
  const routesEl = $("#short-routes");
  if (routesEl) routesEl.dataset.hydrated = "true";

  // Pre-rendered HTML is in Turkish ('tr'); only re-render upfront if user selected 'en'
  if (currentLocale !== "tr") {
    applyLocale(currentLocale, { announce: false, syncQuoteLang: true });
  }

  deckApi = initDeck({ slicesSectionId: "hakikat", sliceSteps: 5 });
  slicesApi = initSlices(deckApi, currentLocale);
  episodesApi = initEpisodeGrimoire(currentLocale);
  initRemiliaCursor();
  initAtmosphere();

  document.querySelectorAll(".locale-button").forEach((btn) => {
    btn.addEventListener("click", () => {
      const targetLocale = btn.dataset.locale;
      if (targetLocale && targetLocale !== currentLocale) {
        applyLocale(targetLocale, { announce: true, syncQuoteLang: true });
      }
    });
  });
}

bootstrap();
