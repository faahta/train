#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<semaphore.h>
#include<stdbool.h>

enum DIR{CW, CCW};
enum STAT{FREE, OCCUPIED};

typedef struct Station{
	int sid;
	sem_t *sem[2];
}Station;

typedef struct ConnectionTrack{
	int src_station;
	int dst_station;
	enum STAT status;
	sem_t *sem_track;
	
}ConnectionTrack;

typedef struct Train{
	int tid;
	int station;
	enum DIR dir;
}Train;

Train *train;
Station *station;
ConnectionTrack *conn;
sem_t *S;
int nstations, ntrains;

static void * 
train_thread(void * argtr){

	pthread_detach(pthread_self());
	int *tr;
	tr = (int *)argtr;
	int tid = *tr;
	sleep(2);	
	//wait for start signal
	sem_wait(S);
	
	int i = 0;
	srand(time(NULL));
	while(1){
		printf("\n");
		printf("Train %d at station %d going   ",tid, train[tid].station);
		if(train[tid].dir == 0){
			printf("CLOCKWISE\n");
		}
		if(train[tid].dir == 1){
			printf("COUNTER CLOCKWISE\n");
		}
		//try to go to next station
		int next_station;
		//check if connection track to next station is occupied
		if(conn[train[tid].station].status != 1){
			next_station = conn[train[tid].station].dst_station;			
		} else{
			printf("WARNING: connection track ST%d<---->ST%d is occupied! waiting...\n", 			    	conn[train[tid].station].src_station,conn[train[tid].station].dst_station);
			//wait until it's released
			sem_wait(conn[train[tid].station].sem_track);
			printf("connection track ST%d<------>ST%d released\n", conn[train[tid].station].src_station,conn[train[tid].station].dst_station);
			next_station = conn[train[tid].station].dst_station;			
		}
		//move the train to next station
		if(train[tid].dir == 0){
			//check and wait if next station track is occupied
			sem_wait(station[tid].sem[0]);
			printf("Train %d traveling toward station %d CLOCKWISE\n",tid, next_station);		
			train[tid].station = next_station;
		}			
		if(train[tid].dir == 1){
			sem_wait(station[tid].sem[1]);
			printf("Train %d traveling toward station %d COUNTER CLOCKWISE\n",tid, next_station);
			train[tid].station = next_station;
		}
		
		printf("Train %d arrived at station %d\n", tid, train[tid].station);
		//occupy the track
		conn[train[tid].station].status = OCCUPIED;
		sleep(10);
		//release the track
		conn[train[tid].station].status = FREE;
		//notify other train threads waiting for the connection track(if any)
		sem_post(conn[train[tid].station].sem_track);
		
		//notify other train threads waiting for the station track heading clockwise
		if(train[tid].dir == 0)
			sem_post(station[tid].sem[0]);
		//notify other train threads waiting for the station track heading counter clockwise
		if(train[tid].dir == 1)
			sem_post(station[tid].sem[1]);
	}
	pthread_exit(NULL);
}

void printtracks(int src, int dst){
	printf("ST%d<------->ST%d\n", src, dst);
}
void 
init(){
	station = (Station *)malloc(nstations * sizeof(Station));
	train = (Train *) malloc(ntrains * sizeof(Train));
	conn = (ConnectionTrack *)malloc(nstations * sizeof(ConnectionTrack));	
	S = (sem_t *)malloc(sizeof(sem_t));
	sem_init(S, 0, 0);
}
void 
setup(){
	int i;
	//setup stations
	for(i=0; i<nstations; i++){
		station[i].sid = i;
		if(i != nstations - 1){
			conn[i].src_station = i;
			conn[i].dst_station = i+1;
		} else{
			//connect last station with first
			conn[i].src_station = i;
			conn[i].dst_station = 0;
		}	
		conn[i].sem_track = (sem_t *)malloc(sizeof(sem_t));	
		station[i].sem[0] = (sem_t *)malloc(sizeof(sem_t));
		station[i].sem[1] = (sem_t *)malloc(sizeof(sem_t));
		sem_init(station[i].sem[0], 0, 1);
		sem_init(station[i].sem[1], 0, 1);
		sem_init(conn[i].sem_track, 0, 0);
		printtracks(conn[i].src_station, conn[i].dst_station);
	}	
	//setup train
	srand(time(NULL));
	for(i=0; i<ntrains; i++){
		train[i].tid = i;	
		train[i].station = ((rand() % nstations) + 1);
		int r = ((rand() % 100)+1);
		if (r>=60){
			train[i].dir = CW;
		} else{
			train[i].dir = CCW;
		}
	}
}

int 
main(int argc, char *argv[]){

	pthread_t *th_tr;
	nstations = atoi(argv[1]);
	ntrains = atoi(argv[2]);
	if (ntrains >= nstations){
	  	fprintf (stderr, "ntrains >= nstations\n");
	  	return(1);
  	}
  	
	init();	
	setup();	
	
	th_tr = (pthread_t *)malloc(ntrains * sizeof(pthread_t));
	int *pi;
	int i;
	//create train treads
	for(i=0; i<ntrains; i++){
		pi = (int *)malloc(sizeof(int));
		*pi = i;
		pthread_create(&th_tr[i], NULL, train_thread, (void *)pi);
	}
	//start train threads
	for(i=0; i<ntrains; i++)
		sem_post(S);
			
	pthread_exit((void *)pthread_self());

}
