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

// Function to write fetched HTML to a file
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// Function to fetch HTML content
void *fetch_html(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    CURL *curl;
    FILE *file;
    char filename[30];
    snprintf(filename, sizeof(filename), "page%d.html", data->thread_id);

    curl = curl_easy_init();
    if(curl) {
        file = fopen(filename, "wb");
        if (!file) {
            fprintf(stderr, "Could not open file %s for writing\n", filename);
            free(data->url);
            free(data);
            return NULL;
        }

        curl_easy_setopt(curl, CURLOPT_URL, data->url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); 
        CURLcode res = curl_easy_perform(curl);

        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        printf("Response code for %s: %ld\n", data->url, response_code);

        if(res != CURLE_OK || response_code != 200) { // Check for network failures and invalid URLs
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            fprintf(stderr, "Failed to fetch URL: %s (Response code: %ld)\n", data->url, response_code);
        } else {
            printf("Thread %d fetched %s and saved to %s\n", data->thread_id, data->url, filename);
            // Count occurrences of important words
            fseek(file, 0, SEEK_SET); // Reset file pointer to the beginning
            char *html_content = malloc(1024 * 1024); // Allocate memory for HTML content
            size_t html_size = fread(html_content, 1, 1024 * 1024, file); // Read HTML content
            html_content[html_size] = '\0'; // Null-terminate the string

            int word_counts[WORD_COUNT] = {0}; // Array to hold counts of important words
            for (int i = 0; i < WORD_COUNT; i++) {
                const char *word = important_words[i];
                char *pos = html_content;
                while ((pos = strstr(pos, word)) != NULL) {
                    word_counts[i]++;
                    pos += strlen(word); // Move past the last found word
                }
            }

            // Log word counts
            for (int i = 0; i < WORD_COUNT; i++) {
                printf("Count of '%s' in %s: %d\n", important_words[i], filename, word_counts[i]);
            }

            free(html_content); // Free allocated memory for HTML content
        }

        fclose(file);
        curl_easy_cleanup(curl);
    }

    free(data->url);
    free(data);
    return NULL;
}

int main(void) {
    pthread_t threads[MAX_URLS];
    FILE *file = fopen("urls.txt", "r");
    char url[256];
    int thread_count = 0;

    if (file == NULL) {
        fprintf(stderr, "Could not open urls.txt for reading\n");
        return 1; 
    }

    printf("Starting the web crawler...\n");

    // Read URLs from file and create threads
    while (fgets(url, sizeof(url), file) && thread_count < MAX_URLS) {
        url[strcspn(url, "\n")] = 0; // Remove newline character
        printf("Creating thread for URL: %s\n", url); // Debug output
        ThreadData *data = malloc(sizeof(ThreadData));
        data->url = strdup(url);
        data->thread_id = thread_count;
        pthread_create(&threads[thread_count], NULL, fetch_html, data);
        thread_count++;
    }
    fclose(file);

    // Wait for all threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Web crawling completed.\n");
    return 0;
}
