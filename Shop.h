// ----------------------------- shop.cpp ----------------------------
// Sean Eaton CSS503
// Created: May 12th, 2020 
// Last Modified: May 12th, 2020
// -------------------------------------------------------------------
// Declaration of shop monitor class, providing synchronized functions 
// for barber and customer threads, as well as variables controlling 
// "barbershop" logistical management
// -------------------------------------------------------------------
#ifndef _SHOP_H_
#define _SHOP_H_
#include <pthread.h>
#include <queue>
#include <unistd.h>
#include <string>

using namespace std;

#define DEFAULT_CHAIRS 3 // default number of waiting chairs
#define DEFAULT_BARBERS 1 // default number of barbers

class Shop {
 public:
  Shop( int number_barbers, int number_chairs );
  Shop( );

  int visitShop( int c_id ); // return barber ID or -1 (not served)
  void leaveShop( int c_id, int b_id );
  void helloCustomer( int b_id );
  void byeCustomer( int b_id );
  int n_dropoffs; // the number of customers dropped off

  void makeBarberAvailable( int b_id ); // puts a barber thread's id into the barbers available queue

 private:
  int n_chairs; // the max number of threads that can wait
  int n_barbers; // number of barbers
  queue<int> customers_in_service; // queue of customer IDs being service
  int *barbers_serviced_customer;
  queue<int> barbers_available; // queue of barber IDs waiting for work
  bool *conducting_service; // dynamic array, is the barber with [id] in service? 
  bool *money_paid; // dynamic array is the barber with [id] paid (finished)?
  queue<int> customers_waiting;  // includes the ids of all customer threads waiting in "waiting chairs"

  pthread_mutex_t mutex;
  pthread_cond_t  cond_customers_waiting;
  pthread_cond_t  *cond_customer_served; // dynamic arrays
  pthread_cond_t  *cond_barber_paid;
  pthread_cond_t  *cond_barber_sleeping;
  
  void init( );
  string int2string( int i );
  void print( int person, string message );
};

#endif
