
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */
#include <iostream>
#include <fstream>
using namespace std;

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr) {
  ofstream myfile;
  myfile.open ("keyfile.txt");
  for(int q = 0; q < 100; q++) {
    myfile << "Hello world\n";
  }
  myfile.close();
  //create key
  key_t key = ftok("keyfile.txt",'a');
  shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666|IPC_CREAT);
  if (shmid < 0) {
    cout << "error\n";
    exit(1);
  }
  sharedMemPtr = shmat(shmid, (void*)0 ,0);
  msqid = msgget(key, IPC_CREAT|0666);
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr) {
  shmdt(sharedMemPtr);
}

/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName) {
	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");

	/* A buffer to store message we will send to the receiver. */
	message sndMsg;

	/* A buffer to store message received from the receiver. */
	message rcvMsg;

	/* Was the file open? */
	if(!fp) {
		perror("fopen");
		exit(-1);
	}

	/* Read the whole file */
	while(!feof(fp)) {
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
 		 * fread will return how many bytes it has actually read (since the last chunk may be less
 		 * than SHARED_MEMORY_CHUNK_SIZE).
 		 */
		if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0) {
			perror("fread");
			exit(-1);
		}
		cout << "sent message size = " << sndMsg.size << " bytes.\n";
		sndMsg.mtype = SENDER_DATA_TYPE;
		int w = msgsnd(msqid, &sndMsg,sizeof(sndMsg),0);
		if(w < 0) {
		  cout << "message failed to send\n";
		}
	        if(msgrcv(msqid, &rcvMsg, sizeof(rcvMsg), RECV_DONE_TYPE, 0) < 0 && (sndMsg.size != 0)) {
			 cout << "error receiving message from Receiver\n";
		 } else if(rcvMsg.mtype == RECV_DONE_TYPE && (sndMsg.size!=0)) {
			 cout << "received message from receiver, keep sending...\n";
		 }
	}
	if(sndMsg.size!=0) {
	  sndMsg.mtype = SENDER_DATA_TYPE;
	  sndMsg.size = 0;
	  int d = msgsnd(msqid, &sndMsg, sizeof(rcvMsg),0);
	  if(d < 0) {
	    cout << "send size 0 fail\n";
	  } else {
	    cout << "sent 0 to receiver\n";
	  }
        }

	/* Close the file */
	fclose(fp);
}

int main(int argc, char** argv) {

	/* Check the command line arguments */
	if(argc < 2) {
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

	/* Send the file */
	send(argv[1]);

	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);
	return 0;
}
