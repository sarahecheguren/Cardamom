# Cardamom

School project built with C++ and the [Drogon](https://drogon.org) web framework, backed by MariaDB, with an HTML/CSS/JavaScript front end.

## Getting started

```bash
git clone https://github.com/sarahecheguren/Cardamom.git
cd Cardamom
```

Open the folder in VS Code and accept the prompt to install the recommended extensions. That gives you format-on-save for every file type in the project.

## Code formatting

Two formatters are used, scoped by file type. Both configs live at the repo root, so you never need to choose one manually.

| Files                    | Tool         | Config            |
| ------------------------ | ------------ | ----------------- |
| `.cc` `.cpp` `.h` `.hpp` | clang-format | `.clang-format`   |
| `.html` `.css` `.js`     | Prettier     | `.prettierrc`     |
| `.csp` Drogon views      | none         | `.prettierignore` |

### Install the tools

- **clang-format**: `brew install clang-format` on macOS, `sudo apt install clang-format` on Ubuntu, or it ships with LLVM on Windows.
- **Prettier**: needs Node.js. No install required; it runs through `npx`.

### Format before you push

```bash
# C++
clang-format -i $(git ls-files '*.cc' '*.cpp' '*.h' '*.hpp')

# HTML, CSS, JavaScript
npx prettier@3 --write .
```

Pull requests run both checks automatically. A PR with unformatted code cannot be merged, so run the commands above if the check fails.

### SQL

There is no SQL formatter. Keep keywords uppercase and one clause per line in schema and migration files.

## Contributing

1. Create a branch from `main`: `git checkout -b feature/your-change`
2. Commit your work and push the branch.
3. Open a pull request into `main`. Direct pushes to `main` are blocked.
4. Wait for the format checks to pass and get one approval, then merge.

## Database

MariaDB runs in Docker. You need [Docker Desktop](https://www.docker.com/products/docker-desktop/) installed and running.

### First-time setup

```bash
cp .env.example .env
docker compose up -d
```

The first command creates your local credentials file. The values in `.env.example` are fine for development, and `.env` is gitignored so it never reaches the repo. The second command downloads MariaDB and starts it on port 3306. Once MariaDB starts it runs every file in the `db/migrations/` directory to create the tables.

### Everday commands

```bash
docker compose up -d     ##starts the database
docker compose down      ##stops the database, but keeps the data
docker compose down -v.  ##stops the database and deletes all the data
```

To open a SQL prompt inside the container:

```bash
docker compose exec db mariadb -u cardamom -p cardamom
```

The password is the `DB_PASSWORD` value from your `.env` file.

### Seed data

To fill a running database with demo accounts and recipes:

```bash
./db/seed.sh
```

It loads `db/seed.sql`, which is safe to re-run: the seed accounts are deleted and recreated each time, so counts never drift. Every seed account uses the password `Secret123!`.

| Email               | Tier    | Recipes | Favourites | Purpose                            |
| ------------------- | ------- | ------- | ---------- | ---------------------------------- |
| `maya@example.com`  | Free    | 25      | 0          | Exactly at the Free limit          |
| `theo@example.com`  | Free    | 24      | 1          | At the limit once favourites count |
| `priya@example.com` | Premium | 30      | 2          | More recipes than the Free limit   |
| `sam@example.com`   | Free    | 3       | 2          | Below the limit                    |
| `nina@example.com`  | Free    | 0       | 0          | Empty collection                   |
| `bulk@example.com`  | Premium | 500     | 0          | Premium limit, for search timing   |

### If there are schema changes

Migrations run only once, when the data volume is first created. If you need to bring in new migrations, reset the database.

```bash
docker compose down -v
docker compose up -d
```

This deletes any data you added locally.

### Adding a migration

Create a new numbered file in `db/migrations/`, for example `005_recipe_tags.sql`. Follow the SQL formatting rules above, then reset the database as described in the previous section to test it. Each file should be safe to run on a fresh database, since that is the only way they are ever executed.
