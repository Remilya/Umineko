import { clamp } from "./deck.js";
import { truthQuotes, galleryItems, content } from "./content.js";

export function initSlices(deckApi, initialLocale = "tr") {
  const panel = document.querySelector("#hakikat");
  const stage = document.querySelector("#slicesStage");
  const area = document.querySelector("#slicesArea");
  if (!panel || !stage || !area || !deckApi) {
    return { setLocale() {} };
  }

  const SL = Array.from(area.querySelectorAll(".sl"));
  const N = SL.length; // 4 Truth slices; step 5 = composite 4-Truth panorama
  const COMP_STEP = N + 1;
  const T = Math.tan(Math.PI / 12); // 15 degrees
  const HG = 4 / Math.cos(Math.PI / 12); // 8px perpendicular obsidian ink cut
  const GAP = 8;

  const initialArea = deckApi.getSlicesAreaSize
    ? deckApi.getSlicesAreaSize()
    : { w: area.clientWidth, h: area.clientHeight };

  let step = -1;
  let W = initialArea.w;
  let H = initialArea.h;
  let port = W < H;

  const f = (n) => `${Math.round(n * 10) / 10}px`;
  const poly = (pts) =>
    `polygon(${pts.map(([x, y]) => `${f(x)} ${f(y)}`).join(", ")})`;

  const fitN = (len, cross, n) => {
    const s = cross * T;
    const eq = len / n - s;
    const m = (len - (n - 1) * s) / n;
    return eq >= 32 ? { s, eq: true } : m >= 12 ? { s, eq: false, m } : null;
  };

  const cols = (w, y0, y1, n, odd, fit) => {
    const hBand = y1 - y0;
    const s = hBand * T;
    const xs = [];
    if (fit.eq) {
      for (let c = 1; c < n; c++) xs.push((c * w) / n);
    } else {
      let x = s / 2 + fit.m;
      for (let c = 1; c < n; c++) {
        xs.push(x);
        x += s + fit.m;
      }
    }

    const seam = (c) => {
      if (c === 0) return { t: 0, b: 0, e: true };
      if (c === n) return { t: w, b: w, e: true };
      const x = xs[c - 1];
      const sg = (c + odd) % 2 === 1 ? 1 : -1;
      return { t: x + (sg * s) / 2, b: x - (sg * s) / 2 };
    };

    const out = [];
    for (let c = 0; c < n; c++) {
      const L = seam(c);
      const R = seam(c + 1);
      const lt = L.e ? L.t : L.t + HG;
      const lb = L.e ? L.b : L.b + HG;
      const rt = R.e ? R.t : R.t - HG;
      const rb = R.e ? R.b : R.b - HG;
      const widerTop = rt - lt >= rb - lb;
      const frac = widerTop ? 0.43 : 0.57;
      const lxAt = lt + (lb - lt) * frac;
      const rxAt = rt + (rb - rt) * frac;
      out.push({
        pts: [
          [lt, y0],
          [rt, y0],
          [rb, y1],
          [lb, y1]
        ],
        lab: {
          x: (lxAt + rxAt) / 2,
          y: y0 + hBand * frac,
          w: Math.max(120, rxAt - lxAt - 22),
          a: "center"
        }
      });
    }
    return out;
  };

  const bands = (w, h, n, fit) => {
    const s = w * T;
    const ys = [];
    if (fit.eq) {
      for (let c = 1; c < n; c++) ys.push((c * h) / n);
    } else {
      let y = s / 2 + fit.m;
      for (let c = 1; c < n; c++) {
        ys.push(y);
        y += s + fit.m;
      }
    }

    const seam = (c) => {
      if (c === 0) return { l: 0, r: 0, e: true };
      if (c === n) return { l: h, r: h, e: true };
      const y = ys[c - 1];
      const sg = c % 2 === 1 ? 1 : -1;
      return { l: y + (sg * s) / 2, r: y - (sg * s) / 2 };
    };

    const out = [];
    for (let c = 0; c < n; c++) {
      const A = seam(c);
      const B = seam(c + 1);
      const tl = A.e ? A.l : A.l + HG;
      const tr = A.e ? A.r : A.r + HG;
      const bl = B.e ? B.l : B.l - HG;
      const br = B.e ? B.r : B.r - HG;
      const widerLeft = bl - tl >= br - tr;
      const xFrac = widerLeft ? 0.46 : 0.54;
      const tyAt = tl + (tr - tl) * xFrac;
      const byAt = bl + (br - bl) * xFrac;
      const yBias = c === 0 ? 0.58 : 0.5;
      out.push({
        pts: [
          [0, tl],
          [w, tr],
          [w, br],
          [0, bl]
        ],
        lab: {
          x: w * 0.5,
          y: tyAt + (byAt - tyAt) * yBias,
          w: w - 32,
          a: "row"
        }
      });
    }
    return out;
  };

  const composition = () => {
    const fc = fitN(W, H, N);
    const fr = fitN(H, W, N);
    if (!port && fc) return { mode: "cols", sl: cols(W, 0, H, N, 0, fc) };
    if (port && fr) return { mode: "rows", sl: bands(W, H, N, fr) };

    const R = 2;
    const C = Math.ceil(N / R);
    const rh = (H - (R - 1) * GAP) / R;
    const sl = [];
    for (let r = 0; r < R; r++) {
      const y0 = r * (rh + GAP);
      const y1 = y0 + rh;
      const fit = fitN(W, rh, C) || {
        s: rh * T,
        eq: false,
        m: Math.max(8, (W - (C - 1) * rh * T) / C)
      };
      sl.push(...cols(W, y0, y1, C, r, fit));
    }
    return { mode: "grid", sl: sl.slice(0, N) };
  };

  const accum = (k) => {
    const out = [];
    if (!port) {
      const s = H * T;
      const ws = clamp(38, W * 0.04, 60);
      const seam = (j) => ({ t: j * ws + s, b: j * ws });

      for (let i = 1; i <= N; i++) {
        if (i < k) {
          const L = i > 1 ? seam(i - 1) : null;
          const R = seam(i);
          const lm = L ? (L.t + L.b) / 2 : 0;
          const rm = (R.t + R.b) / 2;
          out.push({
            st: "sliver",
            pts: [
              [L ? L.t + HG : 0, 0],
              [R.t - HG, 0],
              [R.b - HG, H],
              [L ? L.b + HG : 0, H]
            ],
            sliv: {
              x: (lm + rm) / 2 + (L ? 0 : ws * 0.16),
              y: H / 2,
              r: -75
            }
          });
        } else if (i === k) {
          const L = i > 1 ? seam(i - 1) : null;
          const l = L ? L.t + HG : 0;
          out.push({
            st: "active",
            pts: [
              [l, 0],
              [W, 0],
              [W, H],
              [L ? L.b + HG : 0, H]
            ],
            body: {
              l: l + (L ? 20 : clamp(18, W * 0.04, 56)),
              t: 14,
              r: clamp(32, W * 0.055, 76),
              b: 14
            }
          });
        } else {
          const x = W + 4;
          out.push({
            st: "park",
            pts: [
              [x + s, 0],
              [x + s + 1, 0],
              [x + 1, H],
              [x, H]
            ]
          });
        }
      }
    } else {
      const s = W * T;
      const hs = clamp(30, H * 0.046, 42);
      const seam = (j) => ({ l: j * hs + s, r: j * hs });

      for (let i = 1; i <= N; i++) {
        if (i < k) {
          const A = i > 1 ? seam(i - 1) : null;
          const B = seam(i);
          const am = A ? (A.l + A.r) / 2 : 0;
          const bm = (B.l + B.r) / 2;
          out.push({
            st: "sliver",
            pts: [
              [0, A ? A.l + HG : 0],
              [W, A ? A.r + HG : 0],
              [W, B.r - HG],
              [0, B.l - HG]
            ],
            sliv: {
              x: W / 2,
              y: (am + bm) / 2 + (A ? 0 : hs * 0.18),
              r: -15
            }
          });
        } else if (i === k) {
          const A = i > 1 ? seam(i - 1) : null;
          const t = A ? A.l + HG : 0;
          out.push({
            st: "active",
            pts: [
              [0, t],
              [W, A ? A.r + HG : 0],
              [W, H],
              [0, H]
            ],
            body: {
              l: 14,
              t: t + (A ? 12 : 14),
              r: 14,
              b: 12
            }
          });
        } else {
          const y = H + 4;
          out.push({
            st: "park",
            pts: [
              [0, y + s],
              [W, y],
              [W, y + 1],
              [0, y + s + 1]
            ]
          });
        }
      }
    }
    return out;
  };

  const setBody = (el, b) => {
    el.style.setProperty("--bl", f(b.l));
    el.style.setProperty("--bt", f(b.t));
    el.style.setProperty("--br", f(b.r));
    el.style.setProperty("--bb", f(b.b));
  };

  const render = (nextW, nextH) => {
    if (deckApi.flatMQ.matches) {
      stage.classList.remove("is-comp");
      SL.forEach((el) => {
        el.style.clipPath = "";
        el.classList.remove("is-sliver");
        el.classList.add("is-active");
      });
      return;
    }

    if (typeof nextW === "number" && typeof nextH === "number") {
      W = nextW;
      H = nextH;
    } else if (!W || !H) {
      const sz = deckApi.getSlicesAreaSize
        ? deckApi.getSlicesAreaSize()
        : { w: 0, h: 0 };
      W = sz.w || area.clientWidth;
      H = sz.h || area.clientHeight;
    }
    if (!W || !H) return;
    port = W < H;
    stage.classList.toggle("m-port", port);

    const comp = composition();
    stage.classList.remove("m-cols", "m-rows", "m-grid");
    stage.classList.add(`m-${comp.mode}`);

    const k = Math.max(0, step);
    if (k >= 1 && k <= N) {
      accum(k).forEach((g, i) => {
        const el = SL[i];
        el.style.clipPath = poly(g.pts);
        el.classList.toggle("is-active", g.st === "active");
        el.classList.toggle("is-sliver", g.st === "sliver");
        if (g.body) setBody(el, g.body);
        if (g.sliv) {
          el.style.setProperty("--sx", f(g.sliv.x));
          el.style.setProperty("--sy", f(g.sliv.y));
          el.style.setProperty("--sr", `${g.sliv.r}deg`);
        }
      });
    } else if (k === COMP_STEP) {
      comp.sl.forEach((g, i) => {
        const el = SL[i];
        el.classList.remove("is-active", "is-sliver");
        const lab = el.querySelector(".lab");
        if (lab) {
          lab.dataset.a = g.lab.a;
        }
        el.style.setProperty("--lx", f(g.lab.x));
        el.style.setProperty("--ly", f(g.lab.y));
        el.style.setProperty("--lw", f(g.lab.w));
        el.style.clipPath = poly(g.pts);
      });
    } else {
      accum(0).forEach((g, i) => {
        SL[i].style.clipPath = poly(g.pts);
        SL[i].classList.remove("is-active", "is-sliver");
      });
    }

    SL.forEach((el, i) => el.style.setProperty("--z", String(i + 1)));
  };

  const instant = (fn) => {
    stage.classList.add("no-tr");
    fn();
    requestAnimationFrame(() =>
      requestAnimationFrame(() => stage.classList.remove("no-tr"))
    );
  };

  const setStep = (n) => {
    if (n === step) return;
    step = n;
    stage.dataset.step = String(n);
    stage.classList.toggle("is-comp", n === COMP_STEP);
    render();
  };

  const slicesPanelIdx = deckApi.panels.indexOf(panel);
  const updateFromScroll = (st) => {
    const stops = deckApi.getStops();
    if (deckApi.flatMQ.matches) {
      let k = 1;
      stops.forEach((s) => {
        if (s.pi === slicesPanelIdx && s.top <= st + deckApi.DH() / 2) {
          k = s.k;
        }
      });
      setStep(k);
      return;
    }

    const panelTop = deckApi.getPanelTop
      ? deckApi.getPanelTop(slicesPanelIdx)
      : panel.offsetTop;

    if (st < panelTop - deckApi.DH() * 0.55) {
      setStep(0);
      return;
    }

    let k = 1;
    let bd = Infinity;
    stops.forEach((s) => {
      if (s.pi !== slicesPanelIdx) return;
      const d = Math.abs(s.top - st);
      if (d <= bd) {
        bd = d;
        k = s.k;
      }
    });
    setStep(k);
  };

  instant(() => {
    step = -1;
    setStep(0);
  });

  if ("ResizeObserver" in window) {
    new ResizeObserver((entries) => {
      const cr = entries[0]?.contentRect;
      if (!cr) return;
      if (Math.abs(cr.width - W) < 1 && Math.abs(cr.height - H) < 1) return;
      instant(() => render(cr.width, cr.height));
    }).observe(area);
  }

  area.addEventListener("click", (e) => {
    if (e.target.closest("a, button")) return;
    const sl = e.target.closest(".sl");
    if (!sl) return;
    if (
      sl.classList.contains("is-sliver") ||
      stage.classList.contains("is-comp")
    ) {
      const k = Number(sl.dataset.k);
      if (k >= 1 && k <= N) {
        deckApi.goToSlice(k);
      }
    }
  });

  deckApi.registerSlicesUpdater(updateFromScroll);

  // =========================================================================
  // SLICE 1: Live 4-Truth × 3-Language VN Dialogue Box
  // =========================================================================
  let siteLocale = initialLocale === "en" ? "en" : "tr";
  let quoteLang = siteLocale;
  let truthIdx = 0;

  const truthBox = document.querySelector("#vn-box");
  const bustEl = document.querySelector("#vn-bust");
  const badgeEl = document.querySelector("#vn-badge");
  const speakerEl = document.querySelector("#vn-speaker");
  const lineEl = document.querySelector("#vn-line");
  const truthBtns = Array.from(document.querySelectorAll("[data-truth-idx]"));
  const langBtns = Array.from(document.querySelectorAll("[data-quote-lang]"));

  const paintTruth = () => {
    const q = truthQuotes[truthIdx] || truthQuotes[0];
    if (truthBox) {
      truthBox.dataset.tone = q.tone;
    }
    truthBtns.forEach((btn) => {
      btn.setAttribute(
        "aria-pressed",
        String(Number(btn.dataset.truthIdx) === truthIdx)
      );
    });
    langBtns.forEach((btn) => {
      btn.setAttribute(
        "aria-pressed",
        String(btn.dataset.quoteLang === quoteLang)
      );
    });

    if (bustEl && bustEl.getAttribute("src") !== q.bust) {
      bustEl.src = q.bust;
      bustEl.alt = q.speaker[siteLocale] || q.speaker.tr;
    }
    if (badgeEl) {
      badgeEl.textContent = q.badge[quoteLang] || q.badge.tr;
    }
    if (speakerEl) {
      speakerEl.textContent = q.speaker[quoteLang] || q.speaker.tr;
      speakerEl.lang = quoteLang === "ja" ? "ja" : quoteLang;
    }
    if (lineEl) {
      const wrap = quoteLang === "ja" ? ["「", "」"] : ["“", "”"];
      lineEl.textContent = `${wrap[0]}${q.lines[quoteLang] || q.lines.tr}${wrap[1]}`;
      lineEl.lang = quoteLang === "ja" ? "ja" : quoteLang;
    }
  };

  truthBtns.forEach((btn) => {
    btn.addEventListener("click", () => {
      const idx = Number(btn.dataset.truthIdx);
      if (!Number.isNaN(idx) && truthQuotes[idx]) {
        truthIdx = idx;
        paintTruth();
      }
    });
  });

  langBtns.forEach((btn) => {
    btn.addEventListener("click", () => {
      const l = btn.dataset.quoteLang;
      if (l === "tr" || l === "en" || l === "ja") {
        quoteLang = l;
        paintTruth();
      }
    });
  });

  // =========================================================================
  // SLICE 3: Interactive 4-Tab Visual Gallery + Fullscreen Lightbox
  // =========================================================================
  let galIdx = 0;
  const galImg = document.querySelector("#gal-img");
  const galCap = document.querySelector("#gal-caption");
  const galTrigger = document.querySelector("#gal-trigger");
  const galTabs = Array.from(document.querySelectorAll("[data-gal-idx]"));
  const zoomAction = document.querySelector("#sl3-primary");

  const lightbox = document.querySelector("#lightbox");
  const lbImg = document.querySelector("#lightbox-img");
  const lbCap = document.querySelector("#lightbox-caption");
  const lbClose = document.querySelector("#lightbox-close");
  let lastFocused = null;

  const paintGallery = () => {
    const item = galleryItems[galIdx] || galleryItems[0];
    galTabs.forEach((tab, idx) => {
      tab.setAttribute("aria-pressed", String(idx === galIdx));
      const label = item ? galleryItems[idx]?.tab[siteLocale] : "";
      if (label) tab.textContent = label;
    });
    if (galImg) {
      galImg.src = item.thumb;
      galImg.alt = item.caption[siteLocale] || item.caption.tr;
    }
    if (galCap) {
      galCap.textContent = item.caption[siteLocale] || item.caption.tr;
    }
  };

  const openLightbox = (src, captionText) => {
    if (!lightbox || !lbImg) return;
    lastFocused = document.activeElement;
    lbImg.src = src;
    lbImg.alt = captionText;
    if (lbCap) lbCap.textContent = captionText;
    lightbox.hidden = false;
    lightbox.classList.add("is-open");
    if (lbClose) lbClose.focus();
  };

  const closeLightbox = () => {
    if (!lightbox || !lightbox.classList.contains("is-open")) return;
    lightbox.classList.remove("is-open");
    lightbox.hidden = true;
    if (lastFocused && typeof lastFocused.focus === "function") {
      lastFocused.focus();
    }
  };

  galTabs.forEach((tab) => {
    tab.addEventListener("click", () => {
      const idx = Number(tab.dataset.galIdx);
      if (!Number.isNaN(idx) && galleryItems[idx]) {
        galIdx = idx;
        paintGallery();
      }
    });
  });

  const openCurrentGalleryItem = (e) => {
    if (e) e.preventDefault();
    const item = galleryItems[galIdx] || galleryItems[0];
    openLightbox(item.hd, item.caption[siteLocale] || item.caption.tr);
  };

  if (galTrigger) galTrigger.addEventListener("click", openCurrentGalleryItem);
  if (zoomAction) {
    zoomAction.addEventListener("click", (e) => {
      if (zoomAction.getAttribute("href") === "#zoom") {
        openCurrentGalleryItem(e);
      }
    });
  }

  const installerTrigger = document.querySelector("#installer-zoom");
  if (installerTrigger) {
    installerTrigger.addEventListener("click", () => {
      const dict = content[siteLocale] || content.tr;
      openLightbox(
        "assets/installer-preview.webp",
        `${dict.install.exeTitle} · ${dict.install.exeMeta}`
      );
    });
  }

  if (lbClose) lbClose.addEventListener("click", closeLightbox);
  if (lightbox) {
    lightbox.addEventListener("click", (e) => {
      if (e.target === lightbox) closeLightbox();
    });
  }
  window.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && lightbox && lightbox.classList.contains("is-open")) {
      e.preventDefault();
      closeLightbox();
    }
  });

  paintTruth();
  paintGallery();

  return {
    setLocale(nextLocale, syncQuoteLang = false) {
      siteLocale = nextLocale === "en" ? "en" : "tr";
      if (syncQuoteLang && quoteLang !== "ja") {
        quoteLang = siteLocale;
      }
      const dict = content[siteLocale] || content.tr;
      truthBtns.forEach((btn) => {
        const idx = Number(btn.dataset.truthIdx);
        const q = truthQuotes[idx];
        if (q && dict.slices.truthTabs[q.id]) {
          btn.textContent = dict.slices.truthTabs[q.id];
        }
      });
      if (lbClose) lbClose.setAttribute("aria-label", dict.aria.closeModal);
      paintTruth();
      paintGallery();
      instant(() => render());
    }
  };
}
