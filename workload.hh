
#include <string>
#include <vector>

class workload {

public:
  
	
  //stores the elements needed to make a cache request
  struct request {
    std::string type;
    std::string key;
    const char* value;
  };

  //constructor should take as parameters the default settings for prob_get, prob_set, prob_del, prob_del, prob_reuse, size parameters
  workload(int nreqs = 0);

	
  ~workload();
		

  // //gets the requests in the workload
  const std::vector<request> get_reqs();
	
  //populates the requests vector with a
  //ratio of gets, sets, and del's determined
  //by the user
  //also includes options to control key reuse
  //and invalidation
  //uses the stored parameters
  void gen_reqs(int nreqs);

  //sets the workload parameters for future request generations
  void set_parameters(double prob_get = 20,
		      double prob_set = 1,
		      double prob_del = 7,
		      double prob_reuse = 0.96,
		      double prob_inval = 0.1,
		      double key_size_alpha = 4,
		      double key_size_beta = 7,
		      double value_size_alpha = 1,
		      double value_size_beta = 200);
	
  
private:
  //stores keys used in set requests
  std::vector<std::string> keys_used_;
	
  //stores the actual request objects
  std::vector<request> reqs_;
		
  int ngets_;
  int nsets_;
  int ndels_;

  double prob_get_ = 0.33;
  double prob_set_ = 0.33;
  double prob_del_ = 0.33;
  double prob_reuse_ = 0.1;
  double prob_inval_ = 0.1;
  double key_size_alpha_ = 1;
  double key_size_beta_ = 1;
  double value_size_alpha_ = 1;
  double value_size_beta_ = 1;

  //generates random keys	
  std::string gen_key(bool reuse, bool add);
	
  //generates random data for set requests by allocating a value on the heap. responsibility of user to deallocate?
  const char* gen_value();
  
};
	






