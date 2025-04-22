#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>

#define MAX_URLS 100
#define WORD_COUNT 3
#define MAX_URL_LENGTH 2048

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

int read_urls(const char *filename, char urls[MAX_URLS][MAX_URL_LENGTH]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open urls.txt");
        return 0;
    }

    int count = 0;
    while (count < MAX_URLS && fgets(urls[count], MAX_URL_LENGTH, file)) {
        // Remove trailing newline character
        urls[count][strcspn(urls[count], "\r\n")] = 0;
        count++;
    }

    fclose(file);
    return count;
}

int main(void) {
    char urls[MAX_URLS][MAX_URL_LENGTH];
    int url_count = read_urls("urls.txt", urls);

    if (url_count == 0) {
        printf("No URLs found.\n");
        return 1;
    }

    // Test: print the URLs read
    printf("Read %d URLs:\n", url_count);
    for (int i = 0; i < url_count; i++) {
        printf("%d: %s\n", i + 1, urls[i]);
    }

    // Later: Create threads to fetch each URL

    return 0;
}
