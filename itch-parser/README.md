# High-Performance Nasdaq ITCH 5.0 Parser

An ultra-low latency, zero-copy Nasdaq TotalView-ITCH 5.0 parser built in C++20. Designed for high-frequency trading (HFT) and quantitative research pipelines where processing millions of events per second with sub-10 nanosecond latency is critical.

## Problem Statement

Exchanges like Nasdaq distribute order flow events (adds, executions, cancels, deletes, trades) as a fast, high-volume binary stream. A naive approach to parsing this stream introduces significant latency overhead due to:
1. **Dynamic Allocations:** Copying message buffers onto the heap (e.g., using `std::vector` or `std::string` for tickers).
2. **I/O Bottlenecks:** Blocking stream reads (`std::ifstream`) rather than leveraging kernel-level paging mechanisms.
3. **Dispatch Overhead:** Dynamic mapping/lookup structures like `std::unordered_map` with `std::function` wrappers for message dispatching.
4. **Data Copying:** Unpacking fields individually rather than overlaying structures directly on memory views.

This project implements and compares a correctness-first **Naive Baseline** against an **Optimized mmap-based Parser** using compiler-level optimizations (`-O3 -march=native -flto`), zero-copy packed structures, prefetching, and jump-table branchless dispatching.

---

## ITCH 5.0 Format Summary

Nasdaq ITCH 5.0 messages are **length-prefixed** (2-byte big-endian integer) and start with a **1-byte message type** ASCII character. All multi-byte numeric fields are encoded in big-endian byte order. 

This parser supports the following 6 core message types (which cover >95% of total market volume):

| Message Type | Character | Payload Size (Bytes) | Fields (Offset & Length) |
|---|---|---|---|
| **System Event** | `'S'` | 12 | Type(0:1), Stock Locate(1:2), Tracking(3:2), Timestamp(5:6), Event Code(11:1) |
| **Add Order** (No MPID) | `'A'` | 36 | Type(0:1), Stock Locate(1:2), Tracking(3:2), Timestamp(5:6), Order Ref(11:8), Side(19:1), Shares(20:4), Ticker(24:8), Price(32:4) |
| **Order Executed** | `'E'` | 31 | Type(0:1), Stock Locate(1:2), Tracking(3:2), Timestamp(5:6), Order Ref(11:8), Executed Shares(19:4), Match No(23:8) |
| **Order Cancel** | `'X'` | 23 | Type(0:1), Stock Locate(1:2), Tracking(3:2), Timestamp(5:6), Order Ref(11:8), Cancelled Shares(19:4) |
| **Order Delete** | `'D'` | 19 | Type(0:1), Stock Locate(1:2), Tracking(3:2), Timestamp(5:6), Order Ref(11:8) |
| **Trade (Non-Cross)** | `'P'` | 44 | Type(0:1), Stock Locate(1:2), Tracking(3:2), Timestamp(5:6), Order Ref(11:8), Side(19:1), Shares(20:4), Ticker(24:8), Price(32:4), Match No(36:8) |

---

## Architecture Diagram

```
+-------------------------------------------------------+
|             Nasdaq ITCH 5.0 Binary Data               |
+-------------------------------------------------------+
                           |
                           v (Zero-Copy OS Memory-Map via VirtualAlloc / mmap)
+-------------------------------------------------------+
|              Zero-Copy mmap Buffer (Memory)           |
+-------------------------------------------------------+
                           |
         +-----------------+-----------------+
         | (Jump Table / TableParser)        | (Switch Dispatch)
         v                                   v
+--------------------+              +--------------------+
|  Table Dispatch    |              |  Switch Dispatch   |
|  m_table[msg[0]]   |              |   switch(msg[0])   |
+--------------------+              +--------------------+
         |                                   |
         +-----------------+-----------------+
                           | (Direct Pointer Overlay)
                           v
+-------------------------------------------------------+
|              Zero-Copy Packed Structs                 |
|       (AddOrderMsg, OrderExecutedMsg, etc.)           |
+-------------------------------------------------------+
                           |
                           v
+-------------------------------------------------------+
|                    Downstream                         |
|      (StatsHandler / High-Performance Order Book)     |
+-------------------------------------------------------+
```

---

## Performance Benchmarks

### Environment
* **CPU:** Intel Core (13th/14th Gen Raptor Lake Family 6 Model 183)
* **Compiler:** Clang 22.1.7 (LLVM MinGW UCRT x86_64)
* **Optimization Flags:** `-O3 -march=native -flto`
* **Test Dataset:** 25MB NASDAQ ITCH 5.0 raw sample binary (2,243,750 messages)

### Micro-Benchmark Results (2 Warmup Runs + 10 Timed Runs)

| Parser Implementation | Median Parsing Time (s) | Throughput (Million msgs/sec) | Avg Latency (ns/msg) | Speedup vs Naive |
| :--- | :---: | :---: | :---: | :---: |
| **Naive Baseline** | 0.47913 s | 4.68 M/s | 213.54 ns | *Baseline* |
| **Optimized Switch** | 0.02174 s | 103.21 M/s | 9.69 ns | **22.0x** |
| **Optimized Jump Table** | 0.02071 s | **108.33 M/s** | **9.23 ns** | **23.1x** |
| **Order Book Building** | 0.34855 s | 6.44 M/s | 155.35 ns | **1.37x** (with book states) |

*Note: In-memory parsing latency drops to **~9.2 ns/message** once the memory-mapped file pages are warmed up in cache, bypassing cold disk I/O faults.*

---

## Cycle-Level Profiling (Perf Counter Analysis)

On a Linux/Unix system, you can capture cycle-level performance counters using `perf`:

```bash
# Pin to a specific CPU core and measure hardware performance counters
taskset -c 2 perf stat -e cycles,instructions,branches,branch-misses,cache-misses ./build/bench_parser data/sample.itch
```

### Analysis of Optimizations:
1. **Branch Predictor & Jump Table:** The jump-table parser (`TableParser`) eliminates conditional branches by directly index-jumping through a 256-entry array of function pointers. This prevents branch mispredictions, which cost 15-20 cycles on modern deep pipelined architectures.
2. **Instruction-Level Parallelism (ILP):** Because the message layout is aligned and the parser compiles with `-O3 -flto`, Clang merges instructions, pipelines loads, and uses vector registers to perform fast block loads.
3. **Data Prefetching:** Using `__builtin_prefetch` on the next message's address (`data + off + 2 + msg_len`) loads data from RAM into L1/L2 caches ahead of time, preventing stalls on memory boundary crossings.

---

## What Actually Moved the Needle (Lessons Learned)

1. **Memory Mapping (`mmap`/`MapViewOfFile`) is King:** Streaming with `std::ifstream` limits throughput by copying pages into user-space buffers sequentially. Memory-mapping maps the file descriptors straight to the process space, delegating page faults to the OS kernel and letting the CPU's memory management unit (MMU) optimize reads.
2. **Zero-Copy Packed Struct Casts:** By declaring structs with `#pragma pack(push, 1)`, we can overlay pointers directly over the memory mapped region. Instead of unpacking fields (which requires shift-and-assign operations), we read them in-place, incurring zero structure copy overhead.
3. **Template-Based Inlining:** Wrapping message callbacks in templates rather than using virtual dispatch (polymorphism) or `std::function` allows the compiler to fully inline the callback logic directly into the parser's hot loop. This turns callback invocations into simple local register operations.
4. **Skipping Unnecessary Fields:** Byte-swapping (`be32`/`be64`) is expensive. The optimized parser only swaps fields that are actually requested (e.g., `shares`, `price`) on-demand, keeping cold fields like `tracking_number` as raw bytes.
