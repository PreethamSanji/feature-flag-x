-- 002_audit_log.sql: Audit trail for flag mutations

CREATE TYPE audit_action AS ENUM ('create', 'update', 'delete', 'toggle');

CREATE TABLE audit_log (
    id          BIGSERIAL PRIMARY KEY,
    flag_id     UUID NOT NULL REFERENCES flags(id) ON DELETE CASCADE,
    user_id     UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    action      audit_action NOT NULL,
    prev_value  JSONB,
    new_value   JSONB,
    timestamp   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_audit_flag_time ON audit_log (flag_id, timestamp DESC);
CREATE INDEX idx_audit_user ON audit_log (user_id);
