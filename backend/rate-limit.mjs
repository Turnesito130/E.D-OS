export function createRateLimiter({
  limit = 20,
  windowMs = 60_000,
  maxEntries = 5_000
} = {}) {
  if (!Number.isSafeInteger(limit) || limit < 1 ||
      !Number.isSafeInteger(windowMs) || windowMs < 1 ||
      !Number.isSafeInteger(maxEntries) || maxEntries < 1) {
    throw new TypeError("Rate limiter settings must be positive safe integers");
  }

  const entries = new Map();

  function consume(key, now = Date.now()) {
    if (typeof key !== "string" || !key || !Number.isFinite(now)) {
      return { allowed: false, reason: "invalid_key" };
    }

    let entry = entries.get(key);
    if (!entry) {
      if (entries.size >= maxEntries) {
        for (const [candidate, value] of entries) {
          if (now >= value.resetAt) entries.delete(candidate);
        }
      }
      if (entries.size >= maxEntries) {
        return { allowed: false, reason: "capacity" };
      }
      entry = { count: 0, resetAt: now + windowMs };
      entries.set(key, entry);
    } else if (now >= entry.resetAt) {
      entry.count = 0;
      entry.resetAt = now + windowMs;
    }

    entry.count += 1;
    if (entry.count > limit) {
      return {
        allowed: false,
        reason: "rate_limited",
        retryAfterMs: Math.max(0, entry.resetAt - now)
      };
    }
    return { allowed: true, remaining: limit - entry.count };
  }

  return { consume, size: () => entries.size };
}
