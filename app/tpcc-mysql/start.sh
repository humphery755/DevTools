export LD_LIBRARY_PATH=/usr/local/mysql/lib/mysql/
DBNAME=$1
WH=$2
HOST=127.0.0.1
STEP=100

#./tpcc_start -h server_host -P port -d database_name -u mysql_user -p mysql_password -w warehouse -c connections -r warmup_time -I running_time -i report-interval -f report-file
 
./tpcc_start -h 127.0.0.1 -P 3306 -d tpcc -u root -p 000000 -w 1 -c 5 -r 1 -l 10 -i 1 -f test.t -t t_file > tt.t