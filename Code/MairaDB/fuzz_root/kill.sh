#!/bin/bash

while true; do
    process_list=$(/usr/local/mysql/bin/mariadb-admin -u root processlist)

    # echo $process_list

    process_ids=$(echo "${process_list}" | awk -F '|' '$5~"test1" && $7>60 { print $2 }')

    if [ -n "$process_ids" ]; then
        for process_id in $process_ids; do
	    echo "Killing process ID: $process_id"
	    /usr/local/mysql/bin/mariadb -u root -e "kill $process_id;"
        done
    fi

    sleep 10
done
