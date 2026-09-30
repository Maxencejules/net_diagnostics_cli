/* Exercise production probe error handling without making a connection. */
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET TestSocket;
typedef int TestLength;
#define TEST_PERMISSION WSAEACCES
#define TEST_REFUSED WSAECONNREFUSED
#define TEST_PENDING WSAEWOULDBLOCK
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
typedef int TestSocket;
typedef socklen_t TestLength;
#define TEST_PERMISSION EACCES
#define TEST_REFUSED ECONNREFUSED
#define TEST_PENDING EINPROGRESS
#endif

static int permission_error, delayed_error;

static int fake_connect(TestSocket sock, const struct sockaddr *address, TestLength length) {
    (void)sock; (void)address; (void)length;
    int error = delayed_error ? TEST_PENDING : (permission_error ? TEST_PERMISSION : TEST_REFUSED);
#ifdef _WIN32
    WSASetLastError(error);
#else
    errno = error;
#endif
    return -1;
}

static int fake_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    (void)nfds; (void)readfds; (void)writefds; (void)exceptfds; (void)timeout;
    return 1;
}

static int fake_getsockopt(TestSocket sock, int level, int option, char *value, TestLength *length) {
    (void)sock; (void)level; (void)option; (void)length;
    *(int *)value = permission_error ? TEST_PERMISSION : TEST_REFUSED;
    return 0;
}

#define connect fake_connect
#define select fake_select
#define getsockopt fake_getsockopt
#include "../src/scan.c"

int main(void) {
    for (delayed_error = 0; delayed_error <= 1; ++delayed_error) {
        for (permission_error = 0; permission_error <= 1; ++permission_error) {
            ScanResult result;
            int status = tcp_scan("127.0.0.1", 80, 80, 0, &result);
            if (status != permission_error || result.error_count != permission_error || result.open_total != 0) {
                fprintf(stderr, "Probe classification: delayed=%d permission=%d status=%d errors=%d\n",
                        delayed_error, permission_error, status, result.error_count);
                return 1;
            }
        }
    }
    return 0;
}
