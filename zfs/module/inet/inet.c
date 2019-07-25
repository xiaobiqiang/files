#include <sys/in.h>
#include <uapi/linux/in6.h>
#include <uapi/linux/in.h>
#include <linux/inet.h>

static void
convert2ascii(char *buf, const struct in6_addr *addr)
{
	int		hexdigits;
	int		head_zero = 0;
	int		tail_zero = 0;
	/* tempbuf must be big enough to hold ffff:\0 */
	char		tempbuf[6];
	char		*ptr;
	uint16_t	*addr_component;
	size_t		len;
	boolean_t	first = B_FALSE;
	boolean_t	med_zero = B_FALSE;
	boolean_t	end_zero = B_FALSE;

	addr_component = (uint16_t *)addr;
	ptr = buf;

	/* First count if trailing zeroes higher in number */
	for (hexdigits = 0; hexdigits < 8; hexdigits++) {
		if (*addr_component == 0) {
			if (hexdigits < 4)
				head_zero++;
			else
				tail_zero++;
		}
		addr_component++;
	}
	addr_component = (uint16_t *)addr;
	if (tail_zero > head_zero && (head_zero + tail_zero) != 7)
		end_zero = B_TRUE;

	for (hexdigits = 0; hexdigits < 8; hexdigits++) {

		/* if entry is a 0 */

		if (*addr_component == 0) {
			if (!first && *(addr_component + 1) == 0) {
				if (end_zero && (hexdigits < 4)) {
					*ptr++ = '0';
					*ptr++ = ':';
				} else {
					/*
					 * address starts with 0s ..
					 * stick in leading ':' of pair
					 */
					if (hexdigits == 0)
						*ptr++ = ':';
					/* add another */
					*ptr++ = ':';
					first = B_TRUE;
					med_zero = B_TRUE;
				}
			} else if (first && med_zero) {
				if (hexdigits == 7)
					*ptr++ = ':';
				addr_component++;
				continue;
			} else {
				*ptr++ = '0';
				*ptr++ = ':';
			}
			addr_component++;
			continue;
		}
		if (med_zero)
			med_zero = B_FALSE;

		tempbuf[0] = '\0';
		(void) sprintf(tempbuf, "%x:", ntohs(*addr_component) & 0xffff);
		len = strlen(tempbuf);
		bcopy(tempbuf, ptr, len);
		ptr = ptr + len;
		addr_component++;
	}
	*--ptr = '\0';
}


char *
inet_ntop(int af, const void *addr, char *buf, int addrlen)
{
	static char local_buf[INET6_ADDRSTRLEN];
	static char *err_buf1 = "<badaddr>";
	static char *err_buf2 = "<badfamily>";
	struct in6_addr	*v6addr;
	uchar_t		*v4addr;
	char		*caddr;

	/*
	 * We don't allow thread unsafe inet_ntop calls, they
	 * must pass a non-null buffer pointer. For DEBUG mode
	 * we use the ASSERT() and for non-debug kernel it will
	 * silently allow it for now. Someday we should remove
	 * the static buffer from this function.
	 */

	ASSERT(buf != NULL);
	if (buf == NULL)
		buf = local_buf;
	buf[0] = '\0';

	/* Let user know politely not to send NULL or unaligned addr */
	if (addr == NULL) {
#ifdef DEBUG
		cmn_err(CE_WARN, "inet_ntop: addr is <null> or unaligned");
#endif
		return (err_buf1);
	}


#define	UC(b)	(((int)b) & 0xff)
	switch (af) {
	case AF_INET:
		ASSERT(addrlen >= INET_ADDRSTRLEN);
		v4addr = (uchar_t *)addr;
		(void) sprintf(buf, "%03d.%03d.%03d.%03d",
		    UC(v4addr[0]), UC(v4addr[1]), UC(v4addr[2]), UC(v4addr[3]));
		return (buf);

	case AF_INET6:
		ASSERT(addrlen >= INET6_ADDRSTRLEN);
		v6addr = (struct in6_addr *)addr;
		if (IN6_IS_ADDR_V4MAPPED(v6addr)) {
			caddr = (char *)addr;
			(void) sprintf(buf, "::ffff:%d.%d.%d.%d",
			    UC(caddr[12]), UC(caddr[13]),
			    UC(caddr[14]), UC(caddr[15]));
		} else if (IN6_IS_ADDR_V4COMPAT(v6addr)) {
			caddr = (char *)addr;
			(void) sprintf(buf, "::%d.%d.%d.%d",
			    UC(caddr[12]), UC(caddr[13]), UC(caddr[14]),
			    UC(caddr[15]));
		} else if (IN6_IS_ADDR_UNSPECIFIED(v6addr)) {
			(void) sprintf(buf, "::");
		} else {
			convert2ascii(buf, v6addr);
		}
		return (buf);

	default:
		return (err_buf2);
	}
#undef UC
}