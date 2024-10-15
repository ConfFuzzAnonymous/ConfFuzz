#!/bin/bash

if [ $# -ne 1 ]; then
  x="databases"
else
  x="$1"
fi

/usr/local/mysql/bin/mariadb-dump -u root test1 > ../output/crashes/"$x.sql"
mv options_string.log ../output/crashes/"$x.conf" 

