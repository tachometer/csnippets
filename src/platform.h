/*
 * Platform specific stuff
 */
#ifndef __platform_h
#define __platform_h

#ifdef _WIN32
#ifndef __use_select
    #define __use_select
#endif
#ifdef __use_epoll
    #undef __use_epoll
#endif
#ifdef __use_kqueue
    #undef __use_kqueue
#endif
#define ERRNO WSAGetLastError()
#define set_last_error(e) SetLastError((e))
#define E_BLOCK WSAEWOULDBLOCK
#define E_AGAIN
#define E_ISCONN WSAEISCONN
#define E_ALREADY WSAEALREADY
#define E_INPROGRESS WSAEINPROGRESS
#else
#ifdef __use_kqueue
    #error "kqueue isn't implemented yet."
#endif
#if !defined __use_epoll && !defined __use_select
    #define __use_epoll
#endif
#define ERRNO errno
#define set_last_error(e) errno = (e)
#define E_BLOCK EWOULDBLOCK
#define E_AGAIN EAGAIN
#define E_ISCONN EISCONN
#define E_ALREADY EALREADY
#define E_INPROGRESS EINPROGRESS
#endif

#endif   /* __platform_h */

