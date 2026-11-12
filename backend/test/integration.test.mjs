import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { spawn } from "node:child_process";
import { once } from "node:events";
import { Pool } from "pg";
import { S3Client, CreateBucketCommand, HeadObjectCommand } from "@aws-sdk/client-s3";
import { cleanupAbandonedUploads } from "../cloud-maintenance.mjs";
import { createS3ProtocolFixture } from "./s3-protocol-fixture.mjs";

const databaseUrl = process.env.DATABASE_URL;
if (!databaseUrl) throw new Error("DATABASE_URL is required for integration tests");
const port = 19000 + Math.floor(Math.random() * 1000);
const base = `http://127.0.0.1:${port}`;
const pool = new Pool({ connectionString: databaseUrl });
const s3 = new S3Client({
  region: "us-east-1", endpoint: "http://127.0.0.1:4566", forcePathStyle: true,
  credentials: { accessKeyId: "edtest", secretAccessKey: "ed_test_password_123" }
});
let child;
let mockS3Server;

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
  const fixture = createS3ProtocolFixture();
  mockS3Server = fixture.server;
  await new Promise((resolve, reject) => {
    mockS3Server.once("error", reject);
    mockS3Server.listen(4566, "127.0.0.1", resolve);
  });
  await s3.send(new CreateBucketCommand({ Bucket: "ed-test" }));
  child = spawn(process.execPath, ["server.mjs"], {
    cwd: new URL("..", import.meta.url),
    env: {
      ...process.env, PORT: String(port), DATABASE_URL: databaseUrl, CORS_ORIGINS: "https://example.test",
      S3_ENDPOINT: "http://127.0.0.1:4566", S3_BUCKET: "ed-test",
      S3_ACCESS_KEY_ID: "test-access-key", S3_SECRET_ACCESS_KEY: "test-secret-key",
      S3_REGION: "us-east-1"
    },
    stdio: ["ignore", "pipe", "pipe"]
  });
  let logs = "";
  child.stdout.on("data", chunk => { logs += chunk; });
  child.stderr.on("data", chunk => { logs += chunk; });
  t.after(async () => {
    if (child && child.exitCode === null) child.kill("SIGTERM");
    if (child) await Promise.race([once(child, "exit"), new Promise(resolve => setTimeout(resolve, 2000))]);
    if (mockS3Server?.listening) await new Promise(resolve => mockS3Server.close(resolve));
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

  // Comprueba que la cuota bloquea reservas concurrentes que la superarían.
  const quotaRequests = await Promise.all(Array.from({ length: 42 }, (_, index) =>
    request("/v1/cloud/files", {
      method: "POST", token,
      body: {
        fileName: `quota-${index}.bin`,
        contentType: "application/octet-stream",
        sizeBytes: 49 * 1024 * 1024
      }
    })
  ));
  assert.equal(quotaRequests.filter(result => result.status === 201).length, 41, JSON.stringify(quotaRequests.slice(0, 5)));
  assert.equal(quotaRequests.filter(result => result.status === 413).length, 1);
  const reserved = await pool.query(
    "SELECT COALESCE(sum(size_bytes), 0)::bigint AS used, count(*)::int AS files FROM cloud_files WHERE account_id = (SELECT id FROM accounts WHERE email = $1)",
    [email]
  );
  assert.equal(Number(reserved.rows[0].used), 41 * 49 * 1024 * 1024);
  assert.equal(reserved.rows[0].files, 41);

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

  // End-to-end Cloud upload, completion, signed download and deletion through the S3 HTTP fixture.
  const cloudToken = other.data.session.token;
  const payload = Buffer.from("E.D. Cloud integration test payload");
  const upload = await request("/v1/cloud/files", {
    method: "POST", token: cloudToken,
    body: { fileName: "hello.txt", contentType: "text/plain", sizeBytes: payload.length }
  });
  assert.equal(upload.status, 201);
  const put = await fetch(upload.data.uploadUrl, {
    method: "PUT", headers: upload.data.requiredHeaders, body: payload
  });
  assert.equal(put.status, 200);
  const completed = await request(`/v1/cloud/files/${upload.data.file.id}/complete`, {
    method: "POST", token: cloudToken
  });
  assert.equal(completed.status, 200);
  assert.equal(completed.data.file.status, "completed");
  const download = await request(`/v1/cloud/files/${upload.data.file.id}/download`, { token: cloudToken });
  assert.equal(download.status, 200);
  const downloaded = await fetch(download.data.downloadUrl);
  assert.equal(downloaded.status, 200);
  assert.deepEqual(Buffer.from(await downloaded.arrayBuffer()), payload);
  assert.equal((await request(`/v1/cloud/files/${upload.data.file.id}`, {
    method: "DELETE", token: cloudToken
  })).status, 200);

  // An abandoned upload is marked deleting, retried after a storage error,
  // and removed from both LocalStack S3 and PostgreSQL on the next cleanup pass.
  const abandoned = await request("/v1/cloud/files", {
    method: "POST", token: cloudToken,
    body: { fileName: "abandoned.bin", contentType: "application/octet-stream", sizeBytes: payload.length }
  });
  assert.equal(abandoned.status, 201);
  const abandonedPut = await fetch(abandoned.data.uploadUrl, {
    method: "PUT", headers: abandoned.data.requiredHeaders, body: payload
  });
  assert.equal(abandonedPut.status, 200);
  const abandonedObject = await pool.query("SELECT object_key FROM cloud_files WHERE id = $1", [abandoned.data.file.id]);
  assert.equal(abandonedObject.rowCount, 1);
  await pool.query("UPDATE cloud_files SET created_at = now() - interval '2 hours' WHERE id = $1", [abandoned.data.file.id]);
  const failedCleanup = await cleanupAbandonedUploads({
    pool, s3: { send: async () => { throw new Error("simulated temporary S3 outage"); } },
    bucket: "ed-test", olderThanMinutes: 60
  });
  assert.equal(failedCleanup.failed, 1);
  const retryState = await pool.query("SELECT status FROM cloud_files WHERE id = $1", [abandoned.data.file.id]);
  assert.equal(retryState.rows[0].status, "deleting");
  const retriedCleanup = await cleanupAbandonedUploads({ pool, s3, bucket: "ed-test", olderThanMinutes: 60 });
  assert.equal(retriedCleanup.deleted, 1);
  const abandonedRow = await pool.query("SELECT id FROM cloud_files WHERE id = $1", [abandoned.data.file.id]);
  assert.equal(abandonedRow.rowCount, 0);
  await assert.rejects(s3.send(new HeadObjectCommand({
    Bucket: "ed-test", Key: abandonedObject.rows[0].object_key
  })));

  assert.equal((await request(`/v1/devices/${firstDeviceId}/revoke`, { method: "POST", token })).status, 200);
  assert.equal((await request("/v1/devices", { token })).data.devices.length, 1);
  assert.equal((await request("/v1/auth/logout", { method: "POST", token })).status, 200);
  assert.equal((await request("/v1/account/me", { token })).status, 401);
});
