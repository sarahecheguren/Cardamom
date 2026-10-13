-- This simple table stores the data related to user tiers.
CREATE TABLE IF NOT EXISTS user_tiers (
    id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
    tier_name VARCHAR(50) NOT NULL,
    recipe_limit INT UNSIGNED NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY uq_user_tiers_name (tier_name)
) ENGINE = InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_unicode_ci;

-- The two user tiers are inserted here since they are required to run the app.
INSERT INTO user_tiers (id, tier_name, recipe_limit)
VALUES
    (1, 'Free', 25),
    (2, 'Premium', 500);
