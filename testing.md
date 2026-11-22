# Testing and Benchmark Results

All results below were measured on one machine, with the client and server on the
same host (loopback). Raw outputs are in `benchmarks/`.

**Environment:** Intel i5-12450H, Fedora Linux, GCC 15.2.1, ApacheBench 2.3
**Target:** `GET /` returning a 209-byte `index.html`
**Build:** Release build (`build-rel`) for performance runs; separate Debug builds for sanitizers

> **Scope:** the handler only reads a small file and sends it, so each request is very
> cheap. These numbers describe the server's connection handling on a trivial workload.
> They are not a prediction of real-network or large-file performance.

---

## 1. Worker count sweep

Command: `ab -n 20000 -c 100 http://127.0.0.1:8080/`, queue size 128, 3 runs per setting
(median shown). Failed requests: 0 in every run. Files: `benchmarks/Before/worker*/`.

| Workers | Req/s (median) | p99 latency |
|---|---|---|
| 1 | 38,576 | 3 ms |
| 2 | 37,617 | 3 ms |
| 4 | 36,195 | 3 ms |
| 8 | 34,242 | 5 ms |
| 16 | 31,543 | 6 ms |

**Finding:** throughput fell about 18% from 1 to 16 workers. Each request is so cheap
that extra threads mostly add lock contention and context switching. The best worker
count depends on how expensive each request is; this result applies to small static files.

## 2. Queue size sweep (overload)

Command: `ab -n 20000 -c 200 http://127.0.0.1:8080/`, 2 workers, server restarted for
every run, 3 runs per queue size (median shown). Files: `benchmarks/queue-sweep/`.
Non-2xx responses are the intended `503 Service Unavailable` replies.

| Queue size | Req/s | p99 latency | 503 responses | Share shed |
|---|---|---|---|---|
| 1 | 38,427 | 8 ms | 9,085 | 45.4% |
| 4 | 37,909 | 7 ms | 4,681 | 23.4% |
| 8 | 37,022 | 7 ms | 4,489 | 22.4% |
| 32 | 38,708 | 6 ms | 1,045 | 5.2% |
| 128 | 38,041 | 6 ms | 92 | 0.5% |
| 512 | 38,526 | 6 ms | 0 | 0% |

**Findings:**
- Load shedding falls steadily as the queue grows; most of the drop is between queue 8 and 32.
- Throughput stayed at about 37-39K req/s and p99 at 6-8 ms for every queue size, so
  rejecting connections is cheap and did not slow the server.
- A queue of 200 or more cannot fill with 200 clients and 2 workers, so 512 shedding
  nothing is expected and not a special value.
- Server counters match ApacheBench: accepted + rejected is about 20,000 per run (a
  little over, from ApacheBench's own extra connections), and rejected counts agree
  with the 503 counts to within about 0.5%.
- No latency penalty from larger queues was visible, because requests are too short
  for connections to wait long. This trade-off would need slower requests to show.

## 3. Overload stability (workers 2, queue 8, 200 clients)

Four runs of 20,000 requests each (`benchmarks/overload/`):

| Run | 503 responses | Share shed | Req/s | p99 |
|---|---|---|---|---|
| 1 | 3,909 | 19.5% | 38,800 | 6 ms |
| 2 | 3,460 | 17.3% | 36,417 | 8 ms |
| 3 | 4,721 | 23.6% | 36,471 | 8 ms |
| 4 | 4,845 | 24.2% | 36,448 | 11 ms |

Connect, receive and exception errors were 0 in all runs; the server stayed up and
shut down cleanly on SIGINT.

## 4. Sanitizer runs

| Tool | Build flags | Workload | Result |
|---|---|---|---|
| AddressSanitizer + UBSan | `-fsanitize=address,undefined` | ApacheBench load | No reports (`benchmarks/asan.txt` empty) |
| ThreadSanitizer | `-fsanitize=thread` | ApacheBench load | No reports (`benchmarks/tsan.txt` empty) |
| ThreadSanitizer | `-fsanitize=thread` | SIGINT sent during load | No reports (`benchmarks/tsan_shutdown.txt` empty) |

Sanitizer reports are written to stderr, which was redirected into these files.

## 5. Functional tests

`tests/test_server.sh` checks: 200 and Content-Type, 404, 405, HEAD without body, path
traversal rejection, 400 for malformed requests, 431 for oversized headers.

```bash
./build-rel/server --port 8080 &
bash tests/test_server.sh 8080
```

## 6. Known limitations

- Localhost only, one small file, `Connection: close` on every response (no keep-alive).
- ApacheBench is single-threaded and may limit the measured peak.
- Not tested: large files, slow or idle clients holding workers, long soak runs,
  file descriptor leak checks.
- Symlinks inside `public/` are followed.