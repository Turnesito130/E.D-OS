(() => {
  'use strict';
  const STORAGE_KEY = 'ed-os-sync-pending-v1';
  const CACHE_KEY = 'ed-os-sync-cache-v1';
  const MAX_ITEMS = 500;
  const MAX_PAYLOAD_BYTES = 12_000;
  const KEY_PATTERN = /^[a-zA-Z0-9._-]{1,100}$/;

  function read(storage = window.localStorage, storageKey = STORAGE_KEY) {
    try {
      const parsed = JSON.parse(storage.getItem(storageKey) || '{}');
      return parsed && typeof parsed === 'object' && !Array.isArray(parsed)
        ? Object.assign(Object.create(null), parsed)
        : Object.create(null);
    } catch (_) {
      return Object.create(null);
    }
  }

  function readCache(storage = window.localStorage, storageKey = CACHE_KEY) {
    try {
      const parsed = JSON.parse(storage.getItem(storageKey) || '{}');
      return parsed && typeof parsed === 'object' && !Array.isArray(parsed)
        ? Object.assign(Object.create(null), parsed)
        : Object.create(null);
    } catch (_) {
      return Object.create(null);
    }
  }

  function writeCache(cache, storage = window.localStorage, storageKey = CACHE_KEY) {
    if (!cache || typeof cache !== 'object' || Array.isArray(cache) ||
        Object.keys(cache).length > MAX_ITEMS) return false;
    try {
      storage.setItem(storageKey, JSON.stringify(cache));
      return true;
    } catch (_) {
      return false;
    }
  }

  function write(queue, storage = window.localStorage, storageKey = STORAGE_KEY) {
    if (!queue || typeof queue !== 'object' || Array.isArray(queue)) return false;
    if (Object.keys(queue).length > MAX_ITEMS) return false;
    try {
      storage.setItem(storageKey, JSON.stringify(queue));
      return true;
    } catch (_) {
      return false;
    }
  }

  function enqueue(queue, key, payload, expectedVersion, updatedAt = Date.now()) {
    if (!queue || typeof queue !== 'object' || Array.isArray(queue) ||
        typeof key !== 'string' || !KEY_PATTERN.test(key)) {
      return { ok: false, error: 'La clave del elemento no es válida.' };
    }
    if (!Number.isSafeInteger(expectedVersion) || expectedVersion < 0 ||
        expectedVersion > 2_000_000_000) {
      return { ok: false, error: 'La versión esperada no es válida.' };
    }
    let serialized;
    try { serialized = JSON.stringify(payload); }
    catch (_) { return { ok: false, error: 'El contenido no se puede convertir a JSON.' }; }
    if (serialized === undefined || new TextEncoder().encode(serialized).length > MAX_PAYLOAD_BYTES) {
      return { ok: false, error: 'El contenido JSON supera el límite de 12 KB.' };
    }
    if (!Object.prototype.hasOwnProperty.call(queue, key) && Object.keys(queue).length >= MAX_ITEMS) {
      return { ok: false, error: 'La cola local ha alcanzado 500 elementos.' };
    }
    Object.defineProperty(queue, key, {
      value: { payload, expectedVersion, updatedAt, conflict: false },
      writable: true, enumerable: true, configurable: true
    });
    return { ok: true };
  }

  function remove(queue, key) {
    if (!queue || typeof queue !== 'object' || !Object.prototype.hasOwnProperty.call(queue, key)) return false;
    return delete queue[key];
  }

  function markConflict(queue, key, currentVersion, message) {
    if (!queue || typeof queue !== 'object' || !Object.prototype.hasOwnProperty.call(queue, key)) return false;
    queue[key].conflict = true;
    queue[key].currentVersion = Number.isSafeInteger(currentVersion) ? currentVersion : null;
    queue[key].lastError = String(message || 'Conflicto de sincronización.');
    return true;
  }

  window.EDOSyncQueue = Object.freeze({
    read, write, readCache, writeCache, enqueue, remove, markConflict, STORAGE_KEY, CACHE_KEY, MAX_ITEMS, MAX_PAYLOAD_BYTES
  });
})();
