#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    char topic[50];
    printf("Enter topic to subscribe: ");
    fgets(topic, sizeof(topic), stdin);
    strtok(topic, "\n");  // Remove newline character

    send(sock, topic, strlen(topic), 0);
    char buffer[BUFFER_SIZE] = {0};
    while (1) {
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            printf("Message received for topic %s: %s\n", topic, buffer);
            memset(buffer, 0, BUFFER_SIZE);  // Clear the buffer
        }
    }

    close(sock);
    return 0;
}
