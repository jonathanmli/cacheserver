



# Benchmarking


## Overview

In order to test the performance of our cache server, we wrote a workload generator, a program which creates std::vectors of request objects which can be easily reinterpreted as cache commands, with options to control the ratios ofget, set, and delete commands, and to control key reuse and invalidation. We then wrote baseline_latencies and baseline_performance. Latencies runs latency tests on the cache using the workload requests, and then performance uses those results to compute the 95th percentile latency and throughput.

## Preliminaries

We made the following changes after copying over previous files:

## Part 1: Workload generation 


Our workload generates std::vectors of request objects that a simple function can translate as cache requests. When the workload is generated, the user has options to midify it's size, the relative frequency of request types, the frequencies of key reuse and invalidation, and the sizes of keys and values sent to the cache.

Based on TDD principles, we created a test_workload program to test the workload. This included testing whether set, get, and del ratios can be set and what the hit rate is. The test code is hooked onto cache_store for easy debugging.

In order to mimic the ETC workload key and value sizes, we used a gamma distribution for both of them with parameters a=4, b=7 and a=1, b=200 respectively after experimentation in mathematica to produce matching PDF plots. In particular, our gamma distribution ensured that we captured non-linearities in sizes, with an average key size of 28 bytes and average value size of 200 bytes, matching the paper. 

Through experimentation on the parameters, we found that the maxmem and reuse probability parameters that led to a 0.80 hit rate on 1M requests was 10M and 0.96 respectivity. Coincidentally, or by design :), 0.96 was the 6-hour reuse probability in the paper.

We decided to implement an adaptable workload interface that allows users to set parameters, generate new requests, and get the requests currently contained within the workload. This flexible approach will prove useful later.



## Part 2: Baseline latency and throughput 

We created a separate driver program. Within it, `baseline_latencies` would create a workload with 1M requests, a number that decreased the throughput variance empirically. 

The driver warms up the cache with a workload half the size of the test workload before testing

Results are printed to the console

`baseline_performance` runs `baseline_latencies` and computes the two statistics

The baseline histogram is shown below
![baseline_histogram.png](baseline_histogram.png?raw=true)

We see a exponential decay trend, which makes sense given our gamma distributed value and key sizes


## Part 3: Sensitivity testing 

We choose to test changes to the cache maxmem (1000000 bytes vs. 10000000 bytes), eviction policy (no eviction, LRU, and FIFO), use of optimization, and underlying architecture (mac vs. linux vm). We recorded the resultant 95th percentile latencies and throughputs. The linux_vm results are the baseline results.

![sensitivity_test_95th_percentile_latencies_ms.png](sensitivity_test_95th_percentile_latencies_ms.png?raw=true)
![sensitivity_test_throughputs.png](sensitivity_test_throughputs.png?raw=true)

Clearly, optimization is the largest factor at play here, which makes sense. The choice of evictor seemd to have no bearing on performance, which was interesting, but may have to do with our use of a large cache. FIFO was worse than LRU as expected. Interestingly, the throughput values were much more uniform than the latencies, it seems possible that differences in 95th percentile latency would be more dramatic to to the exponential distribution of frequencies observed in our baseline tests due to its tails. The Mac also performed worse than the linux, and a smaller maxmem cause a slightly worse performance. 

We also tested the interaction between cache sizes and the value size distribution alpha and beta values since how the size of the values and maxmem should both effect how many items we can store. 

![interaction_test_95th_percentile_latencies_ms.png](interaction_test_95th_percentile_latencies_ms.png?raw=true)
![interaction_test_throughputs.png](interaction_test_throughputs.png?raw=true)

The performance here was fairly uniform, except that for the larger cache_ a change in value sizes significantly affected performance. THis is a little difficult to reconcile, although it may by that with a very large cache, hashed values may "spread out", more affecting hardware cache performance.


We did not suspsect any other interaction effects except possibly between eviction and maxmem. 


# Multithreading and code optimization!

The goal of this final part of the project is to maximize our cache server's performance with multithreading and code optimization.

## Overview

We used std::threads to add multithreading to our driver and server. We multithreaded out driver code by running our benchmark program in std::threads with seperately constructed clients. We then used our multithreaded driver to find the maximum throughput of our single-threaded server. We then multithreaded our server by creating multiple request-handling threads and using mutexes to protect our cache when handling get, set, and reset requests. 

## Part 1: multi-threaded benchmark

We added multithreading to the driver by running our previous baseline_latencies inside of independent threads whith seperately constructed cache-server connections. We stored the thread outputs in a global vector of measurement result vectors, then used each vector to calculate our net throughput and combined the vectors to find a total 95th percentile latency. We found little correlation between the mean latency and nthreads, as expected, and we found a maximum unsaturated throughput of 20.1 requests per ms, at four threads (different trial than shown below).

![multithreaded_95th_percentile_latencies.png](multithreaded_95th_percentile_latencies.png?raw=true)
![multithreaded_throughputs.png](multithreaded_throughputs.png?raw=true)

## Part 2: multithreaded server 

We multithreaded the server by having the server run multiple threads with a shared cache object. To ensure that there are no race conditions, a corresponding mutex was created for the common cache object. The cache is only locked for the get, set, and reset operations on the cache, since these are the only operations that change the cache. After a few tests we were convinced that no race conditions exist. 

The following are our multithreaded-server histograms:

2 threads:

![latencies-2-threads.png](latencies-2-threads.png?raw=true)
![throughputs-2-threads.png](throughputs-2-threads.png?raw=true)

3 threads:

![latencies-3-threads.png](latencies-3-threads.png?raw=true)
![throughputs-3-threads.png](throughputs-3-threads.png?raw=true)

4 threads:

![latencies-4-threads.png](latencies-4-threads.png?raw=true)
![throughputs-4-threads.png](throughputs-4-threads.png?raw=true)

Two features worth noting are that the throughput never really exceeded 20 reqs/ms and that only three client threads could run against the server. Both of these limitations can probably be explained by the fact that this test was run on one machine, with only 8 threads in total.

