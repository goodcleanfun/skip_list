# skip_list
Skip list (linked lists with express lanes), randomized but with an expected O(log n) for search/insert/delete. Uses memory pool for nodes and O(1) level generation from [Skip Lists Done Right](https://ticki.github.io/blog/skip-lists-done-right/) to reduce random coin flips. Includes a lock-free concurrent version.
