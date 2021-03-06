#include "stdio_impl.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

FILE* __fdopen(int fd, const char* mode) {
    FILE* f;

    /* Check for valid initial mode character */
    if (!strchr("rwa", *mode)) {
        errno = EINVAL;
        return 0;
    }

    /* Validate fd */
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        return 0;
    }

    /* Allocate FILE+buffer or fail */
    if (!(f = malloc(sizeof *f + UNGET + BUFSIZ)))
        return 0;

    /* Zero-fill only the struct, not the buffer */
    memset(f, 0, sizeof *f);

    /* Impose mode restrictions */
    if (!strchr(mode, '+'))
        f->flags = (*mode == 'r') ? F_NOWR : F_NORD;

    /* Apply close-on-exec flag */
    if (strchr(mode, 'e'))
        fcntl(fd, F_SETFD, FD_CLOEXEC);

    /* Set append mode on fd if opened for append */
    if (*mode == 'a') {
        if (!(flags & O_APPEND))
            fcntl(fd, F_SETFL, flags | O_APPEND);
        f->flags |= F_APP;
    }

    f->fd = fd;
    f->buf = (unsigned char*)f + sizeof *f + UNGET;
    f->buf_size = BUFSIZ;

    /* Activate line buffered mode for terminals */
    f->lbf = EOF;
    if (!(f->flags & F_NOWR) && isatty(fd))
        f->lbf = '\n';

    /* Initialize op ptrs. No problem if some are unneeded. */
    f->read = __stdio_read;
    f->write = __stdio_write;
    f->seek = __stdio_seek;
    f->close = __stdio_close;

    /* Add new FILE to open file list */
    return __ofl_add(f);
}

weak_alias(__fdopen, fdopen);
