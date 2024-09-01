#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#define POOL_SIZE 10
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 80

typedef struct {
    int sockfd;
    int in_use;
} Connection;

typedef struct {
    Connection connections[POOL_SIZE];
    pthread_mutex_t lock;
} ConnectionPool;

ConnectionPool pool;

void initialize_pool() {
    pthread_mutex_init(&pool.lock, NULL);
    for (int i = 0; i < POOL_SIZE; i++) {
        pool.connections[i].sockfd = -1;
        pool.connections[i].in_use = 0;
    }
}

int create_connection() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

Connection* get_connection() {
    pthread_mutex_lock(&pool.lock);
    for (int i = 0; i < POOL_SIZE; i++) {
        if (!pool.connections[i].in_use) {
            if (pool.connections[i].sockfd == -1) {
                pool.connections[i].sockfd = create_connection();
                if (pool.connections[i].sockfd < 0) {
                    pthread_mutex_unlock(&pool.lock);
                    return NULL;
                }
            }
            pool.connections[i].in_use = 1;
            pthread_mutex_unlock(&pool.lock);
            return &pool.connections[i];
        }
    }
    pthread_mutex_unlock(&pool.lock);
    return NULL;
}

void release_connection(Connection* conn) {
    pthread_mutex_lock(&pool.lock);
    conn->in_use = 0;
    pthread_mutex_unlock(&pool.lock);
}

void clean_half_closed_connections() {
    pthread_mutex_lock(&pool.lock);
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pool.connections[i].sockfd != -1) {
            struct tcp_info info;
            socklen_t len = sizeof(info);
            if (getsockopt(pool.connections[i].sockfd, IPPROTO_TCP, TCP_INFO, &info, &len) == 0) {
                if (info.tcpi_state == TCP_CLOSE_WAIT || info.tcpi_state == TCP_LAST_ACK) {
                    close(pool.connections[i].sockfd);
                    pool.connections[i].sockfd = -1;
                    pool.connections[i].in_use = 0;
                }
            }
        }
    }
    pthread_mutex_unlock(&pool.lock);
}

void destroy_pool() {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pool.connections[i].sockfd != -1) {
            close(pool.connections[i].sockfd);
        }
    }
    pthread_mutex_destroy(&pool.lock);
}

int main() {
    initialize_pool();

    // Example usage
    Connection* conn = get_connection();
    if (conn) {
        // Use the connection to send an HTTP request here
        // ...

        // Release the connection back to the pool
        release_connection(conn);
    } else {
        printf("No available connections\n");
    }

    // Clean half-closed connections
    clean_half_closed_connections();

    destroy_pool();
    return 0;
}

