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
//to help print fetched html 
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    printf("%.*s", (int)(size * nmemb), (char*)ptr);
    return written;
}
//fetches the html
void fetch_webpage(const char* url) {
    CURL *curl;
    FILE *file;
    
    printf("Fetching: %s\n\n", url);
    //currently made to only fetch one url at a time but can be changed to read it from url.txt
    //this it where multithreading would come into play
    curl = curl_easy_init();
    if(curl) {
        file = fopen("webpage.html", "wb");
        if (!file) {
            fprintf(stderr, "Could not open file for writing\n");
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

        CURLcode res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "\nError: %s\n", curl_easy_strerror(res));
        }

        fclose(file);
        curl_easy_cleanup(curl);
    }
}
//simple main implementation
int main(void) {
    fetch_webpage("https://example.com");
    return 0;
}