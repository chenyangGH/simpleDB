objects = test.o log.o db_operation.o db_operation_test.o rbtree.o
test:$(objects)
	gcc -o test -g $(objects) -lpthread
test.o:log.h db_operation.h
	gcc -c -g -lpthread test.c
log.o:
	gcc -c -g log.c
db_operation_test.o:common.h
	gcc -c -g db_operation_test.c
db_operation.o:common.h 
	gcc -c -g db_operation.c
rbtree.o:common.h
	gcc -c -g rbtree.c
.PHONY: clean
clean:
	rm test $(objects)
