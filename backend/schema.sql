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


-- Registros JSON pequeños sincronizados por cuenta, con control de versiones.
-- No guardar archivos binarios ni secretos en esta tabla.
CREATE TABLE IF NOT EXISTS sync_items (
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  item_key TEXT NOT NULL CHECK (length(item_key) BETWEEN 1 AND 100),
  payload JSONB,
  version INTEGER NOT NULL DEFAULT 1 CHECK (version > 0),
  deleted BOOLEAN NOT NULL DEFAULT false,
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  PRIMARY KEY (account_id, item_key),
  CHECK ((deleted = true AND payload IS NULL) OR deleted = false)
);

CREATE INDEX IF NOT EXISTS sync_items_updated_idx ON sync_items(account_id, updated_at DESC);


-- E.D. Cloud file metadata. Binary content remains in S3-compatible object storage.
CREATE TABLE IF NOT EXISTS cloud_files (
  id TEXT PRIMARY KEY CHECK (id ~ '^[a-f0-9]{32}$'),
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  object_key TEXT NOT NULL UNIQUE,
  file_name TEXT NOT NULL CHECK (length(file_name) BETWEEN 1 AND 180),
  content_type TEXT NOT NULL CHECK (length(content_type) BETWEEN 1 AND 150),
  size_bytes BIGINT NOT NULL CHECK (size_bytes BETWEEN 1 AND 52428800),
  status TEXT NOT NULL DEFAULT 'uploading' CHECK (status IN ('uploading', 'completed', 'deleting')),
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  completed_at TIMESTAMPTZ
);
CREATE INDEX IF NOT EXISTS cloud_files_account_idx ON cloud_files(account_id, created_at DESC);

-- Allow retryable cleanup state on databases created by earlier 0.1 revisions.
ALTER TABLE cloud_files DROP CONSTRAINT IF EXISTS cloud_files_status_check;
ALTER TABLE cloud_files ADD CONSTRAINT cloud_files_status_check
  CHECK (status IN ('uploading', 'completed', 'deleting'));
CREATE INDEX IF NOT EXISTS cloud_files_cleanup_idx
  ON cloud_files(created_at) WHERE status IN ('uploading', 'deleting');


-- Historial mínimo de seguridad por cuenta. No registrar contraseñas, tokens ni secretos.
CREATE TABLE IF NOT EXISTS account_security_events (
  id BIGSERIAL PRIMARY KEY,
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  event_type TEXT NOT NULL CHECK (length(event_type) BETWEEN 1 AND 80),
  ip_address INET,
  details JSONB NOT NULL DEFAULT '{}'::jsonb,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS account_security_events_account_time_idx
  ON account_security_events(account_id, created_at DESC);

-- Permisos explícitos por aplicación y cuenta. Denegado por defecto.
-- Esta tabla almacena concesiones del usuario; cada API sensible debe comprobarlas
-- en el servidor antes de entregar el recurso.
CREATE TABLE IF NOT EXISTS app_permission_grants (
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  app_id TEXT NOT NULL CHECK (app_id ~ '^[a-z0-9][a-z0-9._-]{0,63}$'),
  capability TEXT NOT NULL CHECK (capability IN ('microphone', 'camera', 'location', 'files', 'notifications')),
  granted BOOLEAN NOT NULL DEFAULT false,
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  PRIMARY KEY (account_id, app_id, capability)
);
CREATE INDEX IF NOT EXISTS app_permission_grants_app_idx
  ON app_permission_grants(app_id, capability, granted);

-- Credenciales independientes para aplicaciones registradas por el titular de la cuenta.
-- Solo se almacena el digest; el secreto se muestra una única vez al registrarlo.
CREATE TABLE IF NOT EXISTS app_credentials (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  account_id UUID NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
  app_id TEXT NOT NULL CHECK (app_id ~ '^[a-z0-9][a-z0-9._-]{0,63}$'),
  token_hash CHAR(64) NOT NULL UNIQUE CHECK (token_hash ~ '^[a-f0-9]{64}$'),
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  last_used_at TIMESTAMPTZ,
  revoked_at TIMESTAMPTZ,
  UNIQUE (account_id, app_id)
);
CREATE INDEX IF NOT EXISTS app_credentials_active_idx ON app_credentials(token_hash) WHERE revoked_at IS NULL;
