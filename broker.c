#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sqlite3.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Structure for holding subscriber information
typedef struct {
    int socket;
    char topic[50];
} Subscriber;

Subscriber subscribers[MAX_CLIENTS];
int subscriber_count = 0;
sqlite3 *db;

// Function to log messages
void log_message(const char *msg) {
    openlog("PubSubBroker", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "%s", msg);
    closelog();
}

// Function to initialize the database
void init_db() {
    int rc = sqlite3_open("pubsub.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    char *err_msg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, topic TEXT, message TEXT);";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// Function to insert a message into the database
void insert_message(const char *topic, const char *message) {
    char *err_msg = 0;
    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO messages (topic, message) VALUES ('%s', '%s');", topic, message);
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// Function to handle subscriber connections
void *handle_subscriber(void *arg) {
    int new_socket = *(int *)arg;
    char buffer[BUFFER_SIZE] = {0};
    read(new_socket, buffer, BUFFER_SIZE);
    
    // Store subscriber
    if (subscriber_count < MAX_CLIENTS) {
        strcpy(subscribers[subscriber_count].topic, buffer);
        subscribers[subscriber_count].socket = new_socket;
        subscriber_count++;
        printf("Subscriber registered for topic: %s\n", buffer);
    } else {
        printf("Max subscribers reached. Connection refused.\n");
        close(new_socket);
    }
    return NULL;
}

// Function to publish messages
void publish_message(const char *topic, const char *message) {
    insert_message(topic, message);
    log_message("Publishing message...");
    for (int i = 0; i < subscriber_count; i++) {
        if (strcmp(subscribers[i].topic, topic) == 0) {
            send(subscribers[i].socket, message, strlen(message), 0);
        }
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    init_db();
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("Broker running on port %d. Waiting for subscribers...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_subscriber, (void *)&new_socket);
        pthread_detach(thread_id);
    }

    sqlite3_close(db);
    return 0;
}
