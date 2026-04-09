-- 001_init.sql: Core tables for FeatureFlagX

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Flag type enum
CREATE TYPE flag_type AS ENUM ('boolean', 'string', 'number', 'json');

-- Feature flags table
CREATE TABLE flags (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    key             VARCHAR(128) NOT NULL,
    description     TEXT NOT NULL DEFAULT '',
    flag_type       flag_type NOT NULL DEFAULT 'boolean',
    default_value   JSONB NOT NULL DEFAULT 'false'::jsonb,
    rules           JSONB NOT NULL DEFAULT '[]'::jsonb,
    enabled         BOOLEAN NOT NULL DEFAULT true,
    version         INTEGER NOT NULL DEFAULT 1,
    environment     VARCHAR(32) NOT NULL DEFAULT 'production',
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE UNIQUE INDEX idx_flags_key_env ON flags (key, environment);
CREATE INDEX idx_flags_environment ON flags (environment);

-- User role enum
CREATE TYPE user_role AS ENUM ('viewer', 'editor', 'admin');

-- Users table
CREATE TABLE users (
    id              UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    email           VARCHAR(255) NOT NULL UNIQUE,
    hashed_password VARCHAR(255) NOT NULL,
    salt            VARCHAR(64) NOT NULL,
    role            user_role NOT NULL DEFAULT 'viewer',
    api_key         VARCHAR(64) NOT NULL UNIQUE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_users_email ON users (email);
CREATE INDEX idx_users_api_key ON users (api_key);
