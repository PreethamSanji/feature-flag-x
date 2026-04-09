-- seed.sql: Default admin user and sample flags
-- Password: admin123 (SHA-256 with salt, demo only)

INSERT INTO users (email, hashed_password, salt, role, api_key) VALUES
    ('admin@featureflagx.io',
     'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
     'demo_salt_value',
     'admin',
     'ffx_ak_admin_0000000000000000000000000000000000000000');

-- Sample flags
INSERT INTO flags (key, description, flag_type, default_value, rules, enabled, environment) VALUES
    ('ui.dark_mode',
     'Enable dark mode for the user interface',
     'boolean',
     'false'::jsonb,
     '[{"attribute": "user_id", "operator": "in", "values": ["u_123", "u_456"], "rollout_percentage": 100, "value": true}]'::jsonb,
     true,
     'production'),

    ('api.rate_limit',
     'API rate limit per minute',
     'number',
     '100'::jsonb,
     '[{"attribute": "plan", "operator": "eq", "values": ["pro"], "rollout_percentage": 100, "value": 500}]'::jsonb,
     true,
     'production'),

    ('experiment.checkout_v2',
     'New checkout flow experiment',
     'boolean',
     'false'::jsonb,
     '[{"attribute": "user_id", "operator": "percentage", "values": [], "rollout_percentage": 25, "value": true}]'::jsonb,
     true,
     'staging'),

    ('config.maintenance_banner',
     'Maintenance banner message',
     'string',
     '"No scheduled maintenance"'::jsonb,
     '[]'::jsonb,
     false,
     'production');
