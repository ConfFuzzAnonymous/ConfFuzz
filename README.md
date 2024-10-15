# Artifacts for "CONFFUZZ: A Configuration-oriented Fuzzing Framework for DBMS"


This repository includes the artifacts of the our paper: [CONFFUZZ: A Configuration-oriented Fuzzing Framework for DBMS]

The repository includes the following artifacts:

* `Data`: The data of our emprical study (Section II). 

* `Code`: Three ConfFuzz prototype systems implemented on MySQL, MariaDB and SQLite.

* `Results`: The results of evaluation on three DBMS (Section IV).

## 1. Data

We present an exhaustive collection of all studied cases of the configuration-controlled branch statements elucidated in Section III:

### 1.1 Complex branch

This consists of 375 complex branches, each of which we analyze the semantics of variables (Section II.B).

### 1.2 Dataset

Detailed dataset of our 935 configuration-controlled branch statements.

## 2. Code

### 2.1 Structrue of the source code

The structure of each DBMS prototype system is similar, and here we will use SQLite as an example to describe the structure of the source code.

   *  ConfFuzz ：The main body of ConfFuzz, including instrumentation, distance calculations, and fuzz.
   
   *  fuzz_root ：The test cases of testing phase.
   
   *  include ：Some library files, including AST, mutator, etc.

   *  src: AFL's initial library files.

   *  parser: Binary parser.

   *  temp: The intermediate output of the ConfFuzz.

### 2.2 Steps to run the code

In order to avoid inconsistent results caused by complex repro steps, we implemented the reproducible steps in 'run.py' for MySQL and MariaDB.
The 'run.py' script is located in the fuzz_root directory. For example, to run it with MySQL, first navigate to the 'fuzz_root' directory:

   *  cd Code/MySQL/fuzzing/fuzz_root

Then execute 'run.py' (make sure the Python version >= 3.4.0).

   *  python run.py

Note that in 'run.py', we use tmux to create multiple windows, including the AFL-based fuzzing window, the target software's server window, and the client window. If the AFL window does not appear after running 'run.py', first switch to the fuzzing session:

   *  tmux a -t fuzzing

Then switch to the corresponding window:

   *  tmux select-window -t afl

To start the SQLite fuzzing program:

First, navigate to the fuzz_root directory, then execute:

   *  ./afl-fuzz -z exp -c 45m -i input -o output -t 2000+ ~/sqlite/bld/sqlite3 --bail

## 3. Results

### 3.1 Configuration-controlled branches and locations
 
All the configuration-controlled branch statements we obtained (by TA) from MySQL, SQLite, and MariaDB. It includes the corresponding config option, code, and location of this statement.

### 3.2 Vulnerabilities (RQ3)

9 DBMS vulnerabilities detected by ConfFuzz. It mainly includes the test cases of these vulnerabilities.