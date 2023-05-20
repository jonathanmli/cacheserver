
#include "workload.hh"
#include "cache.hh"
#include <chrono>
#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <cmath>

using measurment_type = double;
using throughput_type = double;

std::mutex shared;

//global object to communicate with baseline_latencies threads
//collects measurement vectors normally returned by baseline_latencies
std::vector<std::vector<measurment_type>> final_results {};

//parses and sends a workload request to the cache. returns true if successful
bool send_request_to_cache(Cache& c, workload::request r) {
  key_type key{r.key};
  Cache::val_type val {r.value,sizeof(r.value)};
  //c.space_used();
   if(r.type == "get") {
    Cache::val_type ret {c.get(key)};
  	auto out = ret.data_ != nullptr;
  	delete[] ret.data_;
  	return out;
  }
  if(r.type == "set") {return c.set(key,val);}
  if(r.type == "del") {return c.del(key);}
  return false;
}

//wrapper to conduct nreq requests 
std::vector<measurment_type> baseline_latencies(unsigned nreq, Cache& c) {

  //scoped in order to deconstruct workload after use
  if (true) {
    //create the workload
    workload w {static_cast<int>(nreq/2)};

    w.set_parameters(20,
		       1,
		       7,
		       0.96,
		       0.1,
		       4,
		       7,
		       1,
		     20);

    //reset and warm up the cache by inserting nreq/2 requests
    c.reset();
    auto rs = w.get_reqs();
    for (auto r : rs) {
      send_request_to_cache(c, r);
    }
  }
  
  // std::cout << "hi" << std::endl;
	
  //create the workload
  workload wk {static_cast<int>(nreq)};

  //generate requests and time how long to process each one of them	
  std::vector<measurment_type> out;
  measurment_type sum_duration;
  auto rs = wk.get_reqs();
	
  for (auto r : rs) {
    auto t1 = std::chrono::high_resolution_clock::now();
    send_request_to_cache(c, r);
    auto t2 = std::chrono::high_resolution_clock::now();
    sum_duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();
    out.push_back(sum_duration);
  }
  
  //send results to the global data object (for multithreading)
  shared.lock();
  final_results.push_back(out);
  shared.unlock();
    
  return out;
}

//a wrapper that allows easy manipulation of parameters and outputs two stats
std::pair<measurment_type, throughput_type> baseline_performance(unsigned nreq, Cache& c) {

  //run baseline latencies 
  auto bl = baseline_latencies(nreq, c);

  //compute relevant statistics
  throughput_type tp;
  measurment_type lat;
  tp = static_cast<throughput_type>(nreq)/static_cast<throughput_type>(std::accumulate(bl.begin(), bl.end(), 0));

  std::cout << std::accumulate(bl.begin(), bl.end(), 0) << std::endl;

  std::sort(bl.begin(), bl.end());
  lat = bl[static_cast<unsigned>(0.95 * nreq)];

  //return the stats

  std::pair<measurment_type, throughput_type> out {lat/1000000.0,1000000.0*tp};

  return out;
}



std::pair<measurment_type, throughput_type> threaded_performance(unsigned nreq, unsigned nthreads) {

  //run baseline latencies
  
  //vectors of cache and thread objects
  
  std::vector<std::shared_ptr<Cache>> caches {};
  std::vector<std::thread> threads {};
   

	//construct baselinr_latencies threads and caches, push to releant vectors
	
   for(unsigned i=0;i<nthreads;i++) {
     std::shared_ptr<Cache> sp(new Cache("127.0.0.1","8555"));
     caches.push_back(sp);
     std::thread t([&]() {baseline_latencies(nreq,*sp);});
     threads.push_back(std::move(t));
   }
   
   for(auto &t : threads) {
     t.join();
   }

  //compute relevant statistics
  throughput_type tp = 0;
  measurment_type lat;
  std::vector<measurment_type> tot {};
  measurment_type max_time = 0.0;

	for(auto res : final_results) {
		
	measurment_type res_time = abs(static_cast<throughput_type>(std::accumulate(res.begin(), res.end(), 0)));
	
	if(res_time > max_time) {
		max_time = res_time;
	}
  
  }
  for(auto res : final_results) {
  
 
    tp += static_cast<throughput_type>(nreq)/max_time;

    tot.insert(tot.end(),res.begin(),res.end());
  
  }
  std::sort(tot.begin(), tot.end());
  lat = tot[static_cast<unsigned>(0.95 * nreq)];

  //return the stats
  std::pair<measurment_type, throughput_type> out {lat/1000000.0,1000000.0*tp};
  
  return out;
}



int main(int argc, char* argv[]) {
  
  // Check command line arguments.
  if ((argc != 2) and (argc != 3))
  {
      std::cerr <<
          "Usage: driver <0 for all latency measurements, 1 for derived stats>\n" <<
          "Example:\n" <<
          "    driver 1\n";
      return EXIT_FAILURE;
  }

  unsigned nreq = 10000;
  Cache test_cache {"127.0.0.1","8555"};

  auto option = static_cast<unsigned short>(std::atoi(argv[1]));

  if (option == 0) {
    auto measurement_results = baseline_latencies(nreq,test_cache);

    for(auto res : measurement_results) { 
      
      //print measurement results to csv or simply to terminal in csv format
      std::cout << res << std::endl;

    }
  }

  else if (option == 1){
    auto performance_results = baseline_performance(nreq,test_cache);
    // std::cout << "The 95 percentile is " << performance_results.first << std::endl;
    // std::cout << "The throughput is " << performance_results.second << std::endl;
    //print out in csv
    std::cout << "95th percentile latency: " << performance_results.first << ", throughput: " << performance_results.second << std::endl;
    //The code below will be the actual driver code
  }

  else if (option == 2){
	unsigned nthreads = static_cast<unsigned short>(std::atoi(argv[2]));
	auto performance_results = threaded_performance(nreq,nthreads);
    // std::cout << "The 95 percentile is " << performance_results.first << std::endl;
    // std::cout << "The throughput is " << performance_results.second << std::endl;
    //print out in csv
	
    std::cout << "95th percentile latency: " << performance_results.first << ", throughput: " << performance_results.second << std::endl;
    //The code below will be the actual driver code
  }
  
  return 0;
}
