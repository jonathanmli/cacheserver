
#include "workload.hh"
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>

//random letter generator for keys and values

char rand_letter() {
  const char* alpha {"abcdefghijklmnopqrstuvwxyz"};
	
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, 25);
	
  int rand = (int)dis(gen);
	
  return alpha[rand];
}

workload::workload(int nreqs)
{
  set_parameters();
  gen_reqs(nreqs);
}

workload::~workload(){
	//delete all request values
	for (auto r : reqs_) {
		delete[] r.value;
	}
}

//random key generator, with options for get and del to reuse keys and for set to add them (to keys_used) 

std::string workload::gen_key(bool reuse, bool add) 
{
    
  std::string key = "";
	
  std::random_device rd;
  std::mt19937 gen(rd());

  std::default_random_engine disgen;

  if(reuse and (not keys_used_.empty())) {
    std::uniform_int_distribution<int> dis1(0, keys_used_.size()-1);
    return keys_used_[(int)dis1(gen)];
  }

  std::gamma_distribution gdis((double)key_size_alpha_,key_size_beta_);
  
  int len = (int)gdis(disgen);
  while((std::find(keys_used_.begin(),keys_used_.end(),key)!=keys_used_.end()) or (key=="")) {
    key += rand_letter();
    for(int i = 0; i < len-2; i++) {		
      key += rand_letter();
    }
  }
  if(add) {
    keys_used_.erase(std::remove(keys_used_.begin(),keys_used_.end(),key),keys_used_.end());
    keys_used_.push_back(key);
  }
  return key;
	
}

// random value generator

const char* workload::gen_value()
{

  std::default_random_engine disgen;
  std::gamma_distribution gdis((double)value_size_alpha_,value_size_beta_);
   
  int size = (int)gdis(disgen);
	
  char* value = new char[size];
	
  for(int i = 0; i < size-1; i++) {
    value[i] = rand_letter();
  }
  value[size-1] = '\0';
  return value;
	
}

// sets workload parameters for construction or reworking

void workload::set_parameters(double prob_get,
			      double prob_set,
			      double prob_del,
			      double prob_reuse,
			      double prob_inval,
			      double key_size_alpha,
			      double key_size_beta,
			      double value_size_alpha,
			      double value_size_beta)

{
  prob_get_ = prob_get;
  prob_set_ = prob_set;
  prob_del_ = prob_del;
  prob_reuse_ = prob_reuse;
  prob_inval_ = prob_inval;
  key_size_alpha_ = key_size_alpha;
  key_size_beta_ = key_size_beta;
  value_size_alpha_ = value_size_alpha;
  value_size_beta_ = value_size_beta;
}
	
// makes the actual requests

void workload::gen_reqs(int nreqs)
  
{				
  
  std::string type {}; 
  std::string key {};
  const char* value {};
		
  double range = prob_get_ + prob_set_ + prob_del_;
		
  double rand {};
		
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(0, range);
	
  std::random_device rd1;
  std::mt19937 gen1(rd1());
  std::uniform_real_distribution<double> dis1(0, 1);
	
  std::random_device rd2;
  std::mt19937 gen2(rd2());
  std::uniform_real_distribution<double> dis2(0, 1);
	
  for(int i=0;i<nreqs; i++) {
    rand = (double)dis(gen);
    if(rand < prob_get_) {
      double rand1 = (double)dis1(gen1);
      type = "get";
			
      if(rand1 > prob_reuse_) {
	key = gen_key(false, false);
      }
      else {
	key = gen_key(true,false);
      }
      reqs_.push_back(request{type,key,nullptr});
      ngets_+= 1;
    }
		
    if((rand >= prob_get_) and (rand < prob_get_ + prob_set_)) {
      type = "set";
      key = gen_key(false,true);
      value = gen_value();
      reqs_.push_back(request{type,key,value});
      nsets_+=1;
    }
		
    if(rand >= prob_get_ + prob_set_) {
      double rand2 = (double)dis2(gen2);
      type = "del";
			
      if(rand2 > prob_inval_) {
	key = gen_key(false,false);
      }
      else {
	key = gen_key(true,false);
      }
      keys_used_.erase(std::remove(keys_used_.begin(),keys_used_.end(),key),keys_used_.end());
      reqs_.push_back(request{type,key,nullptr});
      ndels_+=1;
    }
  }
}

// just a getter method

const std::vector<workload::request> workload::get_reqs() {return reqs_;}
