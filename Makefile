
all: serverM serverC serverEE serverCS client 

serverM: serverM.c common.h
	gcc -g -Wall serverM.c -o serverM

serverC: serverC.c common.h
	gcc -g  -Wall serverC.c -o serverC

serverEE: serverEE.c common.h
	gcc -g  -Wall serverEE.c -o serverEE

serverCS: serverCS.c common.h
	gcc -g  -Wall serverCS.c -o serverCS

client: client.c common.h 
	gcc -g -Wall client.c -o client


clean:
	rm -f *.o client serverM serverC serverCS serverEE
	pkill serverM || echo "serverM was not running."
	pkill serverC || echo "serverC was not running."
	pkill serverEE || echo "serverEE was not running."
	pkill serverCS || echo "serverCS was not running."
	pkill client || echo "client was not running."


run:
	(./serverM &);