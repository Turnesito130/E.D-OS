export const APP_CAPABILITIES = Object.freeze(["microphone", "camera", "location", "files", "notifications"]);

export function validAppId(value) {
  return typeof value === "string" && /^[a-z0-9][a-z0-9._-]{0,63}$/.test(value);
}

export function normalizePermissionGrant(input) {
  if (!input || typeof input !== "object" || Array.isArray(input)) return null;
  const keys = Object.keys(input);
  if (keys.some(key => !APP_CAPABILITIES.includes(key))) return null;
  if (keys.some(key => typeof input[key] !== "boolean")) return null;
  return Object.fromEntries(APP_CAPABILITIES.map(capability => [capability, input[capability] === true]));
}
