# File Hosting Server and Client

## Project Overview

This project implements a simple file hosting server and client in C. The server and client communicate over a network connection to perform actions like listing files, downloading files, and uploading files. This project is developed as part of an engineering student's coursework and serves as a basic introduction to network programming.

## Project Features

- List files available on the server.
- Download files from the server.
- Upload files to the server.

## Project Structure

- `client.c`: The client-side code responsible for connecting to the server, sending commands, and receiving files.
- `server.c`: The server-side code responsible for accepting client connections, processing commands, and serving files.
- `pse.h`: A header file containing utility functions for network programming.

## How to Run

1. **Compile the server and client programs** using a C compiler. For example, you can use `gcc`:

   ```shell
   gcc -o server server.c
   gcc -o client client.c



2. **Run the server program**, specifying a port as a command-line argument:

    ```shell
    ./server <port>

3. **Run the client program**, specifying the server's IP address and port as command-line arguments:

    ```shell
    ./client <server_address> <port>

The client program will display a menu of available actions, allowing you to interact with the server.

## File Storage
Files are stored in the "server_files" directory on the server side.
Files uploaded by clients are stored in the "client_files" directory on the client side.
## Notes
This is a basic implementation for educational purposes and may not cover all possible edge cases or security concerns.
Consider implementing additional error handling, authentication, and encryption for a production environment.
