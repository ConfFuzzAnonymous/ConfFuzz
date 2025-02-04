import libtmux
import re
import time
import os

server = libtmux.Server()
session = server.new_session(session_name="fuzzing")

afl_window = session.new_window(attach=False, window_name="afl")
afl_pane = afl_window.attached_pane
afl_pane.send_keys("../afl-fuzz -i ../input -o ../output aaa")
time.sleep(10)

result = afl_pane.cmd("capture-pane", "-p").stdout
shm_str = ""
for s in result:
    if(s.startswith("SHM_ENV_VAR")):
            shm_str = s[s.find(":")+2:]

print(shm_str)
env_str = "export __AFL_SHM_ID=%s" % shm_str
shminfo = "echo %s > shm.info" %shm_str

server_window = session.new_window(attach=False, window_name="mariadb")
server_pane = server_window.attached_pane
server_pane.send_keys(env_str)
server_pane.send_keys(shminfo)
os.system("kill -9 `pidof mariadbd` >/dev/null 2>&1")
server_pane.send_keys("/usr/local/mysql/bin/mariadbd --basedir=/usr/local/mysql --datadir=/usr/local/mysql/data --log-error=duckerr.err --pid-file=duckpid.pid")
afl_pane.send_keys("")
time.sleep(30)

create_database_window_client = session.new_window(attach=False, window_name="mariadb_client")
pane_client = create_database_window_client.attached_pane
pane_client.send_keys("/usr/local/mysql/bin/mariadb -u root -e \"create database if not exists test1; create database if not exists duck;\"")
time.sleep(5)
