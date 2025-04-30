#define _GNU_SOURCE // Enables GNU-specific extensions like strcasestr() before including standard headers
#include <stdio.h>       // Standard I/O functions
#include <stdlib.h>      // Memoru allocation, exit, etc
#include <pthread.h>     // POSIX threads for multithreading
#include <curl/curl.h>   // libcurl for HTTP requests
#include <string.h>      // String manipulation functions

// Enable support for optional functions like strdup
#define __STDC_WANT_LIB_EXT1__ 1

// Global mutex used to synchronize console output between threads (prevents mixed prints)
pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

// Declare strdup function if not available (for portability across compilers)
char* strdup(const char* s);


// Define constant MAX_URLS for
// maximum number of URLs that can be processed
#define MAX_URLS 100
// Define constant WORD_COUNT for
// number of important words to count
#define WORD_COUNT 3
// Define constant MAX_URL_LENGTH for
// maximum length of a URL string
#define MAX_URL_LENGTH 2048

// Array of important words to count in the fetched HTML content
const char *important_words[WORD_COUNT] = {"Data", "Science", "Algorithm"};

// Structure to hold data passed to each thread
typedef struct {
    char *url;        // Pointer to the URL string (URL to fetch) assigned to the thread
    int thread_id;    // Unique ID for the thread, used for logging and filenames
} ThreadData;

/**
 * Callback function used by libcurl to write downloaded data to a file
 * Called repeatedly by libcurl as chunks of data are received
 * 
 * @param ptr      Pointer to the received data buffer
 * @param size     Size of each data element (usually 1)
 * @param nmemb    Number of data elements in the buffer
 * @param stream   File pointer where the data should be written
 * @return         Number of bytes successfully written to the file
 */
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);    // Write the data to the file
    return written;    // Return number of bytes written so curl knows how much was handled
}

/**
 * Reads URLs from a file and stores them in an array
 * 
 * @param filename  Name of the file containing URLs (e.g., "urls.txt")
 * @param urls      Array to store the URLs read from the file
 * @return          Number of URLs succesfully read from the file
 */
int read_urls(const char *filename, char urls[MAX_URLS][MAX_URL_LENGTH]) {
    // Open the file for reading
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open urls.txt");    // Print error if file cannot be opened
        return 0;    // Return 0 to indicate failure
    }

    // Initialize counter for the number of URLs read
    int count = 0;
    // Read URLs line by line until we reach MAX_URLS or end of file
    while (count < MAX_URLS && fgets(urls[count], MAX_URL_LENGTH, file)) {
        urls[count][strcspn(urls[count], "\r\n")] = 0;    // Remove trailing newline characters from the URL string
        count++;    // Increment the URL count
    }

    fclose(file);    // Close file after reading
    return count;    // Return the total number of URLs read
}

/**
 * Thread function that fetches a webpage and saves it to a file
 * This function is executed by each thread to download a single URL
 * 
 * @param arg  Pointer to ThreadData structure containing URL and thread ID
 * @return     NULL (required for thread functions)
 */
void* fetch_webpage(void* arg) {
    // Extract thread data from the argument (URL and thread ID)
    ThreadData* data = (ThreadData*)arg;
    CURL *curl;    // Declare a CURL object for handling HTTP requests
    FILE *file;    // Declare a FILE pointer to save the fetched HTML content
    char filename[50];    // Declare a buffer to store the filename for saving the HTML content
    
    pthread_mutex_lock(&console_mutex); // Lock the mutex to safely print from this thread (prevents output from overlapping with other threads)
    // Print status message indicating the start of the fetch operation for the thread
    printf("Thread %d: Fetching: %s\n", data->thread_id, data->url);
    pthread_mutex_unlock(&console_mutex); // Unlock the mutex after printing
    
    // Create a unique filename for the output HTML file based on the thread ID
    snprintf(filename, sizeof(filename), "page%d.html", data->thread_id);
    
    // Initialize CURL for HTTP requests
    curl = curl_easy_init();
    // Check if CURL was successfully initialized
    if(curl) {
        // Open a file for writing the fetched HTML content
        file = fopen(filename, "wb");
        // Check if the file was opened successfully
        if (!file) {
            // Print an error message if the file cannot be opened for writing
            fprintf(stderr, "Thread %d: Could not open file for writing\n", data->thread_id);
            free(data->url); // Free memory allocated for the URL string
            free(data);      // Free memory allocated for the ThreadData structure
            return NULL;     // Return NULL to indicate failure and exit the thread
        }

        // Configure CURL options
        curl_easy_setopt(curl, CURLOPT_URL, data->url);                   // Set the URL to fetch
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);    // Set callback function for data handling
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);                  // Set file to write data to
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);               // Enable follow redirects (if needed)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"); // Set user agent for the request

        // Perform the HTTP request to fetch the webpage
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors during request
        if(res != CURLE_OK) {
            // If the request failed (e.g., due to a bad URL or network error), print an error message
            fprintf(stderr, "Thread %d: Error: %s\n", data->thread_id, curl_easy_strerror(res));
        } else {
            pthread_mutex_lock(&console_mutex); // Lock the mutex to safely print from this thread (prevents output from overlapping with other threads)
            printf("Thread %d: Successfully fetched %s\n", data->thread_id, data->url);     // Print success message for this thread
            pthread_mutex_unlock(&console_mutex); // Unlock the mutex after printing
        }

        // Close the file after writing the HTML content
        fclose(file);
        // Clean up CURL resources
        curl_easy_cleanup(curl);
    }
    
    
    free(data->url);    // Free memory allocated for the URL string
    free(data);         // Free memory allocated for the ThreadData structure
    
    return NULL;
}

/**
 * Counts the occurrences of important words in a given file
 * 
 * @param filename  Name of the file to scan
 * @param word      The word to search for
 * @return          The number of occurrences found
 */
int count_word_occurrences(const char *filename, const char *word) {
    // Open the HTML file for reading
    FILE *file = fopen(filename, "r");
    // Check if the file was successfully opened
    if (!file) {
        // Print system error message
        perror("Failed to open HTML file for word counting");
        return 0;    // Return 0 if the file cannot be opened
    }
    
    int count = 0;    // Counter for occurrences of the word
    char buffer[4096];    // Buffer to read lines of the file
    
    // Read the file line by line
    while (fgets(buffer, sizeof(buffer), file)) {
        char *ptr = buffer;    // Pointer to search within the line

        // Search for the word repeatedly in the current line
        while ((ptr = strcasestr(ptr, word)) != NULL) {         // Search for the target word in the current buffer line (case-insensitive match)
        // strcasestr returns a pointer to the first occurrence of 'word' in 'ptr', or NULL if not found                                    
            count++;    // Increment count when word is found
            ptr += strlen(word);    // Move past the current match to continue searching
        }
    }

    fclose(file);    // Close the file
    return count;    // Return the total number of matches
}


/**
 *                   Main function
 * 1. Read a list of URLs from a text file ("urls.txt")
 * 2. Create a thread for each URL to download the corresponding web page
 * 3. Save the HTML content of each page to a separate local file
 * 4. After downloading, count occurrences of important words in each file
 */
int main(void) {
    // 2D Array to store URLs read from file
    // Holds up to MAX_URLS strings, each up to MAX_URL_LENGTH characters
    char urls[MAX_URLS][MAX_URL_LENGTH];
    
    // Call the read_urls function to fill the 'urls' array from "urls.txt"
    // The function returns the total number of successfully read URLs
    int url_count = read_urls("urls.txt", urls);

    // Check if any URLs were succesfully read
    // If no URLs were read from the input file, exit with an error message.
    if (url_count == 0) {
        printf("No URLs found.\n");
        return 1;    // Exit with non-zero to indicate failure
    }

    // Output how many URLs were successfully read from the input file
    printf("Read %d URLs:\n", url_count);
    for (int i = 0; i < url_count; i++) {
        // Print each URL with its index
        printf("%d: %s\n", i + 1, urls[i]);
    }

    // Array of pthread_t to store the thread identifiers for each thread
    pthread_t threads[MAX_URLS];
    printf("Allocating Threads:\n");
    
    // Create a new thread for each URL (by iterating over each URL and creating a thread to fetch it)
    for (int i = 0; i < url_count; i++) {
        // Allocate memory for the ThreadData structure
        // Which contains data to be passed to the thread function
        ThreadData* data = malloc(sizeof(ThreadData));
        // If memory allocation fails
        if (!data) {
            fprintf(stderr, "Failed to allocate memory for thread data\n");         // Output error message
            continue;    // And skip this thread
        }
        
        // Duplicate the URL string using strdup so the thread has its own copy
        data->url = strdup(urls[i]);    // strdup allocates new memory and copies the string from urls[i]
        // If string duplication (strdup) fails
        if (!data->url) {
            fprintf(stderr, "Failed to allocate memory for URL\n");                 // Output error message
            free(data);    // And free memory allocated for the ThreadData structure
            continue;    // Then skip this thread
        }
        // Validate the URL format before creating a thread
        // Skip if the URL is too short or does not start with "http"
        if (strlen(data->url) < 5 || strstr(data->url, "http") != data->url) {
            fprintf(stderr, "Skipping invalid url: %s\n", data->url); // Log skipped invalid URL
            free(data->url); // Free duplicated URL string
            free(data); // Free thread data structure
            continue; // Skip to next URL
        }
        
        // Assign a unique thread ID to this thread's data
        data->thread_id = i + 1;
        
        /* Create the thread using pthread_create
         * The thread will start running the fetch_webpage function with 'data' as argument
         */
        // If thread creation fails
        if (pthread_create(&threads[i], NULL, fetch_webpage, data) != 0) {
            fprintf(stderr, "Failed to create thread for URL %s\n", urls[i]);        // Output error message
            free(data->url);    // Free memory allocated for the URL string
            free(data);         // And free memory allocated for the ThreadData structure
        }
    }
    
    // Wait for all threads to finish their execution before moving on or exiting
    for (int i = 0; i < url_count; i++) {
        pthread_join(threads[i], NULL);
    }

    // Now that all pages have been fetched and saved, begin word counting
    printf("\nWord Occurrences:\n");

    // For each saved HTML file corresponding to the downloaded web pages
    for (int i = 0; i < url_count; i++) {
        char filename[50];    // Buffer to hold generated filename
        
        // Create filename based on the thread index ("webpage_1.html", "webpage_2.html", etc.)
        snprintf(filename, sizeof(filename), "page%d.html", i + 1);
        
        // Display name of file being analyzed
        printf("In file %s:\n", filename);

        // For each important word defined in the important_words array
        for (int j = 0; j < WORD_COUNT; j++) {
            // Call count_word_occurrences to count this word in the current file
            int count = count_word_occurrences(filename, important_words[j]);
            // Print the word and how many times it appeared
            printf("  %s: %d times\n", important_words[j], count);
        }
    }
    return 0;
}
