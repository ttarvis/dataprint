## Limitations

- Not compatible with MSVC. (contact me if this interests you)
- 32 bit address space may be insufficient for large file addressing
- **dataprint** will not work well with singular large JSON documents, for example, a GB+ JSON document. It is best to use a single thread in such scenarios because there is no natural, parallel way to parse a single JSON document. It is recommended to break large JSON documents into smaller documents such as in the JSONL format and recompute.
- integers that exceed 64 bits, big integers, are approximated. This may cause collisions if two big integers are approximated with the same value
- Malformed JSON will probably cause errors depending on the issue. For example, actual newline bytes in the JSON is incorrect JSON and it would not be parsed correctly.
- Pathological inputs will probably cause errors. For example, deeply nested (10000+ layers of nesting) JSON.
- Different Unicode normalizations will cause different fingerprints
- Some edge case floating point values may be represented differently on different systems and cause different fingerprints even though the number is the same. This should be treated as a bug but this generally shouldn't be an issue.
