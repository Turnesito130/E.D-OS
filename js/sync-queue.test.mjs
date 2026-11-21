import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import vm from "node:vm";

const source = readFileSync(new URL("./sync-queue.js", import.meta.url), "utf8");
function createQueueApi(storage = createStorage()) {
  const window = { localStorage: storage };
  vm.runInNewContext(source, { window, TextEncoder, Date, JSON, Object, Number, String, RegExp });
  return { api: window.EDOSyncQueue, storage };
}
function createStorage(initial = null) {
  const values = new Map();
  if (initial !== null) values.set("ed-os-sync-pending-v1", initial);
  return {
    getItem(key) { return values.has(key) ? values.get(key) : null; },
    setItem(key, next) { values.set(key, next); },
    get value() { return values.get("ed-os-sync-pending-v1") ?? null; }
  };
}

test("cola local guarda, lee y elimina elementos sin perder claves especiales", () => {
  const { api, storage } = createQueueApi();
  const queue = api.read(storage);
  assert.equal(Object.getPrototypeOf(queue), null);
  assert.equal(api.enqueue(queue, "__proto__", { safe: true }, 0, 10).ok, true);
  assert.equal(api.write(queue, storage), true);
  const restored = api.read(storage);
  assert.equal(restored["__proto__"].payload.safe, true);
  assert.equal(api.remove(restored, "__proto__"), true);
  assert.equal(Object.keys(restored).length, 0);
});

test("cola local valida clave, versión y tamaño del payload", () => {
  const { api } = createQueueApi();
  const queue = api.read();
  assert.equal(api.enqueue(queue, "../bad", {}, 0).ok, false);
  assert.equal(api.enqueue(queue, "valid-key", {}, -1).ok, false);
  assert.equal(api.enqueue(queue, "valid-key", {}, 2_000_000_001).ok, false);
  assert.equal(api.enqueue(queue, "valid-key", "x".repeat(12_100), 0).ok, false);
  assert.equal(api.enqueue(queue, "valid-key", { ok: true }, 0).ok, true);
});

test("cola local limita el número de claves y marca conflictos", () => {
  const { api } = createQueueApi();
  const queue = api.read();
  for (let i = 0; i < api.MAX_ITEMS; i++) {
    assert.equal(api.enqueue(queue, "item-" + i, { i }, 0).ok, true);
  }
  assert.equal(api.enqueue(queue, "overflow", {}, 0).ok, false);
  assert.equal(api.markConflict(queue, "item-0", 7, "changed"), true);
  assert.equal(queue["item-0"].conflict, true);
  assert.equal(queue["item-0"].currentVersion, 7);
  assert.equal(api.write(queue), true);
});

test("caché local conserva versiones remotas y tombstones", () => {
  const { api, storage } = createQueueApi();
  const cache = api.readCache(storage);
  cache["settings"] = { version: 3, deleted: false, payload: { theme: "dark" } };
  cache["old-item"] = { version: 4, deleted: true, payload: null };
  assert.equal(api.writeCache(cache, storage), true);
  const restored = api.readCache(storage);
  assert.equal(restored.settings.version, 3);
  assert.equal(restored.settings.payload.theme, "dark");
  assert.equal(restored["old-item"].deleted, true);
  assert.equal(restored["old-item"].version, 4);
});

test("cola y caché mantienen aislamiento entre cuentas", () => {
  const { api, storage } = createQueueApi();
  const queueA = api.read(storage, "pending:account-a");
  const queueB = api.read(storage, "pending:account-b");
  assert.equal(api.enqueue(queueA, "settings", { theme: "dark" }, 0).ok, true);
  assert.equal(api.enqueue(queueB, "settings", { theme: "light" }, 0).ok, true);
  assert.equal(api.write(queueA, storage, "pending:account-a"), true);
  assert.equal(api.write(queueB, storage, "pending:account-b"), true);
  assert.equal(api.read(storage, "pending:account-a").settings.payload.theme, "dark");
  assert.equal(api.read(storage, "pending:account-b").settings.payload.theme, "light");
});

test("cola local tolera almacenamiento inaccesible", () => {
  const storage = {
    getItem() { throw new Error("blocked"); },
    setItem() { throw new Error("quota"); }
  };
  const { api } = createQueueApi(storage);
  assert.equal(Object.keys(api.read(storage)).length, 0);
  assert.equal(api.write({}, storage), false);
});
