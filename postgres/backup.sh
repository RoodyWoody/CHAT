#!/bin/bash
set -e

PGUSER=${POSTGRES_USER}
PGPASSWORD=${POSTGRES_PASSWORD}
PGDB=${POSTGRES_DB}

PG_DUMP="/usr/bin/pg_dump"
BACKUP_DIR="/backups"
DATE=$(date +"%Y%m%d")
BACKUP_FILE="$BACKUP_DIR/backup_$DATE.sql"

$PG_DUMP -h localhost -U "$PGUSER" -d "$PGDB" > "$BACKUP_FILE"
