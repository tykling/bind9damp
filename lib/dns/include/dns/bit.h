/*
 * Copyright (C) 2000, 2001  Internet Software Consortium.
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

/* $Id: bit.h,v 1.6.4.1 2001/01/09 22:45:03 bwelling Exp $ */

#ifndef DNS_BIT_H
#define DNS_BIT_H 1

#include <isc/int.h>
#include <isc/boolean.h>

typedef isc_uint64_t dns_bitset_t;

#define DNS_BIT_SET(bit, bitset) \
     (*(bitset) |= ((dns_bitset_t)1 << (bit)))
#define DNS_BIT_CLEAR(bit, bitset) \
     (*(bitset) &= ~((dns_bitset_t)1 << (bit)))
#define DNS_BIT_CHECK(bit, bitset) \
     ISC_TF((*(bitset) & ((dns_bitset_t)1 << (bit))) \
	    == ((dns_bitset_t)1 << (bit)))

#endif /* DNS_BIT_H */

