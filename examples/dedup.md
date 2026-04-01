## Similarity and deduplication

**dataprint** can be used to find similar files for file deduplication quickly.

### Use `compare` to find similar files and remove duplicates

```bash
dataprint compare file1.jsonl file2.jsonl
```

The closer output is to 1, the more similar. An answer of 100% similarity theoretically means they are the same file. You may wish to remove files above a certain threshold.

