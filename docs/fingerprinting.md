## Fingerprinting


### purpose

File fingerprinting is a way of identifying a file using it's structure. It might be just a hash function of the binary data but that can produce a completely different fingerprint for two different files where maybe just the white space was changed. So fingerprinting is more about trying to identify a content by it's meaning, in some sense.

Fingerprints can be used to say two files are different or to show that a file has changed meaningfully.

### dataprint fingerprints

**dataprint** attempts to fingerprint JSONL files in a way that respects canonicalization of data but also is order independent. So two files should have the same fingerprint if the ordering of lines is changed but the content otherwise remains the same.
