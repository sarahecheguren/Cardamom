-- The recipes table. Each recipe has exactly one owner. If an owner deletes a recipe, it is removed from other users favorites.
-- Ingredients and steps are both stored as a single text field.
CREATE TABLE IF NOT EXISTS recipes (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    name VARCHAR(200) NOT NULL,
    visibility ENUM('private', 'public') NOT NULL DEFAULT 'private',
    servings SMALLINT UNSIGNED NULL,
    prep_minutes SMALLINT UNSIGNED NULL,
    cook_minutes SMALLINT UNSIGNED NULL,
    ingredient_list TEXT NOT NULL,
    steps TEXT NULL,
    notes TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY ix_recipes_owner_created (user_id, created_at),
    KEY ix_recipes_visibility (visibility),
    CONSTRAINT fk_recipes_user
        FOREIGN KEY (user_id) REFERENCES users (id)
        ON DELETE CASCADE
) ENGINE = InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_unicode_ci;
