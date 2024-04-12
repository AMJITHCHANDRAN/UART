#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#define BAUD_RATE B2400
#define CHUNK_SIZE 32  // Chunk size for sending/receiving data

int calculate_speed(time_t start_time, time_t end_time, int data_size) {
    double time_taken = difftime(end_time, start_time);
    int speed = (data_size * 8) / time_taken;  // Convert bytes to bits
    return speed;
}

void send_data_to_arduino(int serial_port, char *data) {
    for (int i = 0; i < strlen(data); i += CHUNK_SIZE) {
        char chunk[CHUNK_SIZE + 1];
        strncpy(chunk, &data[i], CHUNK_SIZE);
        chunk[CHUNK_SIZE] = '\0';
        write(serial_port, chunk, CHUNK_SIZE);
        usleep(10000);  // Add delay for transmission
    }
    tcflush(serial_port, TCIOFLUSH);
}

char* receive_data_from_arduino(int serial_port) {
    char received_data[EEPROM_SIZE];
    int bytes_read = 0;
    time_t start_time = time(NULL);
    while (bytes_read < EEPROM_SIZE) {
        char chunk[CHUNK_SIZE + 1];
        int bytes_available = read(serial_port, chunk, CHUNK_SIZE);
        if (bytes_available > 0) {
            strncat(received_data, chunk, bytes_available);
            bytes_read += bytes_available;
            time_t end_time = time(NULL);
            int speed = calculate_speed(start_time, end_time, bytes_read);
            printf("Incoming speed: %d bits/second\n", speed);
        }
    }
    return strdup(received_data);
}

int main() {
    int serial_port = open("/dev/ttyUSB0", O_RDWR);
    if (serial_port < 0) {
        fprintf(stderr, "Error %i from open: %s\n", errno, strerror(errno));
        return 1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(serial_port, &tty) != 0) {
        fprintf(stderr, "Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return 1;
    }
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return 1;
    }

    FILE *file = fopen("data.txt", "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }
    fseek(file, 0, SEEK_END);
    int data_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char data[data_size];
    fread(data, 1, data_size, file);
    fclose(file);

    send_data_to_arduino(serial_port, data);
    char *received_data = receive_data_from_arduino(serial_port);
    printf("Received data: %s\n", received_data);

    close(serial_port);
    free(received_data);

    return 0;
}