#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>

#define MAX_URLS 100
#define WORD_COUNT 3

// Important words to count
const char *important_words[WORD_COUNT] = {"Data", "Science", "Algorithm"};

// Structure to hold thread data
typedef struct {
    char *url;
    int thread_id;
} ThreadData;

void *fetch_html(void *arg){
    ThreadData *data = (ThreadData *)arg;
    CURL *curl;
    FILE *file;
    char filename[30];
    snprintf(filename, sizeof(filename), "page%d.html", data->thread_id);
    curl = curl_easy_init();
    if(curl) {
        file = fopen(filename, "wb");
        if (!file){
            fprintf(stderr, "Could not open file %s for writing\n", filename);
            return NULL;
        }
        //continue with curl
    }

}

int main(void){
printf("Startling Web Crawler");
return 0;
}