#include <stdio.h> 
#include <pthread.h> 
#include <semaphore.h> 
#include <unistd.h>
#include <stdlib.h>

// global semaphores to be used in the threads
sem_t toilet_sem;
sem_t man_sem, woman_sem; 
sem_t man_sem_counter, woman_sem_counter;
sem_t queue;

int men_count, women_count;
// a struct containing the parameters which will be passed to the function
typedef struct parm
{
	int id;
	int num;
	int itr;
}parm;

// this function simulates what happens when a man enters the queue
void man_enters(int i){
	sem_wait(&queue); 	// lock the queue semaphore in order to give this thread a priority while waiting
		sem_wait(&man_sem); 	// lock the man semaphore in order to be able to increment and check for equality without any race condition
			if(men_count == 0) sem_wait(&toilet_sem); 	// lock the bathroom represnting that it is used by men. if this fails, wait untill women leave
			men_count++; 
			printf("Man #%d wants to in the Bathroom\n",i);

		sem_post(&man_sem); 	// unlock man semaphore to allow others to run the critical section
	sem_post(&queue); 	// unlock the queue representing that a new thread can be the head of the queue
	sem_wait(&man_sem_counter); 	// decrease the counting semaphore to allow only 3 threads to in the bathroom
}

// this function simulates what happen when a man leaves the bathroom
void man_leaves(int i){
	sem_post(&man_sem_counter); 	// increment the counting semaphore to allow any new male thread to enter the bathroom

	sem_wait(&man_sem); 	// lock the man semaphore to secure the decrement and equality operation from any race conditions
		men_count--;
		printf("Man #%d left the Bathroom\n",i);

		if(men_count == 0) sem_post(&toilet_sem); 	// free the bathroom from men and allow the other gender to enter if applicable 

	sem_post(&man_sem); 
}

// the thread function for the number of threads representing the men
void* male_thread(void* arg) 
{ 
    parm * thread_arg = arg; 	// convert the void pointer to a param pointer to get the arguments
    int id = thread_arg->id;
	int itr = thread_arg->itr;
    for(int i=0; i<itr; i++){ 	// loop for the number of times passed by the user
    	man_enters(id);		// call the entering function
		printf("Man #%d is in the Bathroom for time number #%d\n",id,i+1); 
    	usleep(250); 	// sleep for 250 microsecond
    	man_leaves(id);		// call the leaving function
    }
	pthread_exit(0);
} 

// this function simulates what happens when a woman enters the queue
void woman_enters(int i){
	sem_wait(&queue);	// lock the queue semaphore in order to give this thread a priority while waiting
		sem_wait(&woman_sem);	// lock the woman semaphore in order to be able to increment and check for equality without any race condition
			if(women_count == 0) sem_wait(&toilet_sem);// lock the bathroom represnting that it is used by women. if this fails, wait untill men leave
			women_count++;
			printf("Woman #%d wants to enter the Bathroom\n",i);
		sem_post(&woman_sem);	// unlock woman semaphore to allow others to run the critical section
	sem_post(&queue);	// unlock the queue representing that a new thread can be the head of the queue
	sem_wait(&woman_sem_counter);	// decrease the counting semaphore to allow only 3 threads to in the bathroom
}

// this function simulates what happen when a woman leaves the bathroom
void woman_leaves(int i){ //as always
	sem_post(&woman_sem_counter);// increment the counting semaphore to allow any new female thread to enter the bathroom
	sem_wait(&woman_sem);// lock the womman semaphore to secure the decrement and equality operation from any race conditions
		women_count--;
		printf("Woman #%d left the Bathroom\n",i);
		if(women_count == 0) sem_post(&toilet_sem);// free the bathroom from women and allow the other gender to enter if applicable 
	sem_post(&woman_sem);
}

// the thread function for the number of threads representing the women
void* female_thread(void* arg) 
{ 
	parm * thread_arg = arg;// convert the void pointer to a param pointer to get the arguments
    int id = thread_arg->id;
	int itr = thread_arg->itr;
    for(int i=0; i<itr; i++){// loop for the number of times passed by the user
    	woman_enters(id); // call the entering function
    	printf("Woman #%d is in the Bathroom for time number #%d\n",id,i+1); 
    	usleep(500); // sleep for 500 microsecond
    	woman_leaves(id); // call the leaving function
    }
	pthread_exit(0);
} 

// this function launches the male threads
void* male_execution(void *arg){
	pthread_t  *men;	// array of threads for each man
	parm * thread_arg = arg;	// convert the void pointer to a stuct pointer
	int num_men = thread_arg->num;
	parm * men_thread_paramaters; // array of structs for each thread

	// allocate memory for the arrays
	men_thread_paramaters = (parm *) malloc(sizeof(parm)*num_men );	
	men = (pthread_t*)malloc(sizeof(pthread_t)*num_men); 
	int ret;
	for(int i = 0; i < num_men; i++){
		men_thread_paramaters[i].id = i+1;
		men_thread_paramaters[i].itr = thread_arg->itr;
		// launch the ith thread
        ret = pthread_create(&men[i], NULL, male_thread, (void *) &men_thread_paramaters[i]);
		if(ret!=0){
			printf("Creating Male Thread failed");
			pthread_exit(0);
		}
	}
	for(int i =0; i < num_men; i++){
		pthread_join(men[i], NULL);
	}
	//morgan
	free(men); // :V
	free(men_thread_paramaters);
	pthread_exit(0);
}

// this function launches the women threads
void* female_execution(void *arg){
	pthread_t  *women;	// array of threads for each woman
	parm * thread_arg = arg;	// convert the void pointer to a stuct pointer
	int num_women = thread_arg->num;
	parm * women_thread_paramaters;		// array of structs for each thread

	// allocate memory for the arrays
	women_thread_paramaters = (parm *) malloc(sizeof(parm)*num_women );
	women = (pthread_t*)malloc(sizeof(pthread_t)*num_women); 

	int ret;
	int err = -1;
	for(int i = 0; i < num_women; i++){
		women_thread_paramaters[i].id = i+1;
		women_thread_paramaters[i].itr = thread_arg->itr;
		// launch the ith thread
		ret = pthread_create(&women[i], NULL, female_thread, (void *) &women_thread_paramaters[i]);
		if(ret!=0){
			printf("Creating Female Thread failed");

			pthread_exit(0);
		}
	}
	for(int i =0; i < num_women; i++){
		pthread_join(women[i], NULL);
	}
	free(women); 
	free(women_thread_paramaters);
	pthread_exit(0);
}

int main() {
	// initialize the parameter structure
	parm * men_parameters, *women_parameters;
	men_parameters = (parm *) malloc(sizeof(parm));
	women_parameters = (parm *) malloc(sizeof(parm));

	// initialize the semaphores
	sem_init(&toilet_sem, 0, 1); 
	sem_init(&man_sem, 0, 1); 
	sem_init(&woman_sem, 0, 1); 
	sem_init(&man_sem_counter, 0, 3); 
	sem_init(&woman_sem_counter, 0, 3); 
	sem_init(&queue, 0, 1); 
	
	// get input from the user
	printf("Enter the number of Men: ");
	scanf("%d",&men_parameters->num);
	printf("Enter the number of Women: ");
	scanf("%d", &women_parameters->num);
	printf("Enter the number of times Men enter the bathroom: ");
	scanf("%d",&men_parameters->itr);
	printf("Enter the number of times Women enter the bathroom: ");
	scanf("%d", &women_parameters->itr);
	
	// launch a thread for males and a thread for females
	// this will provide more concurrency than launching them serially in the main
	pthread_t males, females;
	int ret;
	ret = pthread_create(&females, NULL, female_execution, (void *) women_parameters);
	if(ret!=0){
		printf("Creating Male container Thread failed");
		return 0;
	}
	ret = pthread_create(&males, NULL, male_execution,(void *) men_parameters);
	if(ret!=0){
		printf("Creating Female container Thread failed");
		return 0;
	}
	
	// wait for the threads to finish excuting
	pthread_join(females,NULL);
	pthread_join(males,NULL);
	
	// delete the semaphores
	sem_destroy(&toilet_sem); 
	sem_destroy(&man_sem);
	sem_destroy(&woman_sem);
	sem_destroy(&man_sem_counter);
	sem_destroy(&woman_sem_counter);
	sem_destroy(&queue);

	return 0; 
} 
