#!/usr/bin/env bash
# Loads db/seed.sql into the running MariaDB container. Safe to re-run.
set -euo pipefail
cd "$(dirname "$0")/.."

set -a
source .env
set +a

docker compose exec -T db mariadb -u "${DB_USER:-cardamom}" -p"${DB_PASSWORD}" "${DB_NAME:-cardamom}" < db/seed.sql
echo "Seed data loaded. Log in with any seed account and the password Secret123!"
