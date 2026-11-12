-- E.D. Ecosystem — esquema inicial PostgreSQL 0.1
-- Ejecutar en una base de datos nueva. No contiene datos ni credenciales.

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS accounts (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  email TEXT NOT NULL,
  password_hash TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'suspended'))
);

CREATE UNIQUE INDEX IF NOT EXISTS accounts_email_lower_uq
  ON accounts (lower(email));

CREATE TABLE IF NOT EXISTS sessions (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  token_hash TEXT NOT NULL UNIQUE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  expires_at TIMESTAMPTZ NOT NULL,
  revoked_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS sessions_account_idx ON sessions(account_id);
CREATE INDEX IF NOT EXISTS sessions_expiry_idx ON sessions(expires_at);

CREATE TABLE IF NOT EXISTS devices (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  device_name TEXT NOT NULL,
  device_type TEXT NOT NULL CHECK (device_type IN ('phone', 'tablet', 'computer', 'watch', 'earbuds', 'other')),
  platform TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  last_seen_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  revoked_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS devices_account_idx ON devices(account_id);
CREATE INDEX IF NOT EXISTS devices_active_idx ON devices(account_id) WHERE revoked_at IS NULL;

-- Nunca guardar contraseñas ni tokens en claro en estas tablas.
-- La API almacena hashes scrypt de contraseñas y SHA-256 de tokens aleatorios.


-- Solicitudes de emparejamiento: el código visible y el secreto de consulta
-- solo se almacenan como hashes. El secreto de consulta se entrega al nuevo
-- dispositivo al crear la solicitud y nunca al dispositivo aprobador.
CREATE TABLE IF NOT EXISTS pairing_requests (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  code_hash TEXT NOT NULL,
  poll_secret_hash TEXT NOT NULL,
  device_name TEXT NOT NULL,
  device_type TEXT NOT NULL CHECK (device_type IN ('phone', 'tablet', 'computer', 'watch', 'earbuds', 'other')),
  platform TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'approved', 'claimed', 'rejected')),
  account_id UUID REFERENCES accounts(id) ON DELETE CASCADE,
  device_id UUID REFERENCES devices(id) ON DELETE SET NULL,
  approved_by_device_id UUID REFERENCES devices(id) ON DELETE SET NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  expires_at TIMESTAMPTZ NOT NULL,
  claimed_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS pairing_requests_expiry_idx ON pairing_requests(expires_at);
CREATE INDEX IF NOT EXISTS pairing_requests_account_idx ON pairing_requests(account_id);
