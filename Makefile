CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O3
LDFLAGS=$(CXXFLAGS)
LIBS=-pthread -lboost_program_options
OBJ=$(SRC:.cc=.o)

all:  cache_server test_cache_store test_cache_client test_evictors test_workload driver

cache_server: cache_server.o cache_store.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_evictors: test_evictors.o lru_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_store: test_cache_store.o cache_store.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_client: test_cache_client.o cache_client.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_workload: test_workload.o workload.o cache_store.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

driver: driver.o cache_client.o workload.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	rm -rf *.o test_cache_client test_cache_store test_evictors cache_server test_workload driver

test: all
	./test_cache_store
	./test_evictors
	echo "test_cache_client must be run manually against a running server"

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./test_cache_store
	valgrind --leak-check=full --show-leak-kinds=all ./test_evictors
