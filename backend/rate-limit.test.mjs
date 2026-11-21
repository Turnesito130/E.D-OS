import test from "node:test";
import assert from "node:assert/strict";
import { createRateLimiter } from "./rate-limit.mjs";

test("rate limiter enforces a fixed window per key", () => {
  const limiter = createRateLimiter({ limit: 2, windowMs: 1000, maxEntries: 4 });
  assert.deepEqual(limiter.consume("client-a", 100), { allowed: true, remaining: 1 });
  assert.deepEqual(limiter.consume("client-a", 200), { allowed: true, remaining: 0 });
  assert.deepEqual(limiter.consume("client-a", 300), {
    allowed: false, reason: "rate_limited", retryAfterMs: 800
  });
  assert.deepEqual(limiter.consume("client-a", 1100), { allowed: true, remaining: 1 });
});

test("rate limiter bounds memory and evicts expired keys before rejecting new ones", () => {
  const limiter = createRateLimiter({ limit: 5, windowMs: 1000, maxEntries: 2 });
  assert.equal(limiter.consume("a", 100).allowed, true);
  assert.equal(limiter.consume("b", 100).allowed, true);
  assert.equal(limiter.size(), 2);
  assert.deepEqual(limiter.consume("c", 200), { allowed: false, reason: "capacity" });
  assert.equal(limiter.size(), 2);
  assert.equal(limiter.consume("c", 1100).allowed, true);
  assert.equal(limiter.size(), 1);
});

test("rate limiter rejects invalid keys and configuration", () => {
  const limiter = createRateLimiter();
  assert.equal(limiter.consume("").reason, "invalid_key");
  assert.throws(() => createRateLimiter({ limit: 0 }), TypeError);
  assert.throws(() => createRateLimiter({ maxEntries: -1 }), TypeError);
});
