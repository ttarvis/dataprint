## Architecture

**dataprint** uses a multi-threaded worker pool that waits on a bounded queue containing work items. It uses `simdjson` for parsing the JSON. If you're interested in knowing more of the specifics, contact me.
