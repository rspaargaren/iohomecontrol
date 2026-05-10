let I18N = {};
let FALLBACK = {};
let CURRENT_LANG = "en";

async function loadJson(path) {
  const response = await fetch(path);
  return response.ok ? response.json() : {};
}

function interpolate(text, params = {}) {
  if (typeof text !== "string") return text;
  return text.replace(/\{(\w+)\}/g, (_, key) => {
    return Object.prototype.hasOwnProperty.call(params, key) ? params[key] : `{${key}}`;
  });
}

function t(key, params = {}) {
  const value = I18N[key] ?? FALLBACK[key] ?? key;
  return interpolate(value, params);
}

function applyI18n() {
  document.querySelectorAll("[data-i18n]").forEach((el) => {
    el.textContent = t(el.dataset.i18n);
  });

  document.querySelectorAll("[data-i18n-placeholder]").forEach((el) => {
    el.placeholder = t(el.dataset.i18nPlaceholder);
  });

  document.title = t("page.title");
}

async function setLang(lang) {
  const nextLang = lang || "en";
  I18N = await loadJson(`/lang/${nextLang}.json`);
  CURRENT_LANG = nextLang;
  localStorage.setItem("lang", nextLang);
  applyI18n();
  window.dispatchEvent(new CustomEvent("i18n:changed", { detail: { lang: CURRENT_LANG } }));
}

function getLang() {
  return CURRENT_LANG;
}

(async () => {
  FALLBACK = await loadJson("/lang/en.json");
  const saved = localStorage.getItem("lang");
  const auto = (navigator.language || "en").slice(0, 2).toLowerCase();
  const initial = saved || auto;
  const supported = ["nl", "en", "de", "fr"];
  const lang = supported.includes(initial) ? initial : "en";
  const select = document.getElementById("lang");
  if (select) select.value = lang;
  await setLang(lang);
})();

window.t = t;
window.setLang = setLang;
window.getLang = getLang;
