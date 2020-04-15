all:
	gcc scheduler.c -o scheduler -lrt
	gcc child.c -o child -lrt
