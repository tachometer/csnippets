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
#define close closesocket
#else
#ifdef __use_kqueue
    #error "kqueue isn't implemented yet."
#endif
#if !defined __use_epoll && !defined __use_select
    #define __use_epoll
#endif
#endif

#endif   /* __platform_h */

