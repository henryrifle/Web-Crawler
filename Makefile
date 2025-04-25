all:
	gcc -std=c11 -pedantic crawler.c -o crawler -pthread -lcurl
clean:
	rm -f crawler webpage*.html
run:
	./crawler
