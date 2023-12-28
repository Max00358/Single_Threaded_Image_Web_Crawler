COMPILER = gcc
COMPILER_FLAGS = -Wall -g -std=c99 -c -D_GNU_SOURCE
LINKER = gcc

INCLUDE_DIRS = -I/usr/include/libxml2/
LIBS = -lcurl -lxml2

#======== Valgrind ========
# valgrind --leak-check=yes --log-file=valgrind.rpt ./

#======== for "make all" =========================================================================
all: findpng3.out #all executables should be listed here

#======= for each executable, link together relevant object files and libraries==================
findpng3.out: findpng3.o http_search.o url_vector.o
	$(LINKER) -o findpng3.out findpng3.o http_search.o url_vector.o $(LIBS)

#======= for each object file, compile it from source  ===========================================
findpng3.o: ./findpng3.c
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_DIRS) -o findpng3.o       	./findpng3.c

http_search.o: ./http_search.c ./http_search.h
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_DIRS) -o http_search.o 		./http_search.c

url_vector.o: ./url_vector.c ./url_vector.h
	$(COMPILER) $(COMPILER_FLAGS) -o url_vector.o    	./url_vector.c

#======== for "make clean" ========================================================================
.PHONY: clean
clean:
	rm -f *.d *.o *.out


