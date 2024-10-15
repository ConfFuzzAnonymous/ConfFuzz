#!/bin/bash

/usr/local/mysql/bin/mariadb-dump -u root test1 --no-data --compact > database.sql
