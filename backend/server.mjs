import { createServer } from "node:http";
import { randomBytes, randomInt, scrypt as scryptCallback, timingSafeEqual, createHash } from "node:crypto";
import { promisify } from "node:util";
import pg from "pg";

const scrypt = promisify(scryptCallback);
const { Pool } = pg;
const PORT = Number(process.env.PORT || 8787);
const DATABASE_URL = process.env.DATABASE_URL;
const TOKEN_TTL_HOURS = 12;
const PAIRING_TTL_MINUTES = 5;
const MAX_BODY_BYTES = 16 * 1024;
const pool = DATABASE_URL ? new Pool({ connectionString: DATABASE_URL, ssl: process.env.PGSSLMODE === "require" ? { rejectUnauthorized: true } : undefined }) : null;
const attempts = new Map();

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

function validEmail(value) {
  return typeof value === "string" && value.length <= 254 &&
    /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value);
}

function tokenDigest(token) {
  return createHash("sha256").update(token).digest("hex");
}

async function hashPassword(password, salt = randomBytes(16).toString("hex")) {
  const derived = await scrypt(password, salt, 64);
  return `scrypt:${salt}:${Buffer.from(derived).toString("hex")}`;
}

async function verifyPassword(password, stored) {
  const [scheme, salt, expectedHex] = String(stored).split(":");
  if (scheme !== "scrypt" || !salt || !expectedHex) return false;
  const actual = Buffer.from(await scrypt(password, salt, 64));
  const expected = Buffer.from(expectedHex, "hex");
  return actual.length === expected.length && timingSafeEqual(actual, expected);
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

async function issueSession(client, accountId) {
  const token = randomBytes(32).toString("base64url");
  const expiresAt = new Date(Date.now() + TOKEN_TTL_HOURS * 3600_000);
  await client.query(
    "INSERT INTO sessions (account_id, token_hash, expires_at) VALUES ($1, $2, $3)",
    [accountId, tokenDigest(token), expiresAt]
  );
  return { token, expiresAt: expiresAt.toISOString() };
}

async function authenticatedAccount(req) {
  const header = req.headers.authorization || "";
  if (!header.startsWith("Bearer ")) return null;
  const token = header.slice(7);
  if (token.length < 40 || token.length > 100) return null;
  const result = await pool.query(
    `SELECT a.id, a.email
       FROM sessions s JOIN accounts a ON a.id = s.account_id
      WHERE s.token_hash = $1 AND s.revoked_at IS NULL
        AND s.expires_at > now() AND a.status = 'active'`,
    [tokenDigest(token)]
  );
  return result.rows[0] || null;
}

const server = createServer(async (req, res) => {
  const url = new URL(req.url, "http://localhost");
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
          !/^\\d{8}$/.test(body.pairingCode) || typeof body.approvedByDeviceId !== "string") {
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
        const session = await issueSession(client, pairing.account_id);
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

    const account = await authenticatedAccount(req);
    if (!account) return send(res, 401, { error: "Sesión no válida o caducada." });

    if (req.method === "GET" && url.pathname === "/v1/account/me") {
      return send(res, 200, { account });
    }

    if (req.method === "POST" && url.pathname === "/v1/auth/logout") {
      const token = (req.headers.authorization || "").slice(7);
      await pool.query("UPDATE sessions SET revoked_at = now() WHERE token_hash = $1 AND revoked_at IS NULL", [tokenDigest(token)]);
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

    const syncMatch = url.pathname.match(/^\\/v1\\/sync\\/items\\/([a-zA-Z0-9._-]{1,100})$/);
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
      const result = await pool.query(
        `INSERT INTO devices (account_id, device_name, device_type, platform)
         VALUES ($1, $2, $3, $4)
         RETURNING id, device_name, device_type, platform, created_at, last_seen_at`,
        [account.id, body.deviceName.trim(), body.deviceType, body.platform.trim()]
      );
      return send(res, 201, { device: result.rows[0] });
    }

    const revokeMatch = url.pathname.match(/^\/v1\/devices\/([0-9a-f-]{36})\/revoke$/i);
    if (req.method === "POST" && revokeMatch) {
      const result = await pool.query(
        "UPDATE devices SET revoked_at = now() WHERE id = $1 AND account_id = $2 AND revoked_at IS NULL RETURNING id",
        [revokeMatch[1], account.id]
      );
      if (!result.rowCount) return send(res, 404, { error: "Dispositivo no encontrado o ya revocado." });
      return send(res, 200, { success: true });
    }

    return send(res, 404, { error: "Ruta no encontrada." });
  } catch (error) {
    if (error.status) return send(res, error.status, { error: error.message });
    console.error("Error interno de E.D. API:", error.message);
    return send(res, 500, { error: "Error interno del servicio." });
  }
});

server.listen(PORT, () => {
  console.log(`E.D. Ecosystem API escuchando en el puerto ${PORT}`);
});

process.on("SIGTERM", async () => {
  server.close();
  if (pool) await pool.end();
});
