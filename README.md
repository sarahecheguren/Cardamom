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

The first command creates your local credentials file. The second command downloads MariaDB and starts it on port 3306. Once MariaDB starts it runs every file in the `db/migrations/` directory to create the tables.

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

### If there are schema changes

If you need to bring in new migrations, reset the database.

```bash
docker compose down -v
docker compose up -d
```
