#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>
#define __STDC_WANT_LIB_EXT1__ 1  // Enable strdup

// Declare strdup function if not available
char* strdup(const char* s);



// Constants for program configuration
#define MAX_URLS 100        // Maximum number of URLs that can be processed
#define WORD_COUNT 3        // Number of important words to count
#define MAX_URL_LENGTH 2048 // Maximum length of a URL string

// Important words to count in the fetched HTML content
const char *important_words[WORD_COUNT] = {"Data", "Science", "Algorithm"};

// Structure to hold thread data - passed to each thread
typedef struct {
    char *url;             // The URL to fetch
    int thread_id;         // Unique identifier for each thread
} ThreadData;

/**
 * Callback function for CURL to handle the data received from the web server
 * This function is called by libcurl whenever data is received
 * 
 * @param ptr      Pointer to the received data
 * @param size     Size of each data element
 * @param nmemb    Number of data elements
 * @param stream   File pointer where the data should be written
 * @return         Number of bytes successfully written
 */
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // Write the data to the file
    size_t written = fwrite(ptr, size, nmemb, stream);
    // Uncomment the line below to print HTML to terminal (currently disabled)
    //printf("%.*s", (int)(size * nmemb), (char*)ptr);
    return written;
}

/**
 * Reads URLs from a file into an array
 * 
 * @param filename  Name of the file containing URLs
 * @param urls      Array to store the URLs
 * @return          Number of URLs read from the file
 */
int read_urls(const char *filename, char urls[MAX_URLS][MAX_URL_LENGTH]) {
    // Open the file for reading
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open urls.txt");
        return 0;
    }

    int count = 0;
    // Read URLs line by line until we reach MAX_URLS or end of file
    while (count < MAX_URLS && fgets(urls[count], MAX_URL_LENGTH, file)) {
        // Remove trailing newline character
        urls[count][strcspn(urls[count], "\r\n")] = 0;
        count++;
    }

    fclose(file);
    return count;
}

/**
 * Thread function that fetches a webpage and saves it to a file
 * This function is executed by each thread to download a single URL
 * 
 * @param arg  Pointer to ThreadData structure containing URL and thread ID
 * @return     NULL (required for thread functions)
 */
void* fetch_webpage(void* arg) {
    // Extract thread data from the argument
    ThreadData* data = (ThreadData*)arg;
    CURL *curl;
    FILE *file;
    char filename[50];
    
    // Print status message
    printf("Thread %d: Fetching: %s\n", data->thread_id, data->url);
    
    // Create a unique filename for each thread's output
    snprintf(filename, sizeof(filename), "webpage_%d.html", data->thread_id);
    
    // Initialize CURL for HTTP requests
    curl = curl_easy_init();
    if(curl) {
        // Open file for writing the HTML content
        file = fopen(filename, "wb");
        if (!file) {
            fprintf(stderr, "Thread %d: Could not open file for writing\n", data->thread_id);
            free(data->url);
            free(data);
            return NULL;
        }

        // Configure CURL options
        curl_easy_setopt(curl, CURLOPT_URL, data->url);                    // Set the URL to fetch
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);     // Set callback for data handling
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);                   // Set file to write data to
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);                // Follow redirects
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"); // Set user agent

        // Perform the HTTP request
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            fprintf(stderr, "Thread %d: Error: %s\n", data->thread_id, curl_easy_strerror(res));
        }

        // Clean up resources
        fclose(file);
        curl_easy_cleanup(curl);
    }
    
    // Free the allocated memory
    free(data->url);
    free(data);
    
    return NULL;
}

/**
 * Main function - program entry point
 * Reads URLs from file and creates threads to fetch each URL
 */
int main(void) {
    // Array to store URLs read from file
    char urls[MAX_URLS][MAX_URL_LENGTH];
    int url_count = read_urls("urls.txt", urls);

    // Check if any URLs were read
    if (url_count == 0) {
        printf("No URLs found.\n");
        return 1;
    }

    // Print the URLs that were read
    printf("Read %d URLs:\n", url_count);
    for (int i = 0; i < url_count; i++) {
        printf("%d: %s\n", i + 1, urls[i]);
    }

    // Array to store thread handles
    pthread_t threads[MAX_URLS];
    printf("Allocating Threads:\n");
    
    // Create a thread for each URL
    for (int i = 0; i < url_count; i++) {
        // Allocate memory for thread data
        ThreadData* data = malloc(sizeof(ThreadData));
        if (!data) {
            fprintf(stderr, "Failed to allocate memory for thread data\n");
            continue;
        }
        
        // Make a copy of the URL string
        data->url = strdup(urls[i]);
        if (!data->url) {
            fprintf(stderr, "Failed to allocate memory for URL\n");
            free(data);
            continue;
        }
        
        // Set thread ID
        data->thread_id = i + 1;
        
        // Create the thread
        if (pthread_create(&threads[i], NULL, fetch_webpage, data) != 0) {
            fprintf(stderr, "Failed to create thread for URL %s\n", urls[i]);
            free(data->url);
            free(data);
        }
    }
    
    // Wait for all threads to complete before exiting
    for (int i = 0; i < url_count; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
