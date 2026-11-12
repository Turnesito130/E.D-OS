import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { spawn } from "node:child_process";
import { once } from "node:events";
import { Pool } from "pg";

const databaseUrl = process.env.DATABASE_URL;
if (!databaseUrl) throw new Error("DATABASE_URL is required for integration tests");
const port = 19000 + Math.floor(Math.random() * 1000);
const base = `http://127.0.0.1:${port}`;
const pool = new Pool({ connectionString: databaseUrl });
let child;

async function request(path, { method = "GET", token, body } = {}) {
  const response = await fetch(base + path, {
    method,
    headers: {
      ...(body === undefined ? {} : { "content-type": "application/json" }),
      ...(token ? { authorization: `Bearer ${token}` } : {})
    },
    body: body === undefined ? undefined : JSON.stringify(body)
  });
  let data = {};
  try { data = await response.json(); } catch {}
  return { status: response.status, data };
}

async function waitForServer() {
  for (let i = 0; i < 60; i++) {
    try {
      const response = await fetch(base + "/health");
      if (response.ok) return;
    } catch {}
    await new Promise(resolve => setTimeout(resolve, 250));
  }
  throw new Error("API did not become ready");
}

test("API integration: accounts, device pairing, sync isolation and revocation", async t => {
  await pool.query(await readFile(new URL("../schema.sql", import.meta.url), "utf8"));
  child = spawn(process.execPath, ["server.mjs"], {
    cwd: new URL("..", import.meta.url),
    env: { ...process.env, PORT: String(port), DATABASE_URL: databaseUrl, CORS_ORIGINS: "https://example.test" },
    stdio: ["ignore", "pipe", "pipe"]
  });
  let logs = "";
  child.stdout.on("data", chunk => { logs += chunk; });
  child.stderr.on("data", chunk => { logs += chunk; });
  t.after(async () => {
    if (child && child.exitCode === null) child.kill("SIGTERM");
    if (child) await Promise.race([once(child, "exit"), new Promise(resolve => setTimeout(resolve, 2000))]);
    await pool.query("TRUNCATE pairing_requests, sync_items, devices, sessions, accounts CASCADE");
    await pool.end();
  });
  await waitForServer();

  const email = `test-${Date.now()}@example.test`;
  const password = "correct-horse-battery-staple";
  const registered = await request("/v1/auth/register", { method: "POST", body: { email, password } });
  assert.equal(registered.status, 201);
  const token = registered.data.session.token;
  assert.ok(token);
  assert.equal((await request("/v1/account/me", { token })).status, 200);
  assert.equal((await request("/v1/auth/login", { method: "POST", body: { email, password: "incorrect-password" } })).status, 401);
  assert.equal((await request("/v1/auth/register", { method: "POST", body: { email, password } })).status, 409);

  const first = await request("/v1/devices", {
    method: "POST", token,
    body: { deviceName: "Equipo principal", deviceType: "computer", platform: "linux" }
  });
  assert.equal(first.status, 201);
  const firstDeviceId = first.data.device.id;
  assert.equal((await request("/v1/devices", {
    method: "POST", token,
    body: { deviceName: "Equipo duplicado", deviceType: "phone", platform: "android" }
  })).status, 409);

  const pairing = await request("/v1/pairing/requests", {
    method: "POST",
    body: { deviceName: "Segundo dispositivo", deviceType: "phone", platform: "android" }
  });
  assert.equal(pairing.status, 201);
  const pending = await request("/v1/pairing/claim", {
    method: "POST",
    body: { requestId: pairing.data.requestId, pollSecret: pairing.data.pollSecret }
  });
  assert.equal(pending.status, 202);

  const wrongCode = String((Number(pairing.data.pairingCode) + 1) % 100_000_000).padStart(8, "0");
  const badCode = await request("/v1/pairing/approve", {
    method: "POST", token,
    body: { requestId: pairing.data.requestId, pairingCode: wrongCode, approvedByDeviceId: firstDeviceId }
  });
  assert.equal(badCode.status, 404);

  const approved = await request("/v1/pairing/approve", {
    method: "POST", token,
    body: { requestId: pairing.data.requestId, pairingCode: pairing.data.pairingCode, approvedByDeviceId: firstDeviceId }
  });
  assert.equal(approved.status, 200);
  const claimed = await request("/v1/pairing/claim", {
    method: "POST",
    body: { requestId: pairing.data.requestId, pollSecret: pairing.data.pollSecret }
  });
  assert.equal(claimed.status, 200);
  assert.equal(claimed.data.device.id, approved.data.device.id);
  assert.ok(claimed.data.session.token);
  assert.equal((await request("/v1/pairing/claim", {
    method: "POST",
    body: { requestId: pairing.data.requestId, pollSecret: pairing.data.pollSecret }
  })).status, 409);

  const created = await request("/v1/sync/items/preferences", {
    method: "PUT", token, body: { expectedVersion: 0, payload: { theme: "light" } }
  });
  assert.equal(created.status, 200);
  assert.equal(created.data.item.version, 1);
  const updated = await request("/v1/sync/items/preferences", {
    method: "PUT", token, body: { expectedVersion: 1, payload: { theme: "dark" } }
  });
  assert.equal(updated.status, 200);
  assert.equal(updated.data.item.version, 2);
  assert.equal((await request("/v1/sync/items/preferences", {
    method: "PUT", token, body: { expectedVersion: 1, payload: { theme: "stale" } }
  })).status, 409);

  const other = await request("/v1/auth/register", {
    method: "POST", body: { email: `other-${Date.now()}@example.test`, password }
  });
  const otherItems = await request("/v1/sync/items", { token: other.data.session.token });
  assert.equal(otherItems.status, 200);
  assert.equal(otherItems.data.items.length, 0);

  assert.equal((await request(`/v1/devices/${firstDeviceId}/revoke`, { method: "POST", token })).status, 200);
  assert.equal((await request("/v1/devices", { token })).data.devices.length, 1);
  assert.equal((await request("/v1/auth/logout", { method: "POST", token })).status, 200);
  assert.equal((await request("/v1/account/me", { token })).status, 401);
});
