[![Work in Repl.it](https://classroom.github.com/assets/work-in-replit-14baed9a392b3a25080506f3b7b6d57f295ec2978f6f33ec97e36a161684cbe9.svg)](https://classroom.github.com/online_ide?assignment_repo_id=339747&assignment_repo_type=GroupAssignmentRepo)
# HW6: Crank it up!

The goal of this final part of the project is to maximize your cache server's performance with multithreading and code optimization.

## Overview

> We used std::threads to add multithreading to our driver and server. We multithreaded out driver code by running our benchmark program in std::threads with seperately constructed clients. We then used our multithreaded driver to find the maximum throughput of our single-threaded server. We then multithreaded our server by creating multiple request-handling threads and using mutexes to protect our cache when handling get, set, and reset requests. 

## Part 1 (50%): multi-threaded benchmark

Before we can improve the performance of the server, we must improve the performance of the client. Currently, the throughput our client can measure is limited by the fact that it's only running one synchronous request at a time. In this first part, we'll increase the offered load by increasing the number of client threads, until we find the actual limits of the cache.

To do this, you'll have to encapsulate your benchmarking function (`baseline_latencies`) in an independent thread, and run several of those. Check and ensure the code in this function is thread-safe, meaning that it doesn't modify any state that isn't private to the function / thread (`const` global or `thread_local` variables are thread-safe). Then, write a wrapper function called `threaded_performance` that launches `nthreads` worker threads. Each thread, like before, sends `nreq` synchronous requests to the cache server and returns the resulting vector of latencies. The wrapper thread finds the 95th-percentile latency of all these latencies combined, as well as the combined throughput of all these threads.

Then, create two graphs and include them in your README, one showing the aggregate throughput as a function of the number of worker threads (`nthread`), and another showing the 95th-percentile latency as a function of `nthreads`. (If you know how, you may combine those into a single graph with two different y-axes.) Identify the saturation point and report your maximum unsaturated throughput and latency in the README. This is your new performance baseline.

>We added multithreading to the driver by running our previous baseline_latencies inside of independent threads whith seperately constructed cache-server connections. We stored the thread outputs in a global vector of measurement result vectors, then used each vector to calculate our net throughput and combined the vectors to find a total 95th percentile latency. We found little correlation between the mean latency and nthreads, as expected, and we found a maximum unsaturated throughput of 20.1 requests per ms, at four threads (different trial than shown below).

![multithreaded_95th_percentile_latencies.png](multithreaded_95th_percentile_latencies.png?raw=true)
![multithreaded_throughputs.png](multithreaded_throughputs.png?raw=true)

## Part 2: multithreaded server (50%)

Your new bottleneck should now be the single-threaded server. Rectify this by modifying `cache_server.cc` to run multiple asynchronous request-responding threads. You will have to protect all accesses to shared mutable data (primarily the cache) with appropriate locks. It may take extensive testing to convince yourself that there are no race conditions in your code. (In some cases, these would cause a crash; in others, just data inconsistencies, which are harder to detect. Sprinkle `assert`s liberally in your cache server.)

Now, reproduce your graphs from Part 1 using the new server. Report your new graphs and saturation point in the README. Try different numbers of server threads and see how that affects performance. Remember that server threads are competing with client threads if you're running them both on the same machine. If possible, run the client on a different machine Report any interesting findings.

> We multithreaded the server by having the server run multiple threads with a shared cache object. To ensure that there are no race conditions, a corresponding mutex was created for the common cache object. The cache is only locked for the get, set, and reset operations on the cache, since these are the only operations that change the cache. After a few tests we were convinced that no race conditions exist. 

> The following are our multithreaded-server histograms:

>2 threads:

![latencies-2-threads.png](latencies-2-threads.png?raw=true)
![throughputs-2-threads.png](throughputs-2-threads.png?raw=true)

>3 threads:

![latencies-3-threads.png](latencies-3-threads.png?raw=true)
![throughputs-3-threads.png](throughputs-3-threads.png?raw=true)

>4 threads:

![latencies-4-threads.png](latencies-4-threads.png?raw=true)
![throughputs-4-threads.png](throughputs-4-threads.png?raw=true)

>Two features worth noting are that the throughput never really exceeded 20 reqs/ms and that only three client threads could run against the server. Both of these limitations can probably be explained by the fact that this test was run on one machine, with only 8 threads in total.

## Part 3: Optimization (10% extra credit)

Now that you have the best performance you can get out of the existing code with multiple threads, it's time to squeeze any remaining performance benefits with code optimization. Try different modifications to your cache server code (primarily in `cache_store.cc`) and remeasure performance. Report your experiments and findings in your README.
