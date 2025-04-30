all:
	gcc -std=gnu11 -pedantic crawler.c -o crawler -pthread -lcurl

clean:
	rm -f crawler page*.html

run:
	./crawler
