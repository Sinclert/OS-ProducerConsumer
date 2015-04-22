/*
 * Authors:
 *
 * Sinclert Perez (NIA: 100317201)
 * Adrian Pappalardo (NIA: 100317950)
 */

#include <stdio.h>
#include <stdlib.h>
#include "db_factory.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "factory.h"


// Object that will be inserted into the belt (belt)
struct object{
    int id;
    char name [255];
};


/* GLOBAL VARIABLES */
int error_number = -1;
int correct_number = 0;

struct object belt[MAX_BELT]; /* Common belt for transporters and receivers */

// Number of threads for transport
int number_transporters = 0;
// Number of threads for insert
int number_inserters = 0;
// Number of threads for receive
int number_receivers = 0;

pthread_t * transporters;
pthread_t * inserters;
pthread_t * receivers;

// Total number of elements to be transported and already transported at a time
int total_number = 0;

// Number of elements on the belt
int belt_elements = 0;


int main(int argc, char ** argv){

    if (argc == 2){
		    init_factory(argv[1]);
		    close_factory();
    }

    else {
		    perror("Invalid syntax: ./factory input_file ");
		    return 0;
    }
	
    exit(0);
}



/* Function that parses the input file and initializes all structures and resources */
int init_factory(char *file){

    int i = 0;
    int error = 0;

    int * number_elements;
    int * number_modified_elements;
    int * number_modified_stock;


    /* DOUBLE POINTER NEEDED TO PASS AN ONLY ARGUMENT TO THE INSERTION THREADS */
    int ** insertion_args = NULL;


    if (file != NULL){

        FILE * filefd = fopen(file, "r");
        error = fscanf(filefd, "%d", &number_inserters);
        printf("Number inserters %d\n", number_inserters);

        if (error != 1){
            perror("Error reading from file\n");
        }

        number_transporters = 1;
        error = fscanf(filefd, "%d", &number_receivers);
        printf("Number receivers %d\n", number_receivers);

        if (error != 1){
            perror("Error reading from file\n");
        }


        /* Allocation of memory */

        // Number of elements that are going to be inserted by each thread
        number_elements = (int *) malloc(sizeof(int)*number_inserters);
        // Number of elements which stock is going to be updated
        number_modified_elements = (int *) malloc(sizeof(int)*number_inserters);
        // Number of elements that are going to be added to the stock
        number_modified_stock = (int *) malloc(sizeof(int)*number_inserters);
        // Pthread_t of each inserter
        inserters = (pthread_t *) malloc(sizeof(pthread_t)*number_inserters);
        // Pthread_t of each transporter
        transporters = (pthread_t *) malloc(sizeof(pthread_t)*number_transporters);
        // Pthread_t of each receiver
        receivers = (pthread_t *) malloc(sizeof(pthread_t)*number_receivers);

        // Reading elements depending on number of inserters
        for (i=0 ; i< number_inserters ; i++){
            error = fscanf(filefd, "%d %d %d", &number_elements[i], &number_modified_elements[i], &number_modified_stock[i]);

            if (error != 3 || number_modified_elements[i] > number_elements[i]){
                perror("Error reading from file\n");
                exit(-1);
            }
        }

        // Total number of elements that will be in the database
        for (i=0 ; i<number_inserters ; i++){
            total_number += number_elements[i] + (number_modified_elements[i]*number_modified_stock[i]);
        }


        // Init the database
        error = db_factory_init();	
        if (error != 0){
	           perror("Error when initializing the database\n");
        }

        printf("Total number of elements %d\n", total_number);



        /* CREATION OF THE THREADS */
        int j = 0;

        // Inserter threads creation
        while (j < number_inserters){

            // Each row of the "insertion_args" matrix stores the elements to create, modify and the stock to add of each thread
            insertion_args[j][0] = number_elements[j];
            insertion_args[j][1] = number_modified_elements[j];
            insertion_args[j][2] = number_modified_stock[j];

            pthread_create ((j*sizeof(pthread_t) + inserters), NULL, (void*) inserter, &insertion_args[j]);
            j++;
        }

        // Transporter thread creation
        pthread_create (transporters, NULL, (void*) transporter, NULL);
        j = 0;

        // Receiver threads creation
        while (j < number_receivers){
            pthread_create ((j*sizeof(pthread_t) + receivers), NULL, (void*) receiver, NULL);
            j++;
        }

        return 0;
    }

    else {
        return -1;
    }
}



/* Function that closes and free the resources used by the factory */
int close_factory(){

    int i = 0;
    int error = 0;

    // Close the database
    error = db_factory_close();
    if (error != 0){
        perror("Error when closing the database\n");
    }


    /* FINALIZATION OF THE THREADS */

    // Closing inserters thread
    while (i < number_inserters){
        pthread_join ((pthread_t)(i*sizeof(pthread_t) + inserters), NULL);
        i++;
    }

    // Closing transporter thread
    pthread_join ((pthread_t)transporters, NULL);

    i = 0;
    // Closing receivers thread
    while (i < number_receivers){
        pthread_join ((pthread_t)(i*sizeof(pthread_t) + receivers), NULL);
        i++;
    }

    return 0;
}



/* Function that inserts a new element to the database */
/* Function that updates the stock of the element of the database */
/* Function executed by the inserter thread which is in charge of inserting and updating */
void * inserter(void * data){

    /* To be completed for concurrency */
    int ID, stock;
    int i = 0;
    int error = 0;

    // Creation of the elements
    while (i < data[0]){
        error = db_factory_create_element("Element", 1, &ID);

        if (error != 0){
            perror("Error when creating the elements");
            pthread_exit(&error_number);
        }

        // If the "i" element needs to be updated, the stock passed as argument in "data[2]" is added
        if (i < data[1]){
            error = db_factory_get_stock(ID, &stock);

            if (error != 0){
                perror("Error when getting the stock of the elements");
                pthread_exit(&error_number);
            }

            error = db_factory_update_stock(ID, (stock + data[2]));

            if (error != 0){
                perror("Error when updating the stock of the elements");
                pthread_exit(&error_number);
            }
        }
    
        i++;
    }

    printf("Exitting inserter thread\n");
    pthread_exit(&correct_number);
}



/* Function executed by the thread transporter */
void * transporter(void){

  int status, stock;
  char * name;
  int ID = 0;
  int error = 0;
  int position = 0;
  int transported_elements = 0;

  while (transported_elements < total_number){
      error = db_get_ready_state(ID, &status);

      if (error != 0){
          perror("Error when checking the state of the elements");
          pthread_exit(&error_number);
      }

      if (status == 1){
          error = db_factory_get_stock(ID, &stock);

          if (error != 0){
              perror("Error when getting the stock of the elements");
              pthread_exit(&error_number);
          }

          if (stock > 0){
              error = db_factory_get_element_name(ID, name);

              if (error != 0){
                  perror("Error when getting the name of the elements");
                  pthread_exit(&error_number);
              }

              if (belt_elements < 7){


                  /* HERE THE TRANSPORTER HAS TO WAIT FOR THE RECEIVER THREAD */

                  ID = belt[position].id;
                  name = belt[position].name;

                  error = db_factory_update_stock(ID, (stock -1));

                  if (error != 0){
                      perror("Error when updating the stock of the elements");
                      pthread_exit(&error_number);
                  }

                  // To be printed when inserted in the belt
                  printf("Introducing element %d, %s in position [%d] with %d number of elements\n", ID, name, position, belt_elements);
                  printf("Exitting thread transporter\n");

                  belt_elements++;
                  transported_elements++;
                  position = (position+1) % MAX_BELT;
              }
          }
      }

      ID = (ID+1) % MAX_DATABASE;
  }

  pthread_exit(&correct_number);
}



/* Function execute by the receiver thread */
void * receiver(){

  char * name;
  int error = 0;
  int position = 0;
  int received_elements = 0;

  while (received_elements < total_number){

      error = db_factory_get_element_name(belt[position].id, name);

      if (error != 0){
          perror("Error when getting the name of the elements");
          pthread_exit(&error_number);
      }

      // To be printed when an element is received
      printf("Element %d, %s has been received from position [%d] with %d number of elements\n", belt[position].id, name, position, belt_elements);
      printf("Exitting thread receiver\n");

      belt_elements--;
      received_elements++;
      position = (position+1) % 8;
  }

  pthread_exit(&correct_number);
}