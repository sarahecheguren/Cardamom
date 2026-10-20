# Backend

The Drogon backend now connects to the project's MariaDB database
running through Docker Compose.

Currently working:

- `GET /api/health` checks that the server is running.
- `POST /api/auth/register` creates an account with the default Free tier.
- `POST /api/auth/login` checks the email and password and starts a session.
- `GET /api/auth/me` returns the user's ID if they are logged in.
- `POST /api/auth/logout` clears the session.

Passwords are stored as Argon2id hashes. Restarting the backend
clears the login sessions.

## Running it

You need Docker Compose, Drogon, libsodium, CMake, pkg-config,
and a C++ compiler installed.

Run the following commands from the repository root.

If you don't already have a .env file:

```bash
cp .env.example .env
```

Edit .env and set your local database passwords. If port 3306 is
already being used, change DB_PORT to 3307. Don't commit .env.

Start the database:

```bash
docker compose up -d db
docker compose ps
```

Wait until the database shows as healthy. The SQL files in
db/migrations run when the database volume is first initialized.

Build the backend:

```bash
cmake -S backend -B backend/build
cmake --build backend/build -j2
```

Load your local settings and start the server. These commands
assume the simple KEY=value format shown in .env.example:

```bash
set -a
source .env
set +a
./backend/build/backend
```

The backend reads DB_NAME, DB_USER, DB_PASSWORD, and DB_PORT.
DB_HOST defaults to 127.0.0.1.

The server runs on port 8080. Open /api/health to check it.
There isn't a page at / on this branch yet.

## Trying registration

In a second terminal:

```bash
curl -i http://localhost:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"display_name":"Test User","email":"test@example.com","password":"test-password"}'
```

Registration requires a display name, a valid email address, and
a password between 8 and 1024 bytes that isn't blank.
Display names are limited to 100 bytes and emails to 254 bytes.

A successful request returns 201. Registering an existing email
returns 409, and invalid information returns 400.
Registration doesn't log the user in automatically.

## Running the password tests

After building:

```bash
./backend/build/test/backend_test
```

The existing test checks a correct password, an incorrect password,
an empty password, and an invalid hash.

## Still to do

Connect registration with the frontend and finish the HTTPS setup.
This is currently for local testing.
