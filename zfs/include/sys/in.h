#ifndef _SYS_IN_H
#define _SYS_IN_H

#include <sys/types.h>

#define	IN6_IS_ADDR_UNSPECIFIED(addr) \
	(((addr)->s6_addr32[3] == 0) && \
	((addr)->s6_addr32[2] == 0) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0))

#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_LOOPBACK(addr) \
	(((addr)->s6_addr32[3] == 0x00000001) && \
	((addr)->s6_addr32[2] == 0) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0))
#else /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_LOOPBACK(addr) \
	(((addr)->s6_addr32[3] == 0x01000000) && \
	((addr)->s6_addr32[2] == 0) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0))
#endif /* _BIG_ENDIAN */

#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_MULTICAST(addr) \
	(((addr)->s6_addr32[0] & 0xff000000) == 0xff000000)
#else /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_MULTICAST(addr) \
	(((addr)->s6_addr32[0] & 0x000000ff) == 0x000000ff)
#endif /* _BIG_ENDIAN */

#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_LINKLOCAL(addr) \
	(((addr)->s6_addr32[0] & 0xffc00000) == 0xfe800000)
#else /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_LINKLOCAL(addr) \
	(((addr)->s6_addr32[0] & 0x0000c0ff) == 0x000080fe)
#endif /* _BIG_ENDIAN */

#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_SITELOCAL(addr) \
	(((addr)->s6_addr32[0] & 0xffc00000) == 0xfec00000)
#else /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_SITELOCAL(addr) \
	(((addr)->s6_addr32[0] & 0x0000c0ff) == 0x0000c0fe)
#endif /* _BIG_ENDIAN */

/*
 * IN6_IS_ADDR_V4MAPPED - A IPv4 mapped INADDR_ANY
 * Note: This macro is currently NOT defined in RFC2553 specification
 * and not a standard macro that portable applications should use.
 */
#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_V4MAPPED(addr) \
	(((addr)->s6_addr32[2] == 0x0000ffff) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0))
#else  /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_V4MAPPED(addr) \
	(((addr)->s6_addr32[2] == 0xffff0000U) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0))
#endif /* _BIG_ENDIAN */

/* Exclude loopback and unspecified address */
#ifdef _BIG_ENDIAN
#define	IN6_IS_ADDR_V4COMPAT(addr) \
	(((addr)->s6_addr32[2] == 0) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0) && \
	!((addr)->s6_addr32[3] == 0) && \
	!((addr)->s6_addr32[3] == 0x00000001))

#else /* _BIG_ENDIAN */
#define	IN6_IS_ADDR_V4COMPAT(addr) \
	(((addr)->s6_addr32[2] == 0) && \
	((addr)->s6_addr32[1] == 0) && \
	((addr)->s6_addr32[0] == 0) && \
	!((addr)->s6_addr32[3] == 0) && \
	!((addr)->s6_addr32[3] == 0x01000000))
#endif /* _BIG_ENDIAN */

#define	IN6_V4MAPPED_TO_INADDR(v6, v4) \
	((v4)->s_addr = (v6)->s6_addr32[3])

/*
 * IN6_INADDR_TO_V4MAPPED
 * IN6_IPADDR_TO_V4MAPPED
 *	Assign a IPv4 address address to an IPv6 address as a IPv4-mapped
 *	address.
 *	Note: These macros are NOT defined in RFC2553 or any other standard
 *	specification and are not macros that portable applications should
 *	use.
 *
 * void IN6_INADDR_TO_V4MAPPED(const struct in_addr *v4, in6_addr_t *v6);
 * void IN6_IPADDR_TO_V4MAPPED(const ipaddr_t v4, in6_addr_t *v6);
 *
 */
#ifdef _BIG_ENDIAN
#define	IN6_INADDR_TO_V4MAPPED(v4, v6) \
	((v6)->s6_addr32[3] = (v4)->s_addr, \
	(v6)->s6_addr32[2] = 0x0000ffff, \
	(v6)->s6_addr32[1] = 0, \
	(v6)->s6_addr32[0] = 0)
#define	IN6_IPADDR_TO_V4MAPPED(v4, v6) \
	((v6)->s6_addr32[3] = (v4), \
	(v6)->s6_addr32[2] = 0x0000ffff, \
	(v6)->s6_addr32[1] = 0, \
	(v6)->s6_addr32[0] = 0)
#else /* _BIG_ENDIAN */
#define	IN6_INADDR_TO_V4MAPPED(v4, v6) \
	((v6)->s6_addr32[3] = (v4)->s_addr, \
	(v6)->s6_addr32[2] = 0xffff0000U, \
	(v6)->s6_addr32[1] = 0, \
	(v6)->s6_addr32[0] = 0)
#define	IN6_IPADDR_TO_V4MAPPED(v4, v6) \
	((v6)->s6_addr32[3] = (v4), \
	(v6)->s6_addr32[2] = 0xffff0000U, \
	(v6)->s6_addr32[1] = 0, \
	(v6)->s6_addr32[0] = 0)
#endif /* _BIG_ENDIAN */

#ifndef _IN_ADDR_T
#define	_IN_ADDR_T
typedef	uint32_t	in_addr_t;
#endif

#ifndef	_IPADDR_T
#define	_IPADDR_T
typedef uint32_t ipaddr_t;
#endif

#ifndef _IN6_ADDR_T
#define _IN6_ADDR_T
typedef struct in6_addr in6_addr_t;
#endif

#ifndef _IN_PORT_T
#define _IN_PORT_T
typedef uint16_t in_port_t;
#endif

#ifndef _SOCKLEN_T
#define _SOCKLEN_T
typedef size_t socklen_t;
#endif

extern char *
inet_ntop(int af, const void *addr, char *buf, int addrlen);

extern int
inet_pton(int af, char *inp, void *outp);
#endif
