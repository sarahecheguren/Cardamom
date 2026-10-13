# Backend

So far, I have the Drogon server running and connected to a local
MariaDB database for testing login.

Currently working:
- `/api/health` checks that the server is running.
- `/api/auth/login` checks the email and password against the database.
- `/api/auth/me` returns the user's ID if they are logged in.

Passwords are stored as hashes. Login uses a session cookie, and
restarting the server clears the sessions.

## Running it

You need Drogon, MariaDB, libsodium, CMake, and a C++ compiler installed.

From the backend folder:

```bash
cmake -S . -B build
cmake --build build -j2
```

The current test database is `cardamom_auth_dev`, with a `users` table
containing `id`, `email`, and `password_hash`. The database account is
`cardamom_login`. These need to be set up locally; they aren't included
in the repository.

Before starting the server, enter your local database password:

```bash
read -s -p "Database password: " CARDAMOM_DB_PASSWORD
```

Then run:

```bash
export CARDAMOM_DB_PASSWORD
printf '\n'
./build/backend
```

The server runs on port 8080. Open `/api/health` to check it.
There isn't a page at `/` yet.

The config.json and config.yaml files aren't currently used.
The settings are in main.cc.

## Still to do

Connect this with the team's database and frontend, add registration
and logout, and finish the HTTPS setup. This is currently for local
testing.