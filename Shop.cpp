// ----------------------------- shop.cpp ----------------------------
// Sean Eaton CSS503
// Created: May 12th, 2020 
// Last Modified: May 12th, 2020
// -------------------------------------------------------------------
// Implementation of shop monitor class, providing synchronized 
// functions for barber and customer threads, as well as variables 
// controlling "barbershop" logistical management
// -------------------------------------------------------------------
#include <sstream> 
#include <string>  
#include <iostream>
#include "Shop.h"

// --------------------------- constructor ---------------------------
// Parameterized constructor, initializing all variables for shop 
// operation, using parameters for relevant variables
// Preconditions: None
// Postconditions: All shop variables are initialized
Shop::Shop(int number_barbers, int number_chairs ) {
  this->n_barbers = (number_barbers > 0) ? number_barbers : DEFAULT_BARBERS;
  this->n_chairs = (number_chairs >= 0) ? number_chairs : DEFAULT_CHAIRS;
  this->conducting_service = new bool[n_barbers];
  this->money_paid = new bool[n_barbers];
  this->cond_customer_served = new pthread_cond_t[n_barbers];
  this->cond_barber_paid = new pthread_cond_t[n_barbers];
  this->cond_barber_sleeping = new pthread_cond_t[n_barbers];
  this->barbers_serviced_customer = new int[n_barbers];
  for (int i = 0; i < n_barbers; i++) {
    conducting_service[i] = false;
    money_paid[i] = false;
    barbers_serviced_customer[i] = 0;
  }
  this->n_dropoffs = 0;
  init();
}

// ----------------------- default constructor -----------------------
// Default constructor, initializing all variables for shop operation
// Preconditions: None
// Postconditions: All shop variables are initialized
Shop::Shop( ) {
  this->n_barbers = DEFAULT_BARBERS;
  this->n_chairs = DEFAULT_CHAIRS;
  this->conducting_service = new bool[n_barbers];
  this->money_paid = new bool[n_barbers];
  this->cond_customer_served = new pthread_cond_t[n_barbers];
  this->cond_barber_paid = new pthread_cond_t[n_barbers];
  this->cond_barber_sleeping = new pthread_cond_t[n_barbers];
  this->barbers_serviced_customer = new int[n_barbers];
  for (int i = 0; i < n_barbers; i++) {
    conducting_service[i] = false;
    money_paid[i] = false;
    barbers_serviced_customer[i] = 0;
  }
  this->n_dropoffs = 0;
  init();  
}

// ------------------------------- init ------------------------------
// Called by constructors to initialize all synchronization primitives
// Preconditions: Properly declared synchronization primitives
// Postconditions: All synchronization primitives are initialized
void Shop::init( ) {
  pthread_mutex_init( &mutex, NULL );
  pthread_cond_init( &cond_customers_waiting, NULL );
  for (int i = 0; i < n_barbers; i++ ) {
    pthread_cond_init( &cond_customer_served[i], NULL );
    pthread_cond_init( &cond_barber_paid[i], NULL );
    pthread_cond_init( &cond_barber_sleeping[i], NULL );
  }
}

// ----------------------- makeBarberAvailable -----------------------
// Adds a given barber id to the available barbers queue
// Preconditions: a barber id and a barber_available queue
// Postconditions: barber_available queue will be populated with a new
// barber id
void Shop::makeBarberAvailable( int b_id ) {
  barbers_available.push( b_id );
}

// ------------------------- int2string ------------------------------
// Prints out integer i as a string
// Preconditions: an integer
// Postconditions: integer i will be printed out
string Shop::int2string( int i ) {
  stringstream out;
  out << i;
  return out.str( );
}

// ----------------------------- print -------------------------------
// Print out a preformatted message appending a customer or barber id 
// to an inputted message.
// Use negative value if inputting a barber id.
// Preconditions: an integer and a message
// Postconditions: input message will be printing out with the customer
// or barber id appended before it
void Shop::print( int person, string message ) {
  cout << ( ( person > 0 ) ? "customer[" : "barber  [" )
       << abs(person) << "]: " << message << endl;
}

// --------------------------- visitShop -----------------------------
// A synchronized method simulating a customer entering the shop, 
// waiting if barbers are busy and seats are available, then recieving
// service when called on by a barber
// Preconditions: A shop, a customer thread, and a barber thread
// Postconditions: This customer will have either left the shop due to
// no waiting room, or will have begun being serviced
int Shop::visitShop( int c_id ) {
  pthread_mutex_lock( &mutex );   // lock
  if ( n_chairs > 0 ) { // if management of waiting chairs is required
    if ( customers_waiting.size( ) == n_chairs ) {  // waiting chairs are all occupied
      print( c_id,"leaves the shop because of no available waiting chairs.");
      ++n_dropoffs;
      pthread_mutex_unlock( &mutex );
      return -1;                 // leave the shop
    }
    if ( barbers_available.empty() && customers_waiting.size( ) != n_chairs ) { // barbers are all working, and their is room to wait
      customers_waiting.push( c_id );    // have a waiting chair

      print( c_id, "takes a waiting chair. # waiting seats available = " 
        + int2string( n_chairs - customers_waiting.size( ) ) );
      while ( barbers_available.empty() ) // wait until a barber is available
        pthread_cond_wait( &cond_customers_waiting, &mutex );
      customers_waiting.pop( );        // stand up
    }
  }
  else { // if no waiting chairs need to be managed
    if ( barbers_available.empty() ) { // barbers are busy
      print( c_id,"leaves the shop because of no available waiting chairs.");
      ++n_dropoffs;
      pthread_mutex_unlock( &mutex );
      return -1;                 // leave the shop
    }
  }

  int b_id = barbers_available.front( ); // get the next available barber
  barbers_available.pop( );
  print( c_id, "moves to the service chair[" + int2string(b_id) + "]. # waiting seats available = " 
	 + int2string( n_chairs - customers_waiting.size( ) ) );
  barbers_serviced_customer[b_id] = c_id;
  conducting_service[b_id] = true; //associate barber starts service
  pthread_cond_signal( &cond_barber_sleeping[b_id] ); // wake up the barber just in case if he is sleeping

  pthread_mutex_unlock( &mutex ); // unlock
  return b_id;
}

// --------------------------- leaveShop -----------------------------
// A synchronized method simulating a customer leaving the shop, first
// waiting for their barber to finish their hair-cut, then concluding 
// their visit, allowing the barber to become available again.
// Preconditions: A shop, a customer thread, and a barber thread
// Postconditions: This customer will have left the shop
void Shop::leaveShop( int c_id, int b_id ) {
  pthread_mutex_lock( &mutex );   // lock

  print( c_id, "wait for barber[" + int2string( b_id ) + "] to be done with hair-cut" );
  while ( conducting_service[b_id] )                           // while being served
    pthread_cond_wait( &cond_customer_served[b_id], &mutex );  // just sit.

  money_paid[b_id] = true;
  barbers_available.push( b_id );
  pthread_cond_signal( &cond_barber_paid[b_id] );
  
  barbers_serviced_customer[b_id] = 0;

  print( c_id, "says good-bye to barber[" + int2string( b_id ) + "]." );
  pthread_mutex_unlock( &mutex ); // unlock
}

// ------------------------- helloCustomer ---------------------------
// A synchronized method simulating a barber sleeping until a
// customer wakes them for a hair-cut, then starting that process
// Preconditions: A shop, a customer thread, and a barber thread
// Postconditions: A customer will have begun to recieve service
void Shop::helloCustomer( int b_id ) {
  pthread_mutex_lock( &mutex );   // lock

  if ( customers_waiting.empty( ) && !conducting_service[b_id] ) { // no customers
    print( -b_id, "sleeps because of no customers." );
    while( !conducting_service[b_id] )
      pthread_cond_wait( &cond_barber_sleeping[b_id], &mutex ); // wait while not conducting service
  }
  else {
    while( !conducting_service[b_id] )
      pthread_cond_wait( &cond_barber_sleeping[b_id], &mutex );
  }
  print( -b_id, "starts a hair-cut service for customer[" + int2string( barbers_serviced_customer[b_id] ) + "]");

  pthread_mutex_unlock( &mutex );  // unlock
}

// -------------------------- byeCustomer ----------------------------
// A synchronized method simulating a barber finishing a hair-cut,
// concluding the transaction/telling the customer they can go, and 
// calling in another customer.
// Preconditions: A shop, a customer thread, and a barber thread
// Postconditions: A customer will have been allowed to leave, and 
// another will be told to begin their service.
void Shop::byeCustomer( int b_id ) {
  pthread_mutex_lock( &mutex );    // lock

  print( -b_id, "says he's done with a hair-cut service for customer[" + 
	 int2string( barbers_serviced_customer[b_id] ) + "]");

  money_paid[b_id] = false;
  conducting_service[b_id] = false;
  pthread_cond_signal( &cond_customer_served[b_id] );   // tell the customer "done"

  while ( !money_paid[b_id] ) {
    pthread_cond_wait( &cond_barber_paid[b_id], &mutex );
  }

  print( -b_id, "calls in another customer" );
  pthread_cond_signal( &cond_customers_waiting ); // call in another one

  pthread_mutex_unlock( &mutex );  // unlock
}
