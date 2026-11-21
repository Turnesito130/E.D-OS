import test from "node:test";
import assert from "node:assert/strict";
import { hashPassword, verifyPassword, tokenDigest, validEmail } from "./security-core.mjs";

test("password hashes use unique random salts and verify correctly", async () => {
  const first = await hashPassword("correct horse battery staple");
  const second = await hashPassword("correct horse battery staple");
  assert.match(first, /^scrypt:[a-f0-9]{32}:[a-f0-9]{128}$/);
  assert.notEqual(first, second);
  assert.equal(await verifyPassword("correct horse battery staple", first), true);
  assert.equal(await verifyPassword("wrong password", first), false);
});

test("malformed password hashes are rejected", async () => {
  assert.equal(await verifyPassword("anything", "plain:password"), false);
  assert.equal(await verifyPassword("anything", "scrypt:bad:bad"), false);
  assert.equal(await verifyPassword("anything", ""), false);
});

test("token digests are stable SHA-256 hex strings", () => {
  assert.match(tokenDigest("opaque-session-token"), /^[a-f0-9]{64}$/);
  assert.equal(tokenDigest("same"), tokenDigest("same"));
  assert.notEqual(tokenDigest("first"), tokenDigest("second"));
});

test("email validation rejects malformed and oversized values", () => {
  assert.equal(validEmail("user@example.com"), true);
  assert.equal(validEmail("not-an-email"), false);
  assert.equal(validEmail("a b@example.com"), false);
  assert.equal(validEmail("x".repeat(250) + "@a.com"), false);
});
import { APP_CAPABILITIES, validAppId, normalizePermissionGrant } from "./security-permissions.mjs";

test("app permission grants accept only known capabilities and boolean values", () => {
  assert.deepEqual(normalizePermissionGrant({ microphone: true, files: false }), {
    microphone: true, camera: false, location: false, files: false, notifications: false
  });
  assert.equal(normalizePermissionGrant({ admin: true }), null);
  assert.equal(normalizePermissionGrant({ camera: "yes" }), null);
  assert.equal(normalizePermissionGrant([]), null);
  assert.equal(APP_CAPABILITIES.length, 5);
});

test("app IDs use a strict lowercase allowlist", () => {
  assert.equal(validAppId("ed.notes"), true);
  assert.equal(validAppId("app_2-beta"), true);
  assert.equal(validAppId("../admin"), false);
  assert.equal(validAppId(""), false);
});
