export LD_LIBRARY_PATH=/usr/local/mysql/lib/mysql/
DBNAME=$1
WH=$2
HOST=127.0.0.1
STEP=100

#./tpcc_load [server] [DB] [user] [pass] [warehouse_num]

./tpcc_load -h $HOST -d $DBNAME -u root -p "" -w $WH -l 1 -m 1 -n $WH >> 1.out &


