-- Optional image URL for each recipe.
ALTER TABLE recipes
    ADD COLUMN IF NOT EXISTS image_url VARCHAR(2048) NULL AFTER notes;