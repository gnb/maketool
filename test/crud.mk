
all:: error1

error1:
	gcc -ansi -Wall -c -DCURRDIR=\"$(CURRDIR)\" crud.c
	
clean::
	$(RM) crud.o

