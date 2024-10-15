#!/bin/bash

if [ $# -ne 1 ]; then
	x="databases"
else
	x="$1"
fi

DB_USER="root"
DB_NAME="test1"
OUTPUT_DIR="../output/crashes/$x.sql.s"

TABLES=$(/usr/local/mysql/bin/mariadb -u $DB_USER $DB_NAME -N -e "SHOW TABLES")

for table in $TABLES
do
	/usr/local/mysql/bin/mariadb-dump -u $DB_USER $DB_NAME $table >> $OUTPUT_DIR
done
