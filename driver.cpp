// ---------------------------- driver.cpp ---------------------------
// Sean Eaton CSS503
// Created: May 12th, 2020 
// Last Modified: May 12th, 2020
// -------------------------------------------------------------------
// Driver file for thread-based simulation of a barbershop
// -------------------------------------------------------------------
#include <iostream>   
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include "Shop.h"

using namespace std;

struct ThreadParam { // a small struct for passing multiple parameter into barber and customer runnable functions
  Shop *shop; // pointer to Shop object
  int id; // thread identifier
  int service_time; // service time (in usec) for barber; will be 0 for customer
};

void *barber( void * );
void *customer( void * );

// ------------------------------- main ------------------------------
// Main function for creater barber/customer threads and running them
// Preconditions: None
// Postconditions: a complete barbershop simulation will have been run
int main( int argc, char *argv[] ) {

  int n_barbers = atoi( argv[1] );
  int n_chairs = atoi( argv[2] );
  int n_customers = atoi( argv[3] );
  int service_time = atoi( argv[4] );

  pthread_t barber_threads[n_barbers];
  pthread_t customer_threads[n_customers];
  Shop shop( n_barbers, n_chairs );
  struct ThreadParam barber_param_array[n_barbers];
  
  for ( int i = 0; i < n_barbers; i++ ) {
    int id = i;
    struct ThreadParam barber_params;
    barber_params.shop = &shop;
    barber_params.id = id;
    barber_params.service_time = service_time;
    barber_param_array[i] = barber_params;
    pthread_create( &barber_threads[i], NULL, barber, &barber_param_array[i] );
  }
  for ( int i = 0; i < n_customers; i++ ) {
    usleep( rand( ) % 1000 );
    int id = i + 1;
    struct ThreadParam customer_params;
    customer_params.shop = &shop;
    customer_params.id = id;
    customer_params.service_time = service_time;
    pthread_create( &customer_threads[i], NULL, customer, &customer_params );
  }

  for ( int i = 0; i < n_customers; i++ )
    pthread_join( customer_threads[i], NULL );

  for ( int i = 0; i < n_barbers; i++ ) {
    pthread_cancel( barber_threads[i] );
  }
  cout << "# customers who didn't receive a service = " << shop.n_dropoffs
       << endl;

  return 0;
}

// ------------------------------ barber -----------------------------
// Runnable for barber threads, will call relevant functions from the 
// shop monitor class
// Preconditions: A properly initialized ThreadParam struct
// Postconditions: All barber thread shop functions will have been run
void *barber( void *arg ) {
  // extract parameters
  struct ThreadParam *param = ( struct ThreadParam* )arg;
  Shop &shop = *(param->shop);
  int id = param->id;
  int service_time = param->service_time;
  shop.makeBarberAvailable( id ); // initialize as available (within barber_available queue)

  while( true ) {
    shop.helloCustomer( id );
    usleep( service_time );
    shop.byeCustomer( id );
  }
}

// ---------------------------- customer -----------------------------
// Runnable for customer threads, will call relevant functions from 
// the shop monitor class
// Preconditions: A properly initialized ThreadParam struct
// Postconditions: All customer thread shop functions will have been 
// run
void *customer( void *arg ) {
  // extract parameters
  struct ThreadParam *param = ( struct ThreadParam* )arg;
  Shop &shop = *(param->shop);
  int id = param->id;

  int barber_id = -1;
  if ( (barber_id = shop.visitShop( id )) != -1 ) { // this will have the customer visit the shop
                                               // and if the customer's "barber"s id is still
                                               // -1, then that means he was didn't get a barber
                                               // and it also means there was nowhere to wait
    shop.leaveShop( id, barber_id );
  }
  pthread_exit(0);
}
