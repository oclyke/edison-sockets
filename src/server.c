/**
 * @file server.c
 * @author oclyke
 * @brief server best practices
 *
 * This code represents a long-time-coming need for me to
 * actually write some good C server code. It is meant to
 * clearly and simply illustrate the best practices and
 * usage of some really important functions.
 *
 * Of interest:
 * - argument parsing from argc and argv
 * - constructing a listening socket
 * - accepting client connections
 *
 * References:
 * -
 * https://blog.stephencleary.com/2009/05/using-socket-as-server-listening-socket.html
 * - pubs.opengroup.org/onlinepubs/009696799/functions/<FUNCNAME.html>
 */

#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int show_usage(char* progname);
static int start_server(
    char* hostname, int port_number, int listen_backlog,
    int* listening_sockfd_out);
static int stop_server(int server_socketfd);

int main(int argc, char* argv[]) {
  // set some initial values
  int ret = 0;
  char* hostname = "localhost";
  int port_number = -1;

  // parse arguments
  // - the supplied arguments always begins with the name of the program
  // argv[0] = program name
  // argv[1 ... n] = a string for each space-separated argument on the command
  // line
  char* progname = argv[0];

  // since we are requiring one positional argument the number of arguments
  // must be *at least* 2 (the required argument )
  if (argc < 2) {
    fprintf(stderr, "ERROR: not enough arguments supplied\n");
    show_usage(progname);
    return 1;
  }

  // parse all arguments after the program name
  for (int idx = 1; idx < argc; idx++) {
    char* arg = argv[idx];
    if (strcmp(arg, "--hostname") == 0) {
      idx++;
      hostname = argv[idx];
    } else {
      port_number = atoi(arg);
    }
  }

  // validate arguments
  if (port_number <= 0) {
    fprintf(stderr, "ERROR: invalid port number: %d\n", port_number);
    show_usage(progname);
    return 1;
  }

  // show the user the values of their arguments
  printf("Starting server at %s:%d\n", hostname, port_number);

  // start the server
  // stop_server should be called upon exit after start_server was successfully
  int server_sockfd;
  ret = start_server(hostname, port_number, 5, &server_sockfd);
  if (0 != ret) {
    fprintf(stderr, "ERROR: failed to start server\n");
    return 1;
  }

  // sit there and accept connections
  // for simplicity this will accept one connection at a time, but a
  // production server would likely accept and manage many simultaneous
  // connections
  for (;;) {
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_sockfd;

    // accept the next client
    // depending on conditions one of two things happens:
    // - when there is a pending request (up to the listen backlog amount,
    // handled
    //   by the OS) the next pending connection is returned immediately
    // - if there are no pending requests then accept() will block until one is
    // ready
    client_sockfd =
        accept(server_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_sockfd < 0) {
      fprintf(stderr, "ERROR: failed to accept the client\n");
      ret = 1;
      goto cleanup;
    }
    printf(
        "connected to client: %d (%d)\n", client_sockfd, client_addr.sin_port);

    // now that a client is connected perform simple echoing
    const size_t echo_buffer_len = 512;
    char echo_buffer[echo_buffer_len];
    while (true) {
      // read characters from the client
      int chars_received = recv(client_sockfd, echo_buffer, echo_buffer_len, 0);
      if (0 == chars_received) {
        printf("connection to client closed.\nwaiting for next connection.\n");
        break;
      } else if (chars_received < 0) {
        fprintf(
            stderr,
            "ERROR: failed to receive characters from the client. (%d)\n",
            chars_received);
        ret = 1;
        goto cleanup;
      }

      // send those characters right back to the client
      int chars_sent = send(client_sockfd, echo_buffer, chars_received, 0);
      if (chars_sent < 0) {
        fprintf(stderr, "ERROR: failed send characters back to client.\n");
        ret = 1;
        goto cleanup;
      }
    }
  }

  // indicate success by default
  // if something goes to cleanup explicitly then ret may be nonzero
  ret = 0;

cleanup:
  stop_server(server_sockfd);

  return ret;
}

static int show_usage(char* progname) {
  int ret = 0;

  printf(
      "Usage: %s [options] <listening port number>\n"
      "Options:\n"
      "--hostname <hostname>: the hostname to use, defualts to \"localhost\"\n",
      progname);

out:
  return ret;
}

/**
 * @brief starts a server
 *
 * @param hostname a string used to determine the host. should probably be one
 * of "localhost" or "0.0.0.0" or "127.0.0.1" to start a server on the device
 * @param port_number the port at which the listening socket will be opened.
 * this is the port number that clients will specify to establish a connection
 * @param listen_backlog the back
 * @param listening_sockfd_out this is an output that gives access to the file
 * descriptor of the opened socket.
 * @return int
 */
static int start_server(
    char* hostname, int port_number, int listen_backlog,
    int* listening_sockfd_out) {
  // https://blog.stephencleary.com/2009/05/using-socket-as-server-listening-socket.html
  int ret = 0;

  // construct the listening socket
  // the server will establish a *listening* socket - this socket is only used
  // to listen for incoming connections
  int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sockfd < 0) {
    fprintf(stderr, "ERROR opening listening socket\n");
    ret = 1;
    goto out;
  }

  // bind the listening socket
  // binding on a listening socket is usually only done on the port with
  // the IP address set to "any" (??? is this to allow any IP address to
  // connect? does this mean you could whitelist an IP by setting it here?)
  struct sockaddr_in serv_addr;
  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port_number);
  ret = bind(server_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  if (ret < 0) {
    fprintf(stderr, "ERROR on binding listening socket\n");
    goto out;
  }

  // start listening on the socket
  // this makes the port available for clients to try to establish a connection
  // "the listening socket actually begins listening at this point. it is not
  // yet accepting connections but the OS may accept connections on its behalf."
  ret = listen(server_sockfd, listen_backlog);
  if (0 != ret) {
    fprintf(stderr, "ERROR listening on the socket\n");
    goto out;
  }

  if (NULL != listening_sockfd_out) {
    *listening_sockfd_out = server_sockfd;
  }

out:
  return ret;
}

/**
 * @brief Stop
 *
 * @param server_sockfd
 * @return int
 */
static int stop_server(int server_sockfd) {
  int ret = 0;

  ret = close(server_sockfd);

out:
  return ret;
}
