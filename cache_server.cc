//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>


#include "cache.hh"
// #include "lru_evictor.hh"

//we will be using the default eviction policy

namespace po = boost::program_options; 	// from <boost/program_options.hpp>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    Cache &cache_,
    std::mutex &mutex_,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send)
{

    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };


    // Returns a method not allowed response
    auto const method_not_allowed =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(what);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head && 
        req.method() != http::verb::put &&
        req.method() != http::verb::delete_ &&
        req.method() != http::verb::post)
        return send(method_not_allowed("The method is not allowed."));


    // error detection
    beast::error_code ec;

    // Handle an unknown error
    if(ec)
        return send(server_error(ec.message()));

 	// Respond to PUT request
    if(req.method() == http::verb::put) {
    	http::response<http::empty_body> res; 

    	//parse out request body
    	auto targ = req.target();
    	auto target_string = std::string(targ);

    	// Request key must exist
		if( req.target().empty() ||
		    req.target()[0] != '/')
		    return send(bad_request("Illegal request-key"));

		size_t pos = 0;
		target_string.erase(0, pos + 1);
		pos = target_string.find("/");

		//Request value must exist 
		if (pos == std::string::npos)
			return send(bad_request("Illegal request-key"));

    	auto key = target_string.substr(0, pos);
    	target_string.erase(0, pos + 1);
    	auto val = target_string;

    	//Request value and key must be valid
    	if (key.size() == 0 || val.size() == 0)
    		return send(bad_request("Illegal request-key"));

    	//create new val type
    	char* new_array = new char[val.size()+1];
    	strcpy(new_array, val.c_str());
    	Cache::val_type new_val {new_array, static_cast<Cache::size_type>(val.size()+1)};

    	//lock since we are modifying the cache
    	{
    		std::lock_guard guard(mutex_);

    		//return error if value could not be placed
	    	if (!cache_.set(key, new_val)) {
	    		return send(server_error("Could not place key"));
	    	}
    	}

    	

    	//delete the previously allocated array
    	delete[] new_array;

    	//send the response
		res.version(req.version());
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING); 
		res.result(204); //request processed, no content
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
    } 
   

    // Respond to GET request
    if(req.method() == http::verb::get) {
    	http::response<http::string_body> res; 

    	//parse out request body
    	auto targ = req.target();
    	auto target_string = std::string(targ);

    	// Request key must be valid.
		if( req.target().empty() ||
		    req.target()[0] != '/')
		    return send(bad_request("Illegal request-key"));

    	auto key = target_string.substr(target_string.find("/")+1, target_string.size()-1);

    	const Cache::byte_type* value;

    	//lock since we are modifying the cache (hit rate to be specific)
    	{
    		std::lock_guard guard(mutex_);

    		value = cache_.get(key).data_;
    	}

    	//return error if key not found
    	if (value == nullptr) {
    		return send(not_found(targ));
    	}


    	//send response
		res.version(req.version());
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING); 
		res.result(http::status::ok);

		//json tuple format
		res.body() = "{" + key + ":" + value + "}"; 

		//make sure to deallocate
		delete[] value;
		res.prepare_payload();
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
    } 

    // Respond to HEAD request
    if(req.method() == http::verb::head) {

    	http::response<http::empty_body> res; 

    	//no need to lock since these operations do not write

    	//create new fields
    	res.insert("Space-used", std::to_string(cache_.space_used()));
  		res.insert("Hit-rate", std::to_string(cache_.hit_rate()));

    	//set fields
    	res.set(http::field::content_type, "application/json");
    	res.set(http::field::accept, "text/html");

    	//send response
		res.version(req.version());
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING); 

		res.result(200); //request processed
		res.prepare_payload();
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
    } 


    // Respond to DELETE request
    if(req.method() == http::verb::delete_) {
    	http::response<http::empty_body> res; 

    	//parse out request target
    	auto targ = req.target();
    	auto target_string = std::string(targ);

    	// Request key must be valid.
		if( req.target().empty() ||
		    req.target()[0] != '/')
		    return send(bad_request("Illegal request-key"));

    	auto key = target_string.substr(target_string.find("/")+1, target_string.size()-1);


    	//lock since we are modifying the cache
    	{
    		std::lock_guard guard(mutex_);
	    	//return error if not deleted
	    	if (!cache_.del(key)) {
	    		return send(server_error("Could not delete"));
	    	}
    	}

    	//send response
		res.version(req.version());
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING); 
		res.result(204); //request processed, no content
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
    } 



     // Respond to POST request
    if(req.method() == http::verb::post) {
    	http::response<http::empty_body> res; 

    	//parse out request body
    	auto targ = req.target();
    	auto target_string = std::string(targ);

    	//return error if not correct command
    	if (target_string != "/reset") {
    		return send(not_found(targ));
    	} 

    	//lock since we are modifying the cache
    	{
    		std::lock_guard guard(mutex_);
	    	//reset cache
	    	if (!cache_.reset()) {
	    		return send(server_error("Could not reset"));
	    	}
	    }

    	//send response
		res.version(req.version());
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING); 
		res.result(204); //request processed, no content
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
    } 

    return send(bad_request("Not yet implemented"));
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    //no idea how this works

    //my understanding is that we transform the session into an functor that accepts http::message and writes a response (void output)
    struct send_lambda
    {
        session& self_;

        explicit
        send_lambda(session& self)
            : self_(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            // Write the response
            http::async_write(
                self_.stream_,
                *sp,
                beast::bind_front_handler(
                    &session::on_write,
                    self_.shared_from_this(),
                    sp->need_eof()));
        }
    };

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;

    //replace with cache
   	Cache &cache_;

   	//Store mutex
   	std::mutex &mutex_;

    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket,

        Cache &cache_,

        std::mutex &mutex_)

        : stream_(std::move(socket))
        , cache_(cache_) 
        , mutex_(mutex_)
        , lambda_(*this)

    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(
                          &session::do_read,
                          shared_from_this()));
    }

    void
    do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request
        http::async_read(stream_, buffer_, req_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");


        //should recieve input on how to handle request
        // Send the response
        handle_request(cache_, mutex_, std::move(req_), lambda_);
    }

    void
    on_write(
        bool close,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res_ = nullptr;

        // Read another request
        do_read();
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    Cache cache_;
    std::mutex mutex_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        Cache::size_type maxmem)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , cache_(maxmem)
        , mutex_()
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        do_accept();
    }

private:
    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, tcp::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(
                std::move(socket),
                cache_, mutex_)->run(); //pass reference to cache and the mutex
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

/*
This function (main) receives four optional command line arguments, -m maxmem, -s server, -p port, and -t threads. 
At this point, you can ignore the thread count and always assume the server runs on localhost (127.0.0.1). 
The maxmem parameter is used to construct the cache, and the port represents the port number to bind to (both should have reasonable defaults in the case they're not provided). 
You can use getopt, Boost.Program_options, or other libraries to parse the command line. Choose reasonable defaults for these parameters if values aren't provided in the command line. 
Don't use a well-known port number (see Wikipedia for a list), and prefer a high port number (>2000).

main() then calls whatever functions it needs to set up a cache, and establishes a receiving (TCP) socket. 
It then loops indefinitely, listening for messages, processing the requests, and returning the appropriate responses. 
These are the valid HTTP messages that the server accepts (see also here):
*/
int main(int argc, char* argv[])
{
	//optional variables
    Cache::size_type maxmem;
    std::string server;
    unsigned short port;
    int threads;

    //create option menu
    po::options_description desc("Allowed Options");

    desc.add_options()
    	("help", "This function (main) receives four optional command line arguments, -m maxmem, -s server, -p port, and -t threads. \n Usage: cache_server -m <maxmem> -s <server> -p <port> -t <threads>")
 		("maxmem,m", po::value<Cache::size_type>(&maxmem) -> default_value(1000000))
 		("server,s", po::value<std::string>(&server) -> default_value("127.0.0.1"))
 		("port,p", po::value<unsigned short>(&port) -> default_value(8555))
 		("threads,t", po::value<int>(&threads) -> default_value(1))
 	;

 	po::variables_map vm;
 	po::store(po::parse_command_line(argc, argv, desc), vm);
 	po::notify(vm);

 	//print help when needed
 	if (vm.count("help")) {
	    std::cout << desc << std::endl;
	    return 1;
	}

 	auto const address = net::ip::make_address(server);


    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        maxmem)->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}

