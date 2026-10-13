-- This table connects the user with their favorited recipes.
-- Favorites count toward the user's recipe limit alongside recipes they own.
CREATE TABLE IF NOT EXISTS recipe_favorites (
    user_id INT UNSIGNED NOT NULL,
    recipe_id INT UNSIGNED NOT NULL,
    PRIMARY KEY (user_id, recipe_id),
    KEY ix_recipe_favorites_recipe (recipe_id),
    CONSTRAINT fk_recipe_favorites_user
        FOREIGN KEY (user_id) REFERENCES users (id)
        ON DELETE CASCADE,
    CONSTRAINT fk_recipe_favorites_recipe
        FOREIGN KEY (recipe_id) REFERENCES recipes (id)
        ON DELETE CASCADE
) ENGINE = InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_unicode_ci;
