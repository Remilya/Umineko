import { gesture } from "./deck.js";
import { content } from "./content.js";

const deterministicRand = (seed) => {
  const x = Math.sin(seed * 9301 + 49297) * 233280;
  return x - Math.floor(x);
};

export function initEpisodeGrimoire(initialLocale = "tr") {
  const stageEl = document.querySelector("#ep-stage");
  if (!stageEl) {
    return { setLocale() {} };
  }

  let locale = initialLocale === "en" ? "en" : "tr";
  let activeIdx = 0;

  const cards = Array.from(stageEl.querySelectorAll(".ep-card"));
  const tabs = Array.from(document.querySelectorAll("[data-ep-tab]"));
  const prevBtn = document.querySelector("#ep-prev");
  const nextBtn = document.querySelector("#ep-next");

  const epBadgeEl = document.querySelector("#ep-active-badge");
  const statsEl = document.querySelector("#ep-active-stats");
  const titleEl = document.querySelector("#ep-active-title");
  const subEl = document.querySelector("#ep-active-sub");
  const noteEl = document.querySelector("#ep-active-note");
  const truthsEl = document.querySelector("#ep-active-truths");
  const statusEl = document.querySelector("#ep-active-status");
  const ctaEl = document.querySelector("#ep-active-cta");
  const countEl = document.querySelector("#ep-count");

  const paint = () => {
    const dict = content[locale] || content.tr;
    const items = dict.episodes.items;
    const total = items.length;
    activeIdx = ((activeIdx % total) + total) % total;
    const item = items[activeIdx];

    cards.forEach((card, i) => {
      const diff = (i - activeIdx + total) % total;
      let pos = "hidden";
      if (diff === 0) pos = "center";
      else if (diff === 1) pos = "right";
      else if (diff === total - 1) pos = "left";
      else if (diff === 2) pos = "far-right";
      else if (diff === total - 2) pos = "far-left";

      card.dataset.pos = pos;
      card.classList.toggle("is-center", pos === "center");
      const btn = card.querySelector(".ep-card__btn");
      if (btn) {
        btn.setAttribute("aria-pressed", String(pos === "center"));
        if (items[i]) {
          btn.setAttribute("aria-label", `${items[i].ep}: ${items[i].title}`);
        }
      }
    });

    tabs.forEach((tab, i) => {
      tab.setAttribute("aria-pressed", String(i === activeIdx));
      if (items[i]) tab.textContent = items[i].shortTab;
    });

    if (epBadgeEl) epBadgeEl.textContent = item.ep;
    if (statsEl) statsEl.textContent = item.stats;
    if (titleEl) titleEl.textContent = item.title;
    if (subEl) subEl.textContent = item.enSub;
    if (noteEl) noteEl.textContent = item.note;
    if (truthsEl) {
      truthsEl.replaceChildren(
        ...item.truths.map((t) => {
          const span = document.createElement("span");
          span.className = "ep-chip";
          span.textContent = t;
          return span;
        })
      );
    }
    if (statusEl) statusEl.textContent = dict.episodes.statusLabel;
    if (ctaEl) ctaEl.textContent = dict.episodes.ctaLabel;
    if (countEl) {
      countEl.textContent = `0${activeIdx + 1} / 0${total}`;
    }
    if (prevBtn) prevBtn.setAttribute("aria-label", dict.episodes.prevAria);
    if (nextBtn) nextBtn.setAttribute("aria-label", dict.episodes.nextAria);
  };

  const turn = (next) => {
    activeIdx = next;
    paint();
  };

  cards.forEach((card, idx) => {
    const btn = card.querySelector(".ep-card__btn");
    if (!btn) return;
    btn.addEventListener("click", () => {
      if (idx === activeIdx) {
        turn(activeIdx + 1);
      } else {
        turn(idx);
      }
    });
  });

  tabs.forEach((tab) => {
    tab.addEventListener("click", () => {
      const idx = Number(tab.dataset.epTab);
      if (!Number.isNaN(idx)) turn(idx);
    });
  });

  if (prevBtn) prevBtn.addEventListener("click", () => turn(activeIdx - 1));
  if (nextBtn) nextBtn.addEventListener("click", () => turn(activeIdx + 1));

  // Pointer swipe support
  let x0 = null;
  let y0 = null;
  stageEl.addEventListener("pointerdown", (e) => {
    x0 = e.clientX;
    y0 = e.clientY;
  });
  stageEl.addEventListener("pointerup", (e) => {
    if (x0 === null || y0 === null) return;
    const dx = e.clientX - x0;
    const dy = e.clientY - y0;
    x0 = null;
    y0 = null;
    if (Math.abs(dx) > 36 && Math.abs(dx) > Math.abs(dy)) {
      turn(activeIdx + (dx < 0 ? 1 : -1));
    }
  });
  stageEl.addEventListener("pointercancel", () => {
    x0 = null;
    y0 = null;
  });

  // Sideways trackpad & Left/Right arrow navigation
  const wheelTurn = gesture(38, 420);
  const epPanel = document.querySelector("#bolumler");
  if (epPanel) {
    epPanel.addEventListener(
      "wheel",
      (e) => {
        if (Math.abs(e.deltaX) <= Math.abs(e.deltaY)) return;
        e.preventDefault();
        const dir = wheelTurn(
          e.deltaX * (e.deltaMode === 1 ? 16 : 1),
          performance.now()
        );
        if (dir) turn(activeIdx + dir);
      },
      { passive: false }
    );

    epPanel.addEventListener("keydown", (e) => {
      if (e.key === "ArrowLeft") {
        e.preventDefault();
        turn(activeIdx - 1);
      } else if (e.key === "ArrowRight") {
        e.preventDefault();
        turn(activeIdx + 1);
      }
    });
  }

  paint();

  return {
    setLocale(nextLocale) {
      locale = nextLocale === "en" ? "en" : "tr";
      paint();
    }
  };
}

export function initCopyHashes(getLocale) {
  document.querySelectorAll("[data-copy-hash]").forEach((btn) => {
    let timer = 0;
    btn.addEventListener("click", async () => {
      const hash = btn.dataset.copyHash;
      if (!hash) return;
      try {
        await navigator.clipboard.writeText(hash);
      } catch {
        const ta = document.createElement("textarea");
        ta.value = hash;
        ta.style.position = "fixed";
        ta.style.opacity = "0";
        document.body.appendChild(ta);
        ta.select();
        document.execCommand("copy");
        ta.remove();
      }
      const loc = getLocale ? getLocale() : "tr";
      const dict = content[loc] || content.tr;
      const labelEl = btn.querySelector("[data-copy-label]") || btn;
      labelEl.textContent = dict.install.copiedHash;
      btn.classList.add("is-copied");
      window.clearTimeout(timer);
      timer = window.setTimeout(() => {
        const curDict = content[getLocale ? getLocale() : "tr"] || content.tr;
        labelEl.textContent = curDict.install.copyHash;
        btn.classList.remove("is-copied");
      }, 2000);
    });
  });
}

export function initAtmosphere() {
  const reduceMotion = window.matchMedia(
    "(prefers-reduced-motion: reduce)"
  ).matches;
  const coarsePointer = window.matchMedia("(hover: none)").matches;
  if (reduceMotion) return;

  const vw0 = window.innerWidth;
  const vh0 = window.innerHeight;

  // Seed lightweight CSS compositor Golden & Crimson embers in Hero & Credits
  document.querySelectorAll("[data-motes]").forEach((box, b) => {
    const count = Math.round(
      (Number(box.dataset.motes) || 16) * (coarsePointer ? 0.5 : 1)
    );
    const frag = document.createDocumentFragment();
    for (let i = 0; i < count; i++) {
      const seed = (i + 3) * (b + 5) * 13;
      const el = document.createElement("i");
      const isCrimson = i % 4 === 0;
      el.className = isCrimson ? "mote mote--crimson" : "mote mote--gold";
      el.style.cssText = [
        `left:${(deterministicRand(seed) * 100).toFixed(2)}%`,
        `--sz:${(2.6 + deterministicRand(seed + 1) * 3.8).toFixed(1)}px`,
        `--dur:${(10 + deterministicRand(seed + 2) * 11).toFixed(1)}s`,
        `--dl:${(-deterministicRand(seed + 3) * 18).toFixed(1)}s`,
        `--dx:${(deterministicRand(seed + 4) * 88 - 44).toFixed(0)}px`,
        `--op:${(0.35 + deterministicRand(seed + 5) * 0.5).toFixed(2)}`
      ].join(";");
      frag.appendChild(el);
    }
    box.appendChild(frag);
  });

  if (coarsePointer) return;

  const canvas = document.querySelector("#cursorFx");
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  if (!ctx) return;

  canvas.width = vw0;
  canvas.height = vh0;
  window.addEventListener(
    "resize",
    () => {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
    },
    { passive: true }
  );

  const MAX = 42;
  const GAP = 40;
  const IDLE = 220;
  const FADE = 2400;
  const LOCK_SELECTOR = 'a, button, select, input, [data-cursor="lock"]';

  const parts = [];
  let x = -300;
  let y = -300;
  let locked = false;
  let lastMove = performance.now();
  let idleAt = null;
  let idle = false;
  let last = performance.now();
  let lastSpawn = 0;
  let px = null;
  let py = null;
  let pt = performance.now();
  let speed = 0;
  let inButterflyZone = false;

  const checkZone = (target) =>
    Boolean(
      target &&
        target.closest &&
        target.closest(".hero, .grimoire-panel, .contact-panel")
    );

  const drawSpark = (cx, cy, r, a) => {
    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(Math.PI / 4);
    ctx.fillStyle = `rgba(250, 204, 21, ${(a * 0.92).toFixed(3)})`;
    ctx.fillRect(-r, -r, r * 2, r * 2);
    ctx.restore();
  };

  // Beatrice's Golden & Crimson Butterflies
  const drawButterfly = (cx, cy, r, rot, phase, a, crimsonTone) => {
    const flap = 0.4 + 0.6 * Math.abs(Math.sin(phase));
    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(rot);
    ctx.scale(flap, 1);

    const fill = crimsonTone
      ? `rgba(244, 63, 94, ${(a * 0.92).toFixed(3)})`
      : `rgba(250, 204, 21, ${(a * 0.95).toFixed(3)})`;

    for (const side of [-1, 1]) {
      ctx.save();
      ctx.scale(side, 1);
      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.bezierCurveTo(
        r * 0.92,
        -r * 1.18,
        r * 1.68,
        -r * 0.56,
        r * 0.74,
        r * 0.12
      );
      ctx.bezierCurveTo(r * 1.28, r * 0.56, r * 0.66, r * 1.16, 0, r * 0.36);
      ctx.closePath();
      ctx.fillStyle = fill;
      ctx.fill();
      ctx.restore();
    }
    ctx.restore();
  };

  document.addEventListener("mousemove", (e) => {
    const now = performance.now();
    if (px !== null) {
      const dt = now - pt;
      if (dt > 0 && dt < 100) {
        const dx = e.clientX - px;
        const dy = e.clientY - py;
        speed =
          speed * 0.75 + (Math.sqrt(dx * dx + dy * dy) / dt) * 1000 * 0.25;
      }
    }
    px = x = e.clientX;
    py = y = e.clientY;
    pt = lastMove = now;
    inButterflyZone = checkZone(e.target);
  });

  document.addEventListener("mouseover", (e) => {
    if (e.target && e.target.closest && e.target.closest(LOCK_SELECTOR)) {
      locked = true;
    }
  });

  document.addEventListener("mouseout", (e) => {
    if (
      e.target &&
      e.target.closest &&
      e.target.closest(LOCK_SELECTOR) &&
      !(
        e.relatedTarget &&
        e.relatedTarget.closest &&
        e.relatedTarget.closest(LOCK_SELECTOR)
      )
    ) {
      locked = false;
    }
  });

  document.documentElement.addEventListener("mouseleave", () => {
    x = -300;
    y = -300;
  });

  const spawn = (sx, sy, strength) => {
    if (parts.length >= MAX) return;
    const life = 0.24 + strength * 0.76;
    if (inButterflyZone) {
      parts.push({
        x: sx + (Math.random() - 0.5) * 9,
        y: sy + (Math.random() - 0.5) * 9,
        vx: (Math.random() - 0.5) * 0.68,
        vy: -(Math.random() * 0.58 + 0.18),
        life,
        lifeAtIdle: null,
        r: Math.random() * 2.9 + 3.6,
        type: "butterfly",
        crimson: Math.random() < 0.24,
        rot: (Math.random() - 0.5) * 0.8,
        rotSpeed: (Math.random() - 0.5) * 0.025,
        phase: Math.random() * Math.PI * 2,
        decay: 1500
      });
    } else {
      parts.push({
        x: sx + (Math.random() - 0.5) * 5,
        y: sy + (Math.random() - 0.5) * 5,
        vx: (Math.random() - 0.5) * 0.42,
        vy: -(Math.random() * 0.45 + 0.14),
        life,
        lifeAtIdle: null,
        r: Math.random() * 1.4 + 1.5,
        type: "spark",
        decay: 1100
      });
    }
  };

  let rafActive = false;
  const startLoop = () => {
    if (rafActive) return;
    rafActive = true;
    last = performance.now();
    requestAnimationFrame(frame);
  };

  document.addEventListener("mousemove", startLoop);

  const frame = (now) => {
    if (document.hidden) {
      rafActive = false;
      return;
    }

    const dt = Math.min(now - last, 50);
    last = now;
    const isIdle = !locked && now - lastMove > IDLE;
    if (isIdle && !idle) {
      idleAt = now;
      parts.forEach((p) => {
        p.lifeAtIdle = p.life;
      });
    }
    idle = isIdle;
    const fade = isIdle && idleAt ? Math.min(1, (now - idleAt) / FADE) : 0;

    if (locked) parts.length = 0;

    const strength = Math.min(speed / 460, 1);
    const gap = GAP + (1 - strength) * 150;
    if (
      !locked &&
      !isIdle &&
      now - lastSpawn > gap &&
      x > -200 &&
      strength > 0.03
    ) {
      spawn(x, y, strength);
      lastSpawn = now;
    }

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (let k = parts.length - 1; k >= 0; k--) {
      const p = parts[k];
      if (isIdle && p.lifeAtIdle !== null) {
        p.life = p.lifeAtIdle * (1 - fade);
      } else {
        p.life -= dt / p.decay;
      }
      if (p.life <= 0) {
        parts.splice(k, 1);
        continue;
      }
      p.x += p.vx;
      p.y += p.vy;
      if (p.type === "butterfly") {
        p.rot += p.rotSpeed;
        p.phase += dt * 0.014;
        drawButterfly(p.x, p.y, p.r, p.rot, p.phase, p.life, p.crimson);
      } else {
        drawSpark(p.x, p.y, p.r, p.life);
      }
    }

    // Stop condition when idle and zero particles remain (per fixing-motion-performance!)
    if (isIdle && parts.length === 0) {
      rafActive = false;
      return;
    }

    requestAnimationFrame(frame);
  };
}
