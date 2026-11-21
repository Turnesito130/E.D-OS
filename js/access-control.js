'use strict';

(() => {
  const gate = document.getElementById('accessGate');
  const form = document.getElementById('accessForm');
  const input = document.getElementById('accessPin');
  const submit = document.getElementById('accessSubmit');
  const message = document.getElementById('accessMessage');
  const title = document.getElementById('accessTitle');
  const description = gate.querySelector('.access-card > p');
  const KEY = 'edos-local-lock-v2';
  const SESSION_KEY = 'edos-access-granted-v2';
  let lockTicker = null;
  const ITERATIONS = 210000;
  const encoder = new TextEncoder();
  let state = { salt: null, verifier: null, failures: 0, lockedUntil: 0 };

  function readState() {
    try {
      const saved = JSON.parse(localStorage.getItem(KEY) || 'null');
      if (saved && typeof saved.salt === 'string' &&
          typeof saved.verifier === 'string' &&
          Number.isInteger(saved.failures) &&
          Number.isFinite(saved.lockedUntil)) {
        state = { ...state, ...saved };
      }
    } catch (_) {}
  }

  function save() {
    try { localStorage.setItem(KEY, JSON.stringify(state)); }
    catch (_) { message.textContent = 'No se pudo guardar el bloqueo local en este navegador.'; }
  }

  function bytesToHex(bytes) {
    return Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join('');
  }

  async function derive(pin, saltHex) {
    const salt = Uint8Array.from(saltHex.match(/.{2}/g), pair => parseInt(pair, 16));
    const material = await crypto.subtle.importKey('raw', encoder.encode(pin), 'PBKDF2', false, ['deriveBits']);
    const bits = await crypto.subtle.deriveBits(
      { name: 'PBKDF2', hash: 'SHA-256', salt, iterations: ITERATIONS },
      material,
      256
    );
    return bytesToHex(new Uint8Array(bits));
  }

  function hasSessionGrant() {
    try { return sessionStorage.getItem(SESSION_KEY) === '1'; }
    catch (_) { return false; }
  }

  function unlock() {
    try { sessionStorage.setItem(SESSION_KEY, '1'); } catch (_) {}
    gate.hidden = true;
    document.body.style.overflow = '';
  }

  function render() {
    const remaining = Math.max(0, state.lockedUntil - Date.now());
    const locked = remaining > 0;
    input.disabled = locked;
    submit.disabled = locked;
    if (locked) {
      if (!lockTicker) lockTicker = setInterval(render, 1000);
      const seconds = Math.ceil(remaining / 1000);
      message.textContent = 'Bloqueo temporal: espera ' +
        Math.floor(seconds / 60) + ':' + String(seconds % 60).padStart(2, '0') + '.';
    } else {
      if (lockTicker) { clearInterval(lockTicker); lockTicker = null; }
      if (state.lockedUntil) { state.lockedUntil = 0; save(); }
      message.textContent = state.failures
        ? 'Vuelve a intentarlo. Los intentos fallidos activan esperas progresivas.'
        : '';
    }
  }

  async function submitPin(event) {
    event.preventDefault();
    if (Date.now() < state.lockedUntil || submit.disabled) return;
    const pin = input.value;
    if (!/^\d{6,12}$/.test(pin)) {
      message.textContent = 'Usa un código numérico de entre 6 y 12 cifras.';
      input.focus();
      return;
    }
    submit.disabled = true;
    input.disabled = true;
    try {
      if (!state.salt || !state.verifier) {
        const salt = crypto.getRandomValues(new Uint8Array(16));
        state.salt = bytesToHex(salt);
        state.verifier = await derive(pin, state.salt);
        state.failures = 0;
        state.lockedUntil = 0;
        save();
        unlock();
        return;
      }
      const candidate = await derive(pin, state.salt);
      if (candidate === state.verifier) {
        state.failures = 0;
        state.lockedUntil = 0;
        save();
        unlock();
        return;
      }
      state.failures += 1;
      const delay = state.failures === 1 ? 5 : 10;
      state.lockedUntil = Date.now() + delay * 60 * 1000;
      save();
      input.value = '';
      render();
    } catch (_) {
      message.textContent = 'Este navegador no permite el cifrado necesario. Abre E.D-OS en un contexto seguro (HTTPS o localhost).';
    } finally {
      if (!gate.hidden) {
        input.disabled = false;
        submit.disabled = false;
        render();
      }
    }
  }

  readState();
  if (hasSessionGrant()) {
    gate.hidden = true;
    document.body.style.overflow = '';
    return;
  }
  const firstSetup = !state.salt || !state.verifier;
  title.textContent = firstSetup ? 'Configura tu acceso local' : 'Acceso local';
  description.textContent = firstSetup
    ? 'Crea un código de 6 a 12 cifras para este navegador.'
    : 'Introduce tu código local para abrir este escritorio.';
  input.minLength = 6;
  input.maxLength = 12;
  input.pattern = '[0-9]{6,12}';
  input.placeholder = '6–12 cifras';
  input.autocomplete = 'off';
  form.addEventListener('submit', submitPin);
  document.body.style.overflow = 'hidden';
  render();
  input.focus();
})();
