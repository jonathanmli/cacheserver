#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <string>

#include <algorithm>
#include "cache.hh"
#include <iostream>
#include <unordered_map>
#include <vector>
#include "assert.h"
#include <memory>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>



  // Create a new cache object with the following parameters:
  // maxmem: The maximum allowance for storage used by values.
  // max_load_factor: Maximum allowed ratio between buckets and table rows.
  // evictor: Eviction policy implementation (if nullptr, no evictions occur
  // and new insertions fail after maxmem has been exceeded).
  // hasher: Hash function to use on the keys. Defaults to C++'s std::hash.

class Cache::Impl
{
  public:
	std::string host_;
	std::string port_;
	net::io_context ioc_;
	tcp::resolver resolver_;
	beast::tcp_stream stream_;	
		
	Impl(std::string host, std::string port);

  ~Impl();
  private: 
};	

Cache::Impl::Impl(std::string host, std::string port): 
  host_(host),
  port_(port),
  ioc_(),
  resolver_(ioc_),
  stream_(ioc_)
{
  //establish connection to server
  auto results = resolver_.resolve(host,port);
	stream_.connect(results);
}


Cache::Impl::~Impl() {

  // Gracefully close the socket
  beast::error_code ec;
  stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

}

// Cache::Cache(size_type maxmem,
//     float max_load_factor,
//     Evictor* evictor,
//     hash_func hasher) = default;



Cache::Cache(std::string host, std::string port): 
	pImpl_(new Impl(host,port))
{ }
	
Cache::~Cache() {
}



// Add a <key, value> pair to the cache.
// If key already exists, it will overwrite the old value.
// Both the key and the value are to be deep-copied (not just pointer copied).
// If maxmem capacity is exceeded, enough values will be removed
// from the cache to accomodate the new value. If unable, the new value
// isn't inserted to the cache.
   // Returns true iff the insertion of the data to the store was successful.



bool Cache::set(key_type key, val_type val) {
  //write http target
  std::string skey {key};
  std::string sval {std::string(val.data_)};
  std::string starg {"/" + skey + "/" + sval};
  const char* targ {starg.c_str()};

  //assemble request
  http::request<http::string_body> req{http::verb::put, targ, 11};
  req.set(http::field::host, pImpl_->host_);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.target(targ);

  //send request to server
  http::write(pImpl_->stream_, req);
  beast::flat_buffer buffer;

  //read server response
  http::response<http::dynamic_body> res;
  http::read(pImpl_->stream_, buffer, res);

  //check result from server
  return res.result_int() == 204;
}	


Cache::val_type Cache::get(key_type key) const {
    //assemble request
    std::string skey = static_cast<std::string>(key);
    http::request<http::string_body> req{http::verb::get, "/" + skey, 11};

    //set request fields and send to server
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    http::write(pImpl_->stream_, req);

    //store server response
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(pImpl_->stream_, buffer, res);


    //if status is not found, return nullptr valtype
    if (res.result() == http::status::not_found) {
      val_type out {nullptr,0};
      return out;
    }

    //store response body
    std::string jsonTuple = beast::buffers_to_string(res.body().data());

    //parse jsontuple, assume gets json tuple
    size_t pos = 0;
    pos = jsonTuple.find(":");
    jsonTuple.erase(0, pos+1);
    pos = jsonTuple.find("}");

    //make val_type from json tuple
    auto sval = jsonTuple.substr(0, pos);   
    auto size = static_cast<size_type>(sval.size()+1); 
    char* val = new char[size];
    strcpy(val,sval.c_str());
    
    val_type out {val,size};
    return out;
}


// Delete an object from the cache, if it's still there.
// Returns true iff the object was deleted from the store.
bool Cache::del(key_type key) {
  //assemble request
  http::request<http::string_body> req{http::verb::delete_, "/" + key, 11};
  req.set(http::field::host, pImpl_->host_);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  //send request
  http::write(pImpl_->stream_, req);

  //store and parse server response
  beast::flat_buffer buffer;
  http::response<http::dynamic_body> res;
  http::read(pImpl_->stream_, buffer, res);
  return res.result_int() == 204;
}

// Compute the total amount of memory used up by all cache values (not keys)
Cache::size_type Cache::space_used() const {
  //assemble request
  http::request<http::string_body> req {http::verb::head, "/", 11};

  //set request fields and send to server
  req.set(http::field::host, pImpl_->host_);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  http::write(pImpl_->stream_, req);

  //store server response
  beast::flat_buffer buffer;
  http::response<http::empty_body> res;
  http::read(pImpl_->stream_, buffer, res);

  //extract space-used from response
  std::string sSpaceUsed = static_cast<std::string>(res.base().at("Space-used"));

  // std::cout << "SU:" << res.base().at("Space-used") << std::endl;
  return static_cast<Cache::size_type>(std::stoi(sSpaceUsed));
}

// Return the ratio of successful gets to all gets
double Cache::hit_rate() const {
  //assemble request and sendd to server
  http::request<http::string_body> req {http::verb::head, "/", 11};
  req.set(http::field::host, pImpl_->host_);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  http::write(pImpl_->stream_, req);

  //store server response
  beast::flat_buffer buffer;
  http::response<http::empty_body> res;
  http::read(pImpl_->stream_, buffer, res);

  //extract HitRate string  from response
  std::string sHitRate = static_cast<std::string>(res.base().at("Hit-rate"));

  //std::cout << "SU:" << res.base().at("Space-used") << std::endl;
  return static_cast<double>(std::stof(sHitRate));
}

// Delete all data from the cache and return true iff successful
bool Cache::reset() {
  //assemble request and send to server
  http::request<http::string_body> req{http::verb::post, "/reset", 11};
  req.set(http::field::host, pImpl_->host_);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  http::write(pImpl_->stream_, req);

  //store and return confirmation from server 
  beast::flat_buffer buffer;
  http::response<http::dynamic_body> res;
  http::read(pImpl_->stream_, buffer, res);
  return res.result_int() == 204;
}




