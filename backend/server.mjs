import { createServer } from "node:http";
import { randomBytes, randomInt, timingSafeEqual, createHash } from "node:crypto";
import pg from "pg";
import { S3Client, PutObjectCommand, GetObjectCommand, HeadObjectCommand, DeleteObjectCommand } from "@aws-sdk/client-s3";
import { getSignedUrl } from "@aws-sdk/s3-request-presigner";
import { cleanupAbandonedUploads } from "./cloud-maintenance.mjs";
import { hashPassword, verifyPassword, tokenDigest, validEmail } from "./security-core.mjs";

const { Pool } = pg;
const PORT = Number(process.env.PORT || 8787);
const DATABASE_URL = process.env.DATABASE_URL;
const TOKEN_TTL_HOURS = 12;
const PAIRING_TTL_MINUTES = 5;
const MAX_BODY_BYTES = 16 * 1024;
const pool = DATABASE_URL ? new Pool({ connectionString: DATABASE_URL, ssl: process.env.PGSSLMODE === "require" ? { rejectUnauthorized: true } : undefined }) : null;
const attempts = new Map();
const allowedOrigins = new Set((process.env.CORS_ORIGINS || "").split(",").map(value => value.trim()).filter(Boolean));
const CLOUD_MAX_FILE_BYTES = 50 * 1024 * 1024;
const CLOUD_QUOTA_BYTES = 2 * 1024 * 1024 * 1024;
const CLOUD_UPLOAD_TTL_MINUTES = 60;
let cleanupRunning = false;
const cloudConfig = {
  endpoint: process.env.S3_ENDPOINT,
  bucket: process.env.S3_BUCKET,
  accessKeyId: process.env.S3_ACCESS_KEY_ID,
  secretAccessKey: process.env.S3_SECRET_ACCESS_KEY,
  region: process.env.S3_REGION || "us-east-1"
};
const cloudEnabled = [cloudConfig.endpoint, cloudConfig.bucket, cloudConfig.accessKeyId, cloudConfig.secretAccessKey].every(Boolean);
const s3 = cloudEnabled ? new S3Client({
  region: cloudConfig.region, endpoint: cloudConfig.endpoint, forcePathStyle: true,
  credentials: { accessKeyId: cloudConfig.accessKeyId, secretAccessKey: cloudConfig.secretAccessKey }
}) : null;

function send(res, status, data) {
  const body = JSON.stringify(data);
  res.writeHead(status, {
    "content-type": "application/json; charset=utf-8",
    "cache-control": "no-store",
    "x-content-type-options": "nosniff",
    "referrer-policy": "no-referrer"
  });
  res.end(body);
}

function readJson(req) {
  return new Promise((resolve, reject) => {
    let raw = "";
    req.on("data", chunk => {
      raw += chunk;
      if (Buffer.byteLength(raw) > MAX_BODY_BYTES) {
        reject(Object.assign(new Error("Cuerpo demasiado grande"), { status: 413 }));
        req.destroy();
      }
    });
    req.on("end", () => {
      try { resolve(raw ? JSON.parse(raw) : {}); }
      catch { reject(Object.assign(new Error("JSON no válido"), { status: 400 })); }
    });
    req.on("error", reject);
  });
}

function rateLimit(req, res) {
  const ip = req.socket.remoteAddress || "unknown";
  const now = Date.now();
  const entry = attempts.get(ip) || { count: 0, reset: now + 60_000 };
  if (now > entry.reset) { entry.count = 0; entry.reset = now + 60_000; }
  entry.count++;
  attempts.set(ip, entry);
  if (entry.count > 20) {
    send(res, 429, { error: "Demasiados intentos. Espera un minuto." });
    return false;
  }
  if (attempts.size > 5000) {
    for (const [key, value] of attempts) if (now > value.reset) attempts.delete(key);
  }
  return true;
}


function safeHashEqual(value, expectedHex) {
  const actual = Buffer.from(tokenDigest(value), "hex");
  const expected = Buffer.from(expectedHex, "hex");
  return actual.length === expected.length && timingSafeEqual(actual, expected);
}

function validDeviceInput(body) {
  const allowedTypes = ["phone", "tablet", "computer", "watch", "earbuds", "other"];
  return typeof body.deviceName === "string" && body.deviceName.trim().length >= 1 &&
    body.deviceName.trim().length <= 60 && allowedTypes.includes(body.deviceType) &&
    typeof body.platform === "string" && body.platform.trim().length >= 1 &&
    body.platform.trim().length <= 40;
}

async function issueSession(client, accountId, deviceId = null) {
  const token = randomBytes(32).toString("base64url");
  const expiresAt = new Date(Date.now() + TOKEN_TTL_HOURS * 3600_000);
  const saved = await client.query(
    "INSERT INTO sessions (account_id, device_id, token_hash, expires_at) VALUES ($1, $2, $3, $4) RETURNING id",
    [accountId, deviceId, tokenDigest(token), expiresAt]
  );
  return { id: saved.rows[0].id, token, expiresAt: expiresAt.toISOString() };
}


async function auditSecurityEvent(db, accountId, eventType, req, details = {}) {
  const ip = req.socket.remoteAddress || null;
  await db.query(
    "INSERT INTO account_security_events (account_id, event_type, ip_address, details) VALUES ($1, $2, $3, $4::jsonb)",
    [accountId, eventType, ip, JSON.stringify(details)]
  );
}

async function authenticatedAccount(req) {
  const header = req.headers.authorization || "";
  if (!header.startsWith("Bearer ")) return null;
  const token = header.slice(7);
  if (token.length < 40 || token.length > 100) return null;
  const result = await pool.query(
    `SELECT a.id, a.email, s.id AS session_id, s.device_id, s.created_at AS session_created_at, s.expires_at AS session_expires_at
       FROM sessions s JOIN accounts a ON a.id = s.account_id
      WHERE s.token_hash = $1 AND s.revoked_at IS NULL
        AND s.expires_at > now() AND a.status = 'active'`,
    [tokenDigest(token)]
  );
  return result.rows[0] || null;
}

const server = createServer(async (req, res) => {
  const url = new URL(req.url, "http://localhost");
  const origin = req.headers.origin;
  if (origin && allowedOrigins.has(origin)) {
    res.setHeader("access-control-allow-origin", origin);
    res.setHeader("vary", "Origin");
    res.setHeader("access-control-allow-methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.setHeader("access-control-allow-headers", "authorization, content-type, x-ed-app-token");
    res.setHeader("access-control-max-age", "600");
  }
  if (req.method === "OPTIONS") {
    if (!origin || !allowedOrigins.has(origin)) return send(res, 403, { error: "Origen no autorizado." });
    res.writeHead(204, { "cache-control": "no-store" });
    return res.end();
  }
  try {
    if (req.method === "GET" && url.pathname === "/health") {
      return send(res, 200, { service: "ed-ecosystem-api", status: "ok", version: "0.1.0" });
    }
    if (!pool) return send(res, 503, { error: "Backend sin configurar: falta DATABASE_URL." });

    if (req.method === "POST" && ["/v1/auth/register", "/v1/auth/login"].includes(url.pathname)) {
      if (!rateLimit(req, res)) return;
      const body = await readJson(req);
      const email = typeof body.email === "string" ? body.email.trim().toLowerCase() : "";
      const password = body.password;
      if (!validEmail(email) || typeof password !== "string" || password.length < 12 || password.length > 200) {
        return send(res, 400, { error: "Introduce un correo válido y una contraseña de entre 12 y 200 caracteres." });
      }

      const client = await pool.connect();
      try {
        let account;
        if (url.pathname.endsWith("/register")) {
          const passwordHash = await hashPassword(password);
          const result = await client.query(
            "INSERT INTO accounts (email, password_hash) VALUES ($1, $2) ON CONFLICT DO NOTHING RETURNING id, email",
            [email, passwordHash]
          );
          account = result.rows[0];
          if (!account) return send(res, 409, { error: "No se pudo crear la cuenta. Comprueba si el correo ya está registrado." });
        } else {
          const result = await client.query(
            "SELECT id, email, password_hash, status FROM accounts WHERE lower(email) = lower($1)",
            [email]
          );
          account = result.rows[0];
          if (!account || account.status !== "active" || !(await verifyPassword(password, account.password_hash))) {
            return send(res, 401, { error: "Correo o contraseña incorrectos." });
          }
        }
        const session = await issueSession(client, account.id);
        await auditSecurityEvent(client, account.id, url.pathname.endsWith("/register") ? "account.registered" : "auth.login", req);
        return send(res, url.pathname.endsWith("/register") ? 201 : 200, {
          account: { id: account.id, email: account.email },
          session
        });
      } finally { client.release(); }
    }


    // A new device creates a short-lived pairing request. Its poll secret is
    // separate from the eight-digit code shown to the user.
    if (req.method === "POST" && url.pathname === "/v1/pairing/requests") {
      if (!rateLimit(req, res)) return;
      const body = await readJson(req);
      if (!validDeviceInput(body)) return send(res, 400, { error: "Datos del dispositivo no válidos." });
      const code = String(randomInt(0, 100_000_000)).padStart(8, "0");
      const pollSecret = randomBytes(32).toString("base64url");
      const expiresAt = new Date(Date.now() + PAIRING_TTL_MINUTES * 60_000);
      const result = await pool.query(
        `INSERT INTO pairing_requests
          (code_hash, poll_secret_hash, device_name, device_type, platform, expires_at)
         VALUES ($1, $2, $3, $4, $5, $6)
         RETURNING id, expires_at`,
        [tokenDigest(code), tokenDigest(pollSecret), body.deviceName.trim(), body.deviceType, body.platform.trim(), expiresAt]
      );
      return send(res, 201, {
        requestId: result.rows[0].id,
        pairingCode: code,
        pollSecret,
        expiresAt: result.rows[0].expires_at,
        instructions: "Muestra el código en el nuevo dispositivo e introdúcelo desde un dispositivo E.D. ya autorizado."
      });
    }

    // An already authenticated account member approves a pairing request.
    if (req.method === "POST" && url.pathname === "/v1/pairing/approve") {
      if (!rateLimit(req, res)) return;
      const account = await authenticatedAccount(req);
      if (!account) return send(res, 401, { error: "Necesitas una sesión válida para aprobar dispositivos." });
      const body = await readJson(req);
      if (typeof body.requestId !== "string" || typeof body.pairingCode !== "string" ||
          !/^\d{8}$/.test(body.pairingCode) || typeof body.approvedByDeviceId !== "string") {
        return send(res, 400, { error: "Solicitud de aprobación no válida." });
      }
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        const trusted = await client.query(
          "SELECT id FROM devices WHERE id = $1 AND account_id = $2 AND revoked_at IS NULL FOR UPDATE",
          [body.approvedByDeviceId, account.id]
        );
        if (!trusted.rowCount) {
          await client.query("ROLLBACK");
          return send(res, 403, { error: "El dispositivo aprobador no pertenece a esta cuenta o está revocado." });
        }
        const pending = await client.query(
          "SELECT * FROM pairing_requests WHERE id = $1 AND status = 'pending' AND expires_at > now() FOR UPDATE",
          [body.requestId]
        );
        if (!pending.rowCount || !safeHashEqual(body.pairingCode, pending.rows[0].code_hash)) {
          await client.query("ROLLBACK");
          return send(res, 404, { error: "Solicitud inexistente, código incorrecto o solicitud caducada." });
        }
        const request = pending.rows[0];
        const device = await client.query(
          `INSERT INTO devices (account_id, device_name, device_type, platform)
           VALUES ($1, $2, $3, $4)
           RETURNING id, device_name, device_type, platform`,
          [account.id, request.device_name, request.device_type, request.platform]
        );
        await client.query(
          `UPDATE pairing_requests
              SET status = 'approved', account_id = $2, device_id = $3, approved_by_device_id = $4
            WHERE id = $1`,
          [request.id, account.id, device.rows[0].id, body.approvedByDeviceId]
        );
        await client.query("COMMIT");
        return send(res, 200, { success: true, device: device.rows[0], message: "Dispositivo aprobado. El nuevo equipo puede completar el emparejamiento." });
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally { client.release(); }
    }

    // The new device exchanges its private poll secret for one session exactly once.
    if (req.method === "POST" && url.pathname === "/v1/pairing/claim") {
      if (!rateLimit(req, res)) return;
      const body = await readJson(req);
      if (typeof body.requestId !== "string" || typeof body.pollSecret !== "string" ||
          body.pollSecret.length < 40 || body.pollSecret.length > 100) {
        return send(res, 400, { error: "Datos de emparejamiento no válidos." });
      }
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        const result = await client.query(
          "SELECT * FROM pairing_requests WHERE id = $1 FOR UPDATE",
          [body.requestId]
        );
        if (!result.rowCount || !safeHashEqual(body.pollSecret, result.rows[0].poll_secret_hash)) {
          await client.query("ROLLBACK");
          return send(res, 404, { error: "Solicitud no encontrada." });
        }
        const pairing = result.rows[0];
        if (pairing.expires_at <= new Date() || pairing.status === "rejected") {
          await client.query("ROLLBACK");
          return send(res, 410, { error: "La solicitud ha caducado o fue rechazada." });
        }
        if (pairing.status === "pending") {
          await client.query("ROLLBACK");
          return send(res, 202, { status: "pending", message: "Esperando aprobación desde tu otro dispositivo." });
        }
        if (pairing.status !== "approved" || !pairing.account_id || !pairing.device_id) {
          await client.query("ROLLBACK");
          return send(res, 409, { error: "Esta solicitud ya fue utilizada." });
        }
        const session = await issueSession(client, pairing.account_id, pairing.device_id);
        await client.query(
          "UPDATE pairing_requests SET status = 'claimed', claimed_at = now() WHERE id = $1",
          [pairing.id]
        );
        const device = await client.query(
          "SELECT id, device_name, device_type, platform FROM devices WHERE id = $1 AND account_id = $2 AND revoked_at IS NULL",
          [pairing.device_id, pairing.account_id]
        );
        if (!device.rowCount) {
          await client.query("ROLLBACK");
          return send(res, 410, { error: "El dispositivo fue revocado antes de completar el emparejamiento." });
        }
        await client.query("COMMIT");
        return send(res, 200, {
          status: "claimed",
          account: { id: pairing.account_id },
          device: device.rows[0],
          session
        });
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally { client.release(); }
    }

    // Las aplicaciones se autentican con credenciales propias, nunca con el token de sesión del usuario.
    if (req.method === "POST" && url.pathname === "/v1/security/authorize") {
      if (!rateLimit(req, res)) return;
      const appToken = req.headers["x-ed-app-token"] || "";
      if (typeof appToken !== "string" || appToken.length < 40 || appToken.length > 100) {
        return send(res, 401, { allowed: false, reason: "invalid_app_credential" });
      }
      const body = await readJson(req);
      const { APP_CAPABILITIES, permissionDecision } = await import("./security-permissions.mjs");
      if (!APP_CAPABILITIES.includes(body.capability)) return send(res, 400, { allowed: false, error: "Capacidad no reconocida." });
      const credential = await pool.query(
        `SELECT id, account_id, app_id FROM app_credentials
          WHERE token_hash = $1 AND revoked_at IS NULL`, [tokenDigest(appToken)]
      );
      if (!credential.rowCount) return send(res, 401, { allowed: false, reason: "invalid_app_credential" });
      const app = credential.rows[0];
      const grantResult = await pool.query(
        "SELECT granted FROM app_permission_grants WHERE account_id = $1 AND app_id = $2 AND capability = $3",
        [app.account_id, app.app_id, body.capability]
      );
      const decision = permissionDecision({ credentialActive: true, grant: grantResult.rows[0]?.granted === true });
      await pool.query("UPDATE app_credentials SET last_used_at = now() WHERE id = $1", [app.id]);
      await auditSecurityEvent(pool, app.account_id, decision.allowed ? "app.access.allowed" : "app.access.denied", req, { appId: app.app_id, capability: body.capability, reason: decision.reason });
      return send(res, decision.allowed ? 200 : 403, { ...decision, appId: app.app_id, capability: body.capability });
    }


    const account = await authenticatedAccount(req);
    if (!account) return send(res, 401, { error: "Sesión no válida o caducada." });

    // Registro y revocación de credenciales de aplicaciones (requiere sesión del titular).
    if (req.method === "POST" && url.pathname === "/v1/security/apps") {
      if (!rateLimit(req, res)) return;
      const body = await readJson(req);
      const { validAppId } = await import("./security-permissions.mjs");
      if (!validAppId(body.appId)) return send(res, 400, { error: "Identificador de aplicación no válido." });
      const appToken = randomBytes(32).toString("base64url");
      await pool.query(
        `INSERT INTO app_credentials (account_id, app_id, token_hash, revoked_at)
         VALUES ($1, $2, $3, NULL)
         ON CONFLICT (account_id, app_id)
         DO UPDATE SET token_hash = EXCLUDED.token_hash, created_at = now(), last_used_at = NULL, revoked_at = NULL`,
        [account.id, body.appId, tokenDigest(appToken)]
      );
      await auditSecurityEvent(pool, account.id, "app.credential.issued", req, { appId: body.appId });
      return send(res, 201, { appId: body.appId, appToken, warning: "Guarda este secreto ahora. No volverá a mostrarse." });
    }

    if (req.method === "DELETE" && url.pathname.startsWith("/v1/security/apps/")) {
      const appId = url.pathname.slice("/v1/security/apps/".length);
      const { validAppId } = await import("./security-permissions.mjs");
      if (!validAppId(appId)) return send(res, 400, { error: "Identificador de aplicación no válido." });
      const result = await pool.query(
        "UPDATE app_credentials SET revoked_at = now() WHERE account_id = $1 AND app_id = $2 AND revoked_at IS NULL RETURNING id",
        [account.id, appId]
      );
      if (!result.rowCount) return send(res, 404, { error: "Aplicación no encontrada o ya revocada." });
      await auditSecurityEvent(pool, account.id, "app.credential.revoked", req, { appId });
      return send(res, 200, { success: true, appId });
    }


    // Privacidad: permisos por aplicación, denegados por defecto.
    // Solo se modifica la cuenta autenticada; el servidor valida capacidades conocidas.
    if (req.method === "GET" && url.pathname === "/v1/security/app-permissions") {
      const result = await pool.query(
        "SELECT app_id, capability, granted, updated_at FROM app_permission_grants WHERE account_id = $1 ORDER BY app_id, capability",
        [account.id]
      );
      return send(res, 200, { capabilities: ["microphone", "camera", "location", "files", "notifications"], grants: result.rows });
    }

    const appPermissionMatch = url.pathname.match(/^\/v1\/security\/app-permissions\/([a-z0-9][a-z0-9._-]{0,63})$/);
    if (req.method === "PUT" && appPermissionMatch) {
      const { normalizePermissionGrant } = await import("./security-permissions.mjs");
      const body = await readJson(req);
      const grants = normalizePermissionGrant(body.grants);
      if (!grants) return send(res, 400, { error: "Permisos no válidos. Solo se admiten microphone, camera, location, files y notifications con valores booleanos." });
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        for (const [capability, granted] of Object.entries(grants)) {
          await client.query(
            `INSERT INTO app_permission_grants (account_id, app_id, capability, granted, updated_at)
             VALUES ($1, $2, $3, $4, now())
             ON CONFLICT (account_id, app_id, capability)
             DO UPDATE SET granted = EXCLUDED.granted, updated_at = now()`,
            [account.id, appPermissionMatch[1], capability, granted]
          );
        }
        await auditSecurityEvent(client, account.id, "app.permissions.updated", req, { appId: appPermissionMatch[1] });
        await client.query("COMMIT");
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally { client.release(); }
      return send(res, 200, { success: true, appId: appPermissionMatch[1], grants });
    }


    if (req.method === "GET" && url.pathname === "/v1/security/sessions") {
      const result = await pool.query(
        `SELECT id, created_at, expires_at, revoked_at,
                (id = $2::uuid) AS current
           FROM sessions
          WHERE account_id = $1 AND revoked_at IS NULL AND expires_at > now()
          ORDER BY created_at DESC LIMIT 100`,
        [account.id, account.session_id]
      );
      return send(res, 200, { sessions: result.rows });
    }

    const sessionRevokeMatch = url.pathname.match(/^\/v1\/security\/sessions\/([0-9a-f-]{36})$/i);
    if (req.method === "DELETE" && sessionRevokeMatch) {
      const result = await pool.query(
        "UPDATE sessions SET revoked_at = now() WHERE id = $1 AND account_id = $2 AND revoked_at IS NULL RETURNING id",
        [sessionRevokeMatch[1], account.id]
      );
      if (!result.rowCount) return send(res, 404, { error: "Sesión no encontrada o ya revocada." });
      await auditSecurityEvent(pool, account.id, "session.revoked", req, { sessionId: sessionRevokeMatch[1] });
      return send(res, 200, { success: true });
    }

    if (req.method === "GET" && url.pathname === "/v1/security/events") {
      const result = await pool.query(
        `SELECT event_type, created_at, ip_address
           FROM account_security_events WHERE account_id = $1
          ORDER BY created_at DESC LIMIT 100`, [account.id]
      );
      return send(res, 200, { events: result.rows });
    }

    if (req.method === "POST" && url.pathname === "/v1/auth/change-password") {
      const body = await readJson(req);
      if (typeof body.currentPassword !== "string" || typeof body.newPassword !== "string" ||
          body.newPassword.length < 12 || body.newPassword.length > 200) {
        return send(res, 400, { error: "La contraseña nueva debe tener entre 12 y 200 caracteres." });
      }
      const found = await pool.query("SELECT password_hash FROM accounts WHERE id = $1 AND status = 'active'", [account.id]);
      if (!found.rowCount || !(await verifyPassword(body.currentPassword, found.rows[0].password_hash))) {
        return send(res, 401, { error: "La contraseña actual no es correcta." });
      }
      if (body.currentPassword === body.newPassword) return send(res, 400, { error: "La contraseña nueva debe ser distinta." });
      const newHash = await hashPassword(body.newPassword);
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        await client.query("UPDATE accounts SET password_hash = $2 WHERE id = $1", [account.id, newHash]);
        await client.query("UPDATE sessions SET revoked_at = now() WHERE account_id = $1 AND id <> $2 AND revoked_at IS NULL", [account.id, account.session_id]);
        await auditSecurityEvent(client, account.id, "auth.password_changed", req);
        await client.query("COMMIT");
      } catch (error) { await client.query("ROLLBACK"); throw error; }
      finally { client.release(); }
      return send(res, 200, { success: true, message: "Contraseña actualizada. Se han cerrado las demás sesiones." });
    }

    if (req.method === "GET" && url.pathname === "/v1/account/me") {
      return send(res, 200, { account });
    }

    if (req.method === "POST" && url.pathname === "/v1/auth/logout") {
      const token = (req.headers.authorization || "").slice(7);
      await pool.query("UPDATE sessions SET revoked_at = now() WHERE token_hash = $1 AND revoked_at IS NULL", [tokenDigest(token)]);
      await auditSecurityEvent(pool, account.id, "auth.logout", req);
      return send(res, 200, { success: true });
    }


    // Small JSON sync records with optimistic version checks. This is not file sync.
    if (req.method === "GET" && url.pathname === "/v1/sync/items") {
      const result = await pool.query(
        `SELECT item_key, payload, version, deleted, updated_at
           FROM sync_items WHERE account_id = $1
          ORDER BY updated_at DESC LIMIT 500`,
        [account.id]
      );
      return send(res, 200, { items: result.rows });
    }

    const syncMatch = url.pathname.match(/^\/v1\/sync\/items\/([a-zA-Z0-9._-]{1,100})$/);
    if (req.method === "PUT" && syncMatch) {
      const body = await readJson(req);
      if (!Number.isSafeInteger(body.expectedVersion) || body.expectedVersion < 0 ||
          body.expectedVersion > 2_000_000_000 ||
          body.payload === undefined || Buffer.byteLength(JSON.stringify(body.payload)) > 12_000) {
        return send(res, 400, { error: "Se requiere expectedVersion y un payload JSON de hasta 12 KB." });
      }
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        // Serialize first-write and update races for the same account/key.
        await client.query(
          "SELECT pg_advisory_xact_lock(hashtext($1), hashtext($2))",
          [account.id, syncMatch[1]]
        );
        const current = await client.query(
          "SELECT version, deleted FROM sync_items WHERE account_id = $1 AND item_key = $2 FOR UPDATE",
          [account.id, syncMatch[1]]
        );
        const version = current.rowCount ? current.rows[0].version : 0;
        if (version !== body.expectedVersion || (current.rowCount && current.rows[0].deleted)) {
          await client.query("ROLLBACK");
          return send(res, 409, { error: "Conflicto de sincronización.", currentVersion: version });
        }
        const saved = await client.query(
          `INSERT INTO sync_items (account_id, item_key, payload, version, deleted, updated_at)
           VALUES ($1, $2, $3::jsonb, 1, false, now())
           ON CONFLICT (account_id, item_key)
           DO UPDATE SET payload = EXCLUDED.payload, version = sync_items.version + 1,
                         deleted = false, updated_at = now()
           RETURNING item_key, payload, version, deleted, updated_at`,
          [account.id, syncMatch[1], JSON.stringify(body.payload)]
        );
        await client.query("COMMIT");
        return send(res, 200, { item: saved.rows[0] });
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally { client.release(); }
    }

    if (req.method === "DELETE" && syncMatch) {
      const body = await readJson(req);
      if (!Number.isSafeInteger(body.expectedVersion) || body.expectedVersion < 1) {
        return send(res, 400, { error: "Se requiere expectedVersion para borrar." });
      }
      const result = await pool.query(
        `UPDATE sync_items SET payload = NULL, deleted = true,
                version = version + 1, updated_at = now()
          WHERE account_id = $1 AND item_key = $2 AND version = $3 AND deleted = false
          RETURNING item_key, version, deleted, updated_at`,
        [account.id, syncMatch[1], body.expectedVersion]
      );
      if (!result.rowCount) return send(res, 409, { error: "Conflicto: el elemento cambió o no existe." });
      return send(res, 200, { item: result.rows[0] });
    }

    // File bytes live in S3-compatible object storage; PostgreSQL stores metadata only.
    if (url.pathname === "/v1/cloud/files" && req.method === "GET") {
      if (!cloudEnabled) return send(res, 503, { error: "E.D. Cloud no está configurado: falta el almacenamiento S3." });
      const result = await pool.query(
        `SELECT id, file_name, content_type, size_bytes, status, created_at, completed_at
           FROM cloud_files WHERE account_id = $1 AND status = 'completed'
          ORDER BY created_at DESC LIMIT 500`, [account.id]
      );
      return send(res, 200, { files: result.rows, maxFileBytes: CLOUD_MAX_FILE_BYTES, quotaBytes: CLOUD_QUOTA_BYTES });
    }

    if (url.pathname === "/v1/cloud/files" && req.method === "POST") {
      if (!cloudEnabled) return send(res, 503, { error: "E.D. Cloud no está configurado: falta el almacenamiento S3." });
      const body = await readJson(req);
      const fileName = typeof body.fileName === "string" ? body.fileName.trim() : "";
      const contentType = typeof body.contentType === "string" ? body.contentType.trim() : "";
      const sizeBytes = body.sizeBytes;
      const invalidFileName = !fileName || fileName.length > 180 || fileName.includes("/") || fileName.includes("\\") || /[\x00-\x1f]/.test(fileName);
      const invalidContentType = !contentType || contentType.length > 150;
      const invalidSize = !Number.isSafeInteger(sizeBytes) || sizeBytes < 1 || sizeBytes > CLOUD_MAX_FILE_BYTES;
      if (invalidFileName || invalidContentType || invalidSize) {
        return send(res, 400, {
          error: "Archivo no válido. Máximo 50 MB; nombre y tipo MIME obligatorios."
        });
      }
      const id = randomBytes(16).toString("hex");
      const objectKey = `${account.id}/${id}`;
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        // Serializa reservas de cuota por cuenta para evitar que cargas
        // concurrentes reserven simultáneamente el mismo espacio disponible.
        await client.query(
          "SELECT pg_advisory_xact_lock(hashtext($1), hashtext('ed-cloud-quota'))",
          [account.id]
        );
        const usage = await client.query(
          "SELECT COALESCE(sum(size_bytes), 0)::bigint AS used FROM cloud_files WHERE account_id = $1 AND status IN ('uploading', 'completed', 'deleting')",
          [account.id]
        );
        if (Number(usage.rows[0].used) + sizeBytes > CLOUD_QUOTA_BYTES) {
          await client.query("ROLLBACK");
          return send(res, 413, { error: "Superarías la cuota experimental de 2 GB de E.D. Cloud." });
        }
        await client.query(
          `INSERT INTO cloud_files (id, account_id, object_key, file_name, content_type, size_bytes, status)
           VALUES ($1, $2, $3, $4, $5, $6, 'uploading')`,
          [id, account.id, objectKey, fileName, contentType, sizeBytes]
        );
        await client.query("COMMIT");
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally { client.release(); }

      try {
        const uploadUrl = await getSignedUrl(s3, new PutObjectCommand({
          Bucket: cloudConfig.bucket, Key: objectKey, ContentType: contentType
        }), { expiresIn: 300 });
        return send(res, 201, { file: { id, fileName, contentType, sizeBytes, status: "uploading" }, uploadUrl, uploadMethod: "PUT", requiredHeaders: { "content-type": contentType }, expiresInSeconds: 300 });
      } catch (error) {
        await pool.query("DELETE FROM cloud_files WHERE id = $1 AND account_id = $2 AND status = 'uploading'", [id, account.id]);
        throw error;
      }
    }

    const cloudFileMatch = url.pathname.match(/^\/v1\/cloud\/files\/([a-f0-9]{32})(?:\/(complete|download))?$/);
    if (cloudFileMatch) {
      if (!cloudEnabled) return send(res, 503, { error: "E.D. Cloud no está configurado: falta el almacenamiento S3." });
      const [, fileId, action] = cloudFileMatch;
      const found = await pool.query("SELECT * FROM cloud_files WHERE id = $1 AND account_id = $2", [fileId, account.id]);
      if (!found.rowCount) return send(res, 404, { error: "Archivo no encontrado." });
      const file = found.rows[0];
      if (req.method === "POST" && action === "complete") {
        if (file.status !== "uploading") return send(res, 409, { error: "El archivo no está esperando una carga." });
        let head;
        try { head = await s3.send(new HeadObjectCommand({ Bucket: cloudConfig.bucket, Key: file.object_key })); }
        catch { return send(res, 409, { error: "No se ha encontrado el archivo cargado en el almacenamiento." }); }
        if (Number(head.ContentLength) !== Number(file.size_bytes) || head.ContentType !== file.content_type) {
          await s3.send(new DeleteObjectCommand({ Bucket: cloudConfig.bucket, Key: file.object_key }));
          await pool.query("DELETE FROM cloud_files WHERE id = $1 AND account_id = $2", [fileId, account.id]);
          return send(res, 400, { error: "El tamaño o tipo del archivo no coincide; carga descartada." });
        }
        const completed = await pool.query(
          "UPDATE cloud_files SET status = 'completed', completed_at = now() WHERE id = $1 AND account_id = $2 AND status = 'uploading' RETURNING id, file_name, content_type, size_bytes, status, completed_at",
          [fileId, account.id]
        );
        if (!completed.rowCount) return send(res, 409, { error: "La carga caducó o está siendo limpiada. Vuelve a subir el archivo." });
        return send(res, 200, { file: completed.rows[0] });
      }
      if (req.method === "GET" && action === "download") {
        if (file.status !== "completed") return send(res, 409, { error: "El archivo todavía no está disponible." });
        const downloadUrl = await getSignedUrl(s3, new GetObjectCommand({
          Bucket: cloudConfig.bucket, Key: file.object_key,
          ResponseContentDisposition: `attachment; filename*=UTF-8''${encodeURIComponent(file.file_name)}`
        }), { expiresIn: 300 });
        return send(res, 200, { downloadUrl, expiresInSeconds: 300 });
      }
      if (req.method === "DELETE" && !action) {
        await s3.send(new DeleteObjectCommand({ Bucket: cloudConfig.bucket, Key: file.object_key }));
        await pool.query("DELETE FROM cloud_files WHERE id = $1 AND account_id = $2", [fileId, account.id]);
        return send(res, 200, { success: true });
      }
      return send(res, 405, { error: "Método no permitido para este archivo." });
    }

    if (req.method === "GET" && url.pathname === "/v1/devices") {
      const result = await pool.query(
        `SELECT id, device_name, device_type, platform, created_at, last_seen_at
           FROM devices WHERE account_id = $1 AND revoked_at IS NULL
          ORDER BY last_seen_at DESC`,
        [account.id]
      );
      return send(res, 200, { devices: result.rows });
    }

    if (req.method === "POST" && url.pathname === "/v1/devices") {
      const body = await readJson(req);
      const allowedTypes = ["phone", "tablet", "computer", "watch", "earbuds", "other"];
      if (typeof body.deviceName !== "string" || body.deviceName.trim().length < 1 ||
          body.deviceName.trim().length > 60 || !allowedTypes.includes(body.deviceType) ||
          typeof body.platform !== "string" || body.platform.trim().length < 1 || body.platform.length > 40) {
        return send(res, 400, { error: "Datos del dispositivo no válidos." });
      }
      const existingDevices = await pool.query(
        "SELECT 1 FROM devices WHERE account_id = $1 AND revoked_at IS NULL LIMIT 1",
        [account.id]
      );
      if (existingDevices.rowCount) {
        return send(res, 409, { error: "Esta cuenta ya tiene un dispositivo. Para añadir otro, utiliza el emparejamiento aprobado." });
      }
      const result = await pool.query(
        `INSERT INTO devices (account_id, device_name, device_type, platform)
         VALUES ($1, $2, $3, $4)
         RETURNING id, device_name, device_type, platform, created_at, last_seen_at`,
        [account.id, body.deviceName.trim(), body.deviceType, body.platform.trim()]
      );
      await pool.query(
        "UPDATE sessions SET device_id = $2 WHERE id = $1 AND account_id = $3 AND device_id IS NULL",
        [account.session_id, result.rows[0].id, account.id]
      );
      return send(res, 201, { device: result.rows[0] });
    }

    const revokeMatch = url.pathname.match(/^\/v1\/devices\/([0-9a-f-]{36})\/revoke$/i);
    if (req.method === "POST" && revokeMatch) {
      const client = await pool.connect();
      try {
        await client.query("BEGIN");
        const result = await client.query(
          "UPDATE devices SET revoked_at = now() WHERE id = $1 AND account_id = $2 AND revoked_at IS NULL RETURNING id",
          [revokeMatch[1], account.id]
        );
        if (!result.rowCount) {
          await client.query("ROLLBACK");
          return send(res, 404, { error: "Dispositivo no encontrado o ya revocado." });
        }
        const revokedSessions = await client.query(
          "UPDATE sessions SET revoked_at = now() WHERE device_id = $1 AND account_id = $2 AND revoked_at IS NULL RETURNING id",
          [revokeMatch[1], account.id]
        );
        await auditSecurityEvent(client, account.id, "device.revoked", req, {
          deviceId: revokeMatch[1], revokedSessions: revokedSessions.rowCount
        });
        await client.query("COMMIT");
        return send(res, 200, { success: true, revokedSessions: revokedSessions.rowCount });
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally { client.release(); }
    }

    return send(res, 404, { error: "Ruta no encontrada." });
  } catch (error) {
    if (error.status) return send(res, error.status, { error: error.message });
    console.error("Error interno de E.D. API:", error.message);
    return send(res, 500, { error: "Error interno del servicio." });
  }
});

async function runCloudCleanup() {
  if (!cloudEnabled || !pool || cleanupRunning) return;
  cleanupRunning = true;
  try {
    const result = await cleanupAbandonedUploads({
      pool, s3, bucket: cloudConfig.bucket, olderThanMinutes: CLOUD_UPLOAD_TTL_MINUTES
    });
    if (result.deleted || result.failed) {
      console.log(`E.D. Cloud cleanup: ${result.deleted} eliminados, ${result.failed} pendientes`);
    }
  } catch (error) {
    console.error("E.D. Cloud cleanup falló:", error.message);
  } finally { cleanupRunning = false; }
}

server.listen(PORT, () => {
  console.log(`E.D. Ecosystem API escuchando en el puerto ${PORT}`);
  const startupCleanup = setTimeout(runCloudCleanup, 30_000);
  startupCleanup.unref();
  const cleanupInterval = setInterval(runCloudCleanup, 15 * 60_000);
  cleanupInterval.unref();
});

process.on("SIGTERM", async () => {
  server.close();
  if (pool) await pool.end();
});
