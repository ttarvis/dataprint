
## Performance

### Design Decisions for Performance

**dataprint** uses a chunking pattern on files to create work items in the multi threaded mode. **dataprint** tries to allocate a minimal amount of objects when walking JSON paths. The fastest algorithms and data structures were chosen for each processing segment.

- **Concurrency model:** Worker pool with bounded work item queue to deal with backpressure on large files.
- **Memory management:** Memory use is primarily in chunking which has a capped size and is deallocated after use in RAII pattern.
- **I/O strategy:** Files are memory mapped and chunked.
- **Hotpath optimization:** simdjson parsing and fast hash functions are used to walk paths

### Scaling Characteristics

From benchmarks, speedup over single threaded scales linearly and multithreaded usage efficiency is above 50%.
