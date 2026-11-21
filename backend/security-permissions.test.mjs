import test from "node:test";
import assert from "node:assert/strict";
import { APP_CAPABILITIES, validAppId, normalizePermissionGrant } from "./security-permissions.mjs";

test("permission grants accept only known capabilities and boolean values", () => {
  assert.deepEqual(normalizePermissionGrant({ microphone: true, files: false }), { microphone: true, camera: false, location: false, files: false, notifications: false });
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

test("authorization denies invalid credentials and missing grants by default", async () => {
  const { permissionDecision } = await import("./security-permissions.mjs");
  assert.deepEqual(permissionDecision({ credentialActive: false, grant: true }), { allowed: false, reason: "invalid_app_credential" });
  assert.deepEqual(permissionDecision({ credentialActive: true, grant: false }), { allowed: false, reason: "permission_not_granted" });
  assert.deepEqual(permissionDecision({ credentialActive: true, grant: null }), { allowed: false, reason: "permission_not_granted" });
  assert.deepEqual(permissionDecision({ credentialActive: true, grant: true }), { allowed: true, reason: "granted" });
});
