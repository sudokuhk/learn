#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int gnutls_Init (void)
{
    const char *version = gnutls_check_version ("3.3.0");
    if (version == NULL) {
        printf ("unsupported GnuTLS version\n");
        return -1;
    }
    printf ("using GnuTLS version %s\n", version);
    return 0;
}

static int opensocket(const char * ip, int port)
{
    struct sockaddr_in addr;
    
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("connect to %s:%d error\n", ip, port);
        return -1;
    }
    printf("connect to %s:%d succeed\n", ip, port);
    
    return sock;
}

static ssize_t vlc_gnutls_readv(gnutls_transport_ptr_t ptr, void *buf,
                               size_t length)
{
    int sock = *(int *)ptr;
    struct iovec iov = {
        .iov_base = buf,
        .iov_len = length,
    };
    
    return readv(sock, &iov, 1);
}

static ssize_t vlc_gnutls_writev(gnutls_transport_ptr_t ptr,
                                 const giovec_t *giov, int iovcnt)
{
#ifdef IOV_MAX
    const long iovmax = IOV_MAX;
#else
    const long iovmax = sysconf(_SC_IOV_MAX);
#endif
    if (iovcnt > iovmax) {
        errno = EINVAL;
        return -1;
    }
    if (iovcnt == 0)
        return 0;
    
    int sock = *(int *)ptr;
    struct iovec iov[iovcnt];
    
    for (int i = 0; i < iovcnt; i++) {
        iov[i].iov_base = giov[i].iov_base;
        iov[i].iov_len = giov[i].iov_len;
    }
    
    return writev(sock, iov, iovcnt);
}

static ssize_t vlc_gnutls_write(gnutls_transport_ptr_t ptr,
                                void * buf, size_t size)
{
    int sock = *(int *)ptr;
    return write(sock, buf, size);
}

int main(int argc, char * argv[])
{
    gnutls_certificate_credentials_t x509;
	gnutls_session_t session;
	int val, sock;
    const char *errp;

    gnutls_Init();
    
    val = gnutls_certificate_allocate_credentials (&x509);
    if (val != 0) {
        printf ("cannot allocate credentials: %s\n", gnutls_strerror (val));
        return -1;
    }
    
    val = gnutls_certificate_set_x509_system_trust(x509);
    if (val < 0) {
        printf("cannot load trusted Certificate Authorities "
                "from %s: %s\n", "system", gnutls_strerror(val));
    } else {
        printf("loaded %d trusted CAs from %s\n", val, "system");
    }
    gnutls_certificate_set_verify_flags (x509, GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT);
    
	val = gnutls_init(&session, GNUTLS_CLIENT);
    if (val != 0) {
        printf("cannot initialize TLS session:%s\n", gnutls_strerror(val));
        return -1;
    }
    
    val = gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, x509);
    if (val < 0) {
        printf("cannot set TLS session credentials: %s", gnutls_strerror(val));
        return -1;
    }
    
    val = gnutls_priority_set_direct (session, "NORMAL", &errp);
    if (val < 0) {
        printf("cannot set TLS priorities \"%s\": %s", errp, gnutls_strerror(val));
        return -1;
    }
    
    sock = opensocket("121.9.212.220", 443);
    gnutls_transport_set_ptr(session, &sock);
    gnutls_transport_set_vec_push_function(session, vlc_gnutls_writev);
    gnutls_transport_set_pull_function(session, vlc_gnutls_readv);
    
    do {
        val = gnutls_handshake (session);
        printf("TLS handshake: %s\n", gnutls_strerror (val));
        
        switch (val) {
            case GNUTLS_E_SUCCESS:
                goto done;
            case GNUTLS_E_AGAIN:
            case GNUTLS_E_INTERRUPTED:
                /* I/O event: return to caller's poll() loop */
                return 1 + gnutls_record_get_direction (session);
        }
    }
    while (!gnutls_error_is_fatal (val));
   
done:
    printf("gnutls handshake done!\n");
    
	return 0;
}
