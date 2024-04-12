#include <stdio.h>
#include <windows.h>
#include <time.h>

#define CHUNK_SIZE 32  // Chunk size for sending/receiving data

void send_data_to_virtual_serial(HANDLE serial_port, char *data) {
    DWORD bytes_written;
    LARGE_INTEGER start_time, end_time, frequency;
    QueryPerformanceCounter(&start_time);
    for (int i = 0; i < strlen(data); i += CHUNK_SIZE) {
        char chunk[CHUNK_SIZE + 1];
        strncpy(chunk, &data[i], CHUNK_SIZE);
        chunk[CHUNK_SIZE] = '\0';
        WriteFile(serial_port, chunk, CHUNK_SIZE, &bytes_written, NULL);
        Sleep(10);  // Add delay for transmission
    }
    FlushFileBuffers(serial_port);
    QueryPerformanceCounter(&end_time);
    QueryPerformanceFrequency(&frequency);
    double time_taken = (double)(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
    double speed = (strlen(data) * 8) / time_taken;  // Calculate speed in bits/second
    printf("Outgoing speed: %.2f bits/second\n", speed);
}

void receive_data_from_virtual_serial(HANDLE serial_port) {
    char received_data[1024];
    DWORD bytes_read;
    LARGE_INTEGER start_time, end_time, frequency;
    QueryPerformanceCounter(&start_time);
    int bytes_read_total = 0;
    while (bytes_read_total < 1024) {
        char chunk[CHUNK_SIZE + 1];
        ReadFile(serial_port, chunk, CHUNK_SIZE, &bytes_read, NULL);
        if (bytes_read > 0) {
            strncat(received_data, chunk, bytes_read);
            bytes_read_total += bytes_read;
        }
    }
    QueryPerformanceCounter(&end_time);
    QueryPerformanceFrequency(&frequency);
    double time_taken = (double)(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
    double speed = (bytes_read_total * 8) / time_taken;  // Calculate speed in bits/second
    printf("Incoming speed: %.2f bits/second\n", speed);
    printf("Received data: %s\n", received_data);
}

int main() {
    HANDLE serial_port_arduino = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (serial_port_arduino == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error %d from CreateFile\n", GetLastError());
        return 1;
    }

    HANDLE serial_port_pc = CreateFile("COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (serial_port_pc == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error %d from CreateFile\n", GetLastError());
        CloseHandle(serial_port_arduino);
        return 1;
    }

    FILE *file = fopen("data.txt", "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        CloseHandle(serial_port_arduino);
        CloseHandle(serial_port_pc);
        return 1;
    }
    fseek(file, 0, SEEK_END);
    int data_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char data[data_size];
    fread(data, 1, data_size, file);
    fclose(file);

    send_data_to_virtual_serial(serial_port_arduino, data);
    receive_data_from_virtual_serial(serial_port_pc);

    CloseHandle(serial_port_arduino);
    CloseHandle(serial_port_pc);

    return 0;
}
