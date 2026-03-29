// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include "processor.h"

#include "blake3.h"
#include "boundedqueue.h"
#include "debug.h"
#include "maxheap.h"
#include "policies.h"
#include "simdjson.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#define BLAKE3_OUT_LEN 32
#define CHUNK_SIZE 1ULL * 1024 * 1024 * 1024

// Define the macro to combine two 64-bit hex values
#define MAKE_UINT128(HI, LO) ((((__uint128_t)(HI)) << 64) | (LO))

#define PRIME MAKE_UINT128(0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL)

// combines two 64 bit unsigned ints into a 128 bit unsigned int
#define COMBINE_U128(high, low)                                                \
	(((__uint128_t)(high) << 64) | ((__uint128_t)(low)))

using namespace std;

// used to hash for an array index
struct ArrayIndexPrefix {
	size_t open;
	size_t index;
	size_t close;
};

Processor::Processor(const ProgramOptions& prog_options, std::ostream& out)
    : stream_(out) {
	options = prog_options;
	num_workers_ = std::thread::hardware_concurrency() - 1;
	M = 0;
}

__uint128_t Processor::getM() const { return M; };

/*
 * overload << operator to print 128 bit numbers
 * used for printing fingerprints
 */
std::ostream& operator<<(std::ostream& os, __uint128_t value) {
        if (value == 0)
                return os << '0';
        char buf[33];  // max 32 hex digits + null
        char* p = buf + sizeof(buf);
        *--p = '\0';
        while (value > 0) {
                int nibble = value & 0xF;
                *--p = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
                value >>= 4;
        }
        return os << p;
}

/*
 * alternate operator
 */
/*
std::ostream& operator<<(std::ostream& os, __uint128_t value) {
	if (value == 0) {
		return os << '0';
	}

	char buffer[40];  // __uint128_t max is 39 digits
	char* ptr = buffer + sizeof(buffer);
	*--ptr = '\0';

	while (value > 0) {
		*--ptr = '0' + (value % 10);
		value /= 10;
	}

	return os << ptr;
}
*/

/*
 * run selects major subcommand and runs the corresponding
 * function i.e. fingerprint, diff, etc
 */
void Processor::run() {
	if (options.cmd == "hash") {
		hash();
	}
	if (options.cmd == "fingerprint") {
		if(options.single) {
			fingerprint_s();
		} else {
			fingerprint();
		}
	}
	if (options.cmd == "compare") {
		compare();
	}
}

/*
 * mixHash is a way to mix a 128 bit has from XXHash with
 * a 64 bit integer
 */
XXH128_hash_t mixHash(uint64_t hi, uint64_t lo, uint64_t b) {
	const uint64_t M1 = 0xff51afd7ed558ccdULL;
	const uint64_t M2 = 0xc4ceb9fe1a85ec53ULL;

	hi ^= b * M1;
	hi ^= hi >> 33;
	hi *= M1;
	hi ^= hi >> 32;
	lo ^= b * M2;
	lo ^= lo >> 33;
	lo *= M2;
	lo ^= lo >> 32;

	return XXH128_hash_t{hi ^ lo, hi * M2 ^ lo * M1};
}

/*
 * thread safe way to add to M
 */
void Processor::add_to_m(__uint128_t sum) {
	std::lock_guard<std::mutex> lock(mutex_);
	M = add_mod(M, sum);
}

/*
 * printBytesAsHex is used for printing out the final hash in hex
 * it also prints the file name after it for record keeping sake
 */
void printBytesAsHex(const string file, const unsigned char* data,
                     size_t size) {
	// Set the stream manipulators once outside the loop for efficiency
	std::cout << std::hex << std::setfill('0');

	for (size_t i = 0; i < size; ++i) {
		// Cast each byte to an unsigned int to ensure it's treated as a
		// number and not a character, avoiding potential issues with
		// signed chars
		std::cout << std::setw(2) << static_cast<unsigned int>(data[i]);
	}

	std::cout << "  " << file << std::endl;
	// TODO: Reset the stream to decimal output if needed for future
	// i.e. std::cout << std::dec;
}

void Processor::printFingerprint(const string file) {
	stream_ << "fingerprint: " << M << "  " << file << std::endl;
}

/*
 * hashes the basic elements of JSON
 * it also canonicalizes the JSON in a way that
 * the hash of an equivalent value is equivalent
 */
XXH128_hash_t XXHash(uint64_t path, simdjson::ondemand::value& value) {
	XXH128_hash_t result;
	XXH128_hash_t h;

	switch (value.type()) {
	case simdjson::ondemand::json_type::string: {
		std::string_view str = value.get_string();
		h = XXH3_128bits(str.data(), str.size());
		break;
	}
	case simdjson::ondemand::json_type::number: {
		simdjson::ondemand::number num;
		auto error = value.get_number().get(num);
		if (error == simdjson::BIGINT_ERROR) {
			// XXX
			// in this case we are hashing an approximation to 
			// a big integer. So that means collissions can occur
			// but big integers are rare and collissions with
			// big integers may also be rare
			// but when a raw_json_token is available in this API
			// we can use it
			double big_approx = num.get_double();
			h = XXH3_128bits(&big_approx, sizeof(big_approx));
			break;
		}
		if (error) {
			throw std::runtime_error(
				std::string("number parse error: ") +
				simdjson::error_message(error)
			);
		}
		switch (num.get_number_type()) {
		case simdjson::ondemand::number_type::signed_integer: {
			__int128_t wide = num.get_int64();
			h = XXH3_128bits(&wide, sizeof(wide));
			break;
		}
		case simdjson::ondemand::number_type::unsigned_integer: {
			__uint128_t wide = num.get_uint64();
			h = XXH3_128bits(&wide, sizeof(wide));
			break;
		}
		case simdjson::ondemand::number_type::floating_point_number: {
			double d = num.get_double();
			// this is checking if the double is a whole number
			if (double intpart; std::modf(d, &intpart) == 0.0 &&
			                    d >= (double)INT64_MIN &&
			                    d <= (double)INT64_MAX) {
				__int128_t wide = static_cast<int64_t>(d);
				h = XXH3_128bits(&wide, sizeof(wide));
			} else {
				h = XXH3_128bits(&d, sizeof(d));
			}
			break;
		}
		case simdjson::ondemand::number_type::big_integer: {
			// just a dummy case to silence compiler
			break;
		}
		}
		break;
	}
	case simdjson::ondemand::json_type::boolean: {
		bool b = value.get_bool();
		h = XXH3_128bits(&b, sizeof(b));
		break;
	}
	default: {
		const char null_str[2] = {'\0', '\0'};
		h = XXH3_128bits(null_str, 2);
		break;
	}
	}
	result = mixHash(h.high64, h.low64, path);
	return result;
}

/*
 * hashBasicValues walks and hashes paths in JSON documents
 */
template <typename Policy>
void Processor::hashBasicValues(simdjson::ondemand::value element,
                                typename Policy::Accumulator& local_acc, uint64_t path_hash) {
	switch (element.type()) {
	case simdjson::ondemand::json_type::object: {
		simdjson::ondemand::object obj = element.get_object();
		if (obj.is_empty()) {
			// when there is a {}
			XXH128_hash_t h = mixHash(1, 1, 0);
			__uint128_t u128 = COMBINE_U128(h.high64, h.low64);
			Policy::accumulate(local_acc, u128);
		}
		for (auto field : obj) {
			std::string_view key = field.unescaped_key(false).value();
			XXH64_hash_t key_hash = XXH3_64bits(key.data(), key.size());
			uint64_t new_hash = mix(path_hash, key_hash);
			hashBasicValues<Policy>(field.value(), local_acc, new_hash);
		}
		break;
	}
	case simdjson::ondemand::json_type::array: {
		size_t index = 0;
		simdjson::ondemand::array arr = element.get_array();
		if (arr.is_empty()) {
			// when there is a []
			XXH128_hash_t h = mixHash(1, 1, 1);
			__uint128_t u128 = COMBINE_U128(h.high64, h.low64);
			Policy::accumulate(local_acc, u128);
		}
		for (auto child : arr) {
			ArrayIndexPrefix p{};
			p.open = '[';
			p.index = index++;
			p.close = ']';
			XXH64_hash_t i_hash = XXH3_64bits(&p, sizeof(p));
			uint64_t new_hash = mix(path_hash, i_hash);
			hashBasicValues<Policy>(child.value(), local_acc, new_hash);
		}
		break;
	}
	default: {
		XXH128_hash_t xxhash = XXHash(path_hash, element);
		__uint128_t h = COMBINE_U128(xxhash.high64, xxhash.low64);
		Policy::accumulate(local_acc, h);
		break;
	}
	}
}

/*
 * just takes a hash of an input
 */
void Processor::hash() {
	for (const auto& file : options.input_files) {
		int fd = open(file.c_str(), O_RDONLY);
		if (fd == -1) {
			close(fd);
			throw std::runtime_error("Failed to open file" + file);
		}
		struct stat st;
		fstat(fd, &st);

		// 1. Map the file
		uint8_t* data = (uint8_t*)mmap(NULL, st.st_size, PROT_READ,
		                               MAP_PRIVATE, fd, 0);

		if (data == MAP_FAILED) {
			close(fd);
			throw std::runtime_error("mmap failed");
		}
        
		// 2. Initialize and Update
		blake3_hasher hasher;
		blake3_hasher_init(&hasher);

		blake3_hasher_update(&hasher, data, st.st_size);

		// 3. Finalize
		uint8_t output[BLAKE3_OUT_LEN];
		blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);

		// 4. print
		printBytesAsHex(file, output, BLAKE3_OUT_LEN);

		// Cleanup
		munmap(data, st.st_size);
		close(fd);
	}
}

/*
 * multiThreaded fingerprinting
 */
void Processor::chunk(const std::string& file, BoundedQueue& queue,
                      size_t chunk_size) {
	// 1. open the file to get a file descriptor
	int fd = open(file.c_str(), O_RDONLY);
	if (fd == -1) {
		close(fd);
		throw std::runtime_error("Failed to open file" + file);
	}

	// 2. get the file size
	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		close(fd);
		throw std::runtime_error("Failed to get file size");
	}
	size_t file_size = sb.st_size;

	if (file_size == 0) {
		close(fd);
		std::cerr << "Warning: skipping empty file: " << file
		          << std::endl;
		return;
	}

	size_t offset = 0;

	// no sliding window implemented just memory map the whole thing
	// it may not make a difference for most cases
	// will consider implementing it
	const char* map = static_cast<const char*>(
	        mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, offset));
	if (map == MAP_FAILED) {
		close(fd);
		throw std::runtime_error("mmap failed");
	}

	// performance hints to OS
	madvise((void*)map, file_size, MADV_SEQUENTIAL);  // Sequential access
	madvise((void*)map, file_size, MADV_WILLNEED);    // Start readahead now

	const char* pos = map;
	const char* file_end = map + file_size;

	while (pos < file_end) {
		const char* chunk_end = pos + chunk_size;

		if (chunk_end >= file_end) {
			// final chunk, take everything
			chunk_end = file_end;
		} else {
			// advance to next new line to avoid a split record
			// returns pointer to matching newline byte if it exists
			const char* newline = static_cast<const char*>(
			        memchr(chunk_end, '\n', file_end - chunk_end));
			chunk_end = newline ? newline + 1 : file_end;
		}

		WorkItem item;
		item.chunk = simdjson::padded_string(pos, chunk_end - pos);
		queue.push(std::move(item));

		pos = chunk_end;
	}

	munmap(const_cast<char*>(map), file_size);
}

/*
 * single threaded fingerprinting
 */
void Processor::fingerprint_s() {
	simdjson::ondemand::parser parser; // local, not a member
	for (const auto& file : options.input_files) {
		M = 0;
		simdjson::padded_string json =
		        simdjson::padded_string::load(file);
		simdjson::ondemand::document_stream docs;
		auto error = parser.iterate_many(json).get(docs);
		if(error) {
			throw std::runtime_error(
				std::string("JSON parse error in file: ") +
				simdjson::error_message(error)
			);
		}

		FingerprintPolicy::Accumulator local_acc = FingerprintPolicy::init();

		for (auto doc : docs) {
			// compute hash here
			hashBasicValues<FingerprintPolicy>(doc.value(), local_acc);
		}

		add_to_m(local_acc);
		printFingerprint(file);
	}
}

/*
 * fingerprint starts the process of fingerprinting
 * in multi threaded mode
 */
void Processor::fingerprint() {
	auto fn = [this](simdjson::ondemand::value element,
	                     FingerprintPolicy::Accumulator& local_acc, uint64_t path) {
		hashBasicValues<FingerprintPolicy>(element, local_acc, path);
	};

	for (const auto& file : options.input_files) {
		M = 0;
		auto result = run_pipeline<FingerprintPolicy>(file, fn);
		add_to_m(result);
		printFingerprint(file);
	}
}

/*
 * sketch starts up the pipeline and returns the 
 * reduced max heap of K (or less) elements for
 * use in the Jaccard Index approximator
 */
MinKAccumulator Processor::sketch(const std::string& file) {
    auto fn = [this](simdjson::ondemand::value element,
                     MinKPolicy::Accumulator& local_acc, uint64_t path) {
        hashBasicValues<MinKPolicy>(element, local_acc, path);
    };

    return run_pipeline<MinKPolicy>(file, fn);
}

/*
 * mapify converts a max heap to a map
 */
std::unordered_map<__uint128_t, bool, U128Hash> mapify(MaxHeap& h) {
	std::unordered_map<__uint128_t, bool, U128Hash> m;
	for (const auto& element : h.data()) {
		m[element] = true;
	}
	return m;
}

/*
 * jaccard computes an approximation to the Jaccard Index
 * u is the number of shared numbers
 */
float jaccard(float k, std::unordered_map<__uint128_t, bool, U128Hash> m1,
              std::unordered_map<__uint128_t, bool, U128Hash> m2) {
	int u = 0;
	if (m1.size() <= m2.size()) {
		for (const auto& [key, value] : m1) {
			if (m2.count(key) > 0) {
				u++;
			}
		}
	} else {
		for (const auto& [key, value] : m2) {
			if (m1.count(key) > 0) {
				u++;
			}
		}
	}
	// this is an exact answer
	// u is size of intersection
	// k is sum of the sizes of both m1 m1
	// k - u subtracts any double counting
	if(k != float(K)) {
		return u / (k-u);
	}

	return (float)u / k;
}

/*
 * compare will compare two files
 * it approximates Jaccard Similarity
 * https://en.wikipedia.org/wiki/Jaccard_index
 */
void Processor::compare() {
	MinKAccumulator h1 = sketch(options.input_files[0]);
	MinKAccumulator h2 = sketch(options.input_files[1]);

	// convert to maps here
	std::unordered_map<__uint128_t, bool, U128Hash> m1 = mapify(h1.heap);
	std::unordered_map<__uint128_t, bool, U128Hash> m2 = mapify(h2.heap);

	float k = float(K);
	// TODO: we might want a cleaner way to do all this so it doesn't look
	// insane
	if (h1.heap.size() < K || h2.heap.size() < K) {
		k = h1.heap.size() + h2.heap.size();
	}
	float index = jaccard(k, m1, m2);

	std::cout << "Similarity:  " << 100 * index << "%" << std::endl;
}
