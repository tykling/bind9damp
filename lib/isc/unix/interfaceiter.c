/*
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: interfaceiter.c,v 1.22.2.1.10.9 2003/10/07 04:27:43 marka Exp $ */

#include <config.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>		/* Required for ifiter_ioctl.c. */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <isc/interfaceiter.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/msgs.h>
#include <isc/net.h>
#include <isc/result.h>
#include <isc/strerror.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

/* Must follow <isc/net.h>. */
#ifdef HAVE_NET_IF6_H
#include <net/if6.h>
#endif
#include <net/if.h>

/* Common utility functions */

/*
 * Extract the network address part from a "struct sockaddr".
 *
 * The address family is given explicitly
 * instead of using src->sa_family, because the latter does not work
 * for copying a network mask obtained by SIOCGIFNETMASK (it does
 * not have a valid address family).
 */

static void
get_addr(unsigned int family, isc_netaddr_t *dst, struct sockaddr *src,
	 char *ifname)
{
	struct sockaddr_in6 *sa6;

	/* clear any remaining value for safety */
	memset(dst, 0, sizeof(*dst));

	dst->family = family;
	switch (family) {
	case AF_INET:
		memcpy(&dst->type.in,
		       &((struct sockaddr_in *) src)->sin_addr,
		       sizeof(struct in_addr));
		break;
	case AF_INET6:
		sa6 = (struct sockaddr_in6 *)src;
		memcpy(&dst->type.in6, &sa6->sin6_addr,
		       sizeof(struct in6_addr));
#ifdef ISC_PLATFORM_HAVESCOPEID
		if (sa6->sin6_scope_id != 0)
			isc_netaddr_setzone(dst, sa6->sin6_scope_id);
		else {
			/*
			 * BSD variants embed scope zone IDs in the 128bit
			 * address as a kernel internal form.  Unfortunately,
			 * the embedded IDs are not hidden from applications
			 * when getting access to them by sysctl or ioctl.
			 * We convert the internal format to the pure address
			 * part and the zone ID part.
			 * Since multicast addresses should not appear here
			 * and they cannot be distinguished from netmasks,
			 * we only consider unicast link-local addresses.
			 */
			if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr)) {
				isc_uint16_t zone16;

				memcpy(&zone16, &sa6->sin6_addr.s6_addr[2],
				       sizeof(zone16));
				zone16 = ntohs(zone16);
				if (zone16 != 0) {
					/* the zone ID is embedded */
					isc_netaddr_setzone(dst,
							    (isc_uint32_t)zone16);
					dst->type.in6.s6_addr[2] = 0;
					dst->type.in6.s6_addr[3] = 0;
				} else if (ifname != NULL) {
					unsigned int zone;

					/*
					 * sin6_scope_id is still not provided,
					 * but the corresponding interface name
					 * is know.  Use the interface ID as
					 * the link ID.
					 */
					zone = if_nametoindex(ifname);
					if (zone != 0) {
						isc_netaddr_setzone(dst,
								    (isc_uint32_t)zone);
					}
				}
			}
		}
#endif
		break;
	default:
		INSIST(0);
		break;
	}
}

/*
 * Include system-dependent code.
 */

#if HAVE_GETIFADDRS
#include "ifiter_getifaddrs.c"
#elif HAVE_IFLIST_SYSCTL
#include "ifiter_sysctl.c"
#else
#include "ifiter_ioctl.c"
#endif

/*
 * The remaining code is common to the sysctl and ioctl case.
 */

isc_result_t
isc_interfaceiter_current(isc_interfaceiter_t *iter,
			  isc_interface_t *ifdata)
{
	REQUIRE(iter->result == ISC_R_SUCCESS);
	memcpy(ifdata, &iter->current, sizeof(*ifdata));
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_interfaceiter_first(isc_interfaceiter_t *iter) {
	isc_result_t result;

	REQUIRE(VALID_IFITER(iter));

	internal_first(iter);
	for (;;) {
		result = internal_current(iter);
		if (result != ISC_R_IGNORE)
			break;
		result = internal_next(iter);
		if (result != ISC_R_SUCCESS)
			break;
	}
	iter->result = result;
	return (result);
}

isc_result_t
isc_interfaceiter_next(isc_interfaceiter_t *iter) {
	isc_result_t result;

	REQUIRE(VALID_IFITER(iter));
	REQUIRE(iter->result == ISC_R_SUCCESS);

	for (;;) {
		result = internal_next(iter);
		if (result != ISC_R_SUCCESS)
			break;
		result = internal_current(iter);
		if (result != ISC_R_IGNORE)
			break;
	}
	iter->result = result;
	return (result);
}

void
isc_interfaceiter_destroy(isc_interfaceiter_t **iterp)
{
	isc_interfaceiter_t *iter;
	REQUIRE(iterp != NULL);
	iter = *iterp;
	REQUIRE(VALID_IFITER(iter));

	internal_destroy(iter);
	if (iter->buf != NULL)
		isc_mem_put(iter->mctx, iter->buf, iter->bufsize);

	iter->magic = 0;
	isc_mem_put(iter->mctx, iter, sizeof(*iter));
	*iterp = NULL;
}
