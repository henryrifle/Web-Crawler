#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>

#define MAX_URLS 100
#define WORD_COUNT 3

// Important words to count
const char *important_words[WORD_COUNT] = {"Data", "Science", "Algorithm"};

printf("Startling Web Crawler.....")
