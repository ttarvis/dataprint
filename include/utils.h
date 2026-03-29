// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#ifndef UTILS_H
#define UTILS_H

#define MAKE_UINT128(HI, LO) ((((__uint128_t)(HI)) << 64) | (LO))
#define PRIME MAKE_UINT128(0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL)

/*
 * hash function for a __uint128_t
 */
struct U128Hash {
	size_t operator()(__uint128_t val) const {
		uint64_t lo = (uint64_t)val;
		uint64_t hi = (uint64_t)(val >> 64);
		return std::hash<uint64_t>{}(lo) ^
		       (std::hash<uint64_t>{}(hi) << 1);
	}
};

/*
 * optimization for faster modular reduction
 * the umodint instruction was taking
 * a huge amount of time
 */
inline __uint128_t reduce_mod_p(__uint128_t x) {
	// works because PRIME = 2^127 - 1
	// x mod (2^127 - 1) = (x >> 127) + (x & PRIME)
	__uint128_t r = (x >> 127) + (x & PRIME);
	if (r >= PRIME)
		r -= PRIME;
	return r;
}

/*
 * add_mod takes two 128 bit integers and adds them
 * mod a large prime P
 * XXX: the prime is not an argument but could be
 * What is best?
 */
inline __uint128_t add_mod(__uint128_t a, __uint128_t b) {
	a = reduce_mod_p(a);
	b = reduce_mod_p(b);
	__uint128_t r = a + b;
	if (r >= PRIME)
		r -= PRIME;
	return r;
}

/*
 * mixer function used to mix in a key with it's previous path hash
 */
inline uint64_t mix(uint64_t h, uint64_t k) {
	h ^= k + 0x9e3779b97f4a7c15 + (h << 6) + (h >> 2);
	return h;
}

#endif
