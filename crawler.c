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


void fetch_webpage(const char* url) {
    CURL *curl;
    FILE *file;
    
    printf("Fetching URL: %s\n", url);
    
    curl = curl_easy_init();
    if(curl) {
        
        file = fopen("webpage.html", "wb");
        if (!file) {
            fprintf(stderr, "Could not open file for writing\n");
            return;
        }

        // Setup CURL
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        CURLcode res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Read and print the HTML content
            fseek(file, 0, SEEK_SET);
            char buffer[4096];
            size_t bytes;
            
            printf("\n=== HTML Content for %s ===\n", url);
            while ((bytes = fread(buffer, 1, sizeof(buffer)-1, file)) > 0) {
                buffer[bytes] = '\0';
                printf("%s", buffer);
            }
            printf("\n=== End of HTML Content ===\n");
        }

        fclose(file);
        curl_easy_cleanup(curl);
    }
}

// Simple main function
int main(void) {
    fetch_webpage("https://example.com");
    return 0;
}