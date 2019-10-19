#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "msg.h"    /* For the message struct */
using namespace std;

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr) {
	key_t key = ftok("keyfile.txt",'a');
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666|IPC_CREAT);
	sharedMemPtr = shmat(shmid, (void*)0, 0);
	msqid = msgget(key, IPC_CREAT|0666);
}

// The main loop
void mainLoop() {
	/* The size of the mesage */
	int msgSize = 0;

	/* Open the file for writing */
	FILE* fp = fopen(recvFileName, "w");

	/* Error checks */
	if(!fp) {
		perror("fopen");
		exit(-1);
	}
	message rcvMsg;
	int k = msgrcv(msqid, &rcvMsg, sizeof(rcvMsg), SENDER_DATA_TYPE, 0);
	msgSize = rcvMsg.size;
	cout << "msgSize = " << msgSize << endl;

	/* Keep receiving until the sender set the size to 0, indicating that
 	 * there is no more data to send
 	 */

	while(msgSize != 0) {
		/* If the sender is not telling us that we are done, then get to work */
		if(msgSize != 0) {
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0) {
				perror("fwrite");
			}
			rcvMsg.mtype = RECV_DONE_TYPE;
			int m = msgsnd(msqid, &rcvMsg,sizeof(rcvMsg) ,0);
			if(m < 0) {
			     	cout << "Message Send FAILED\n";
			}
			int p  = msgrcv(msqid, &rcvMsg, sizeof(rcvMsg), SENDER_DATA_TYPE, 0);
			if(p < 0) {
			        cout << "Message Receive FAILED\n";
			}
			msgSize = rcvMsg.size;
			cout << "msgSize = " << msgSize << endl;
		} else {
     			fclose(fp);
		}
	}
}

/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr) {
  shmdt(sharedMemPtr);
  shmctl(shmid,IPC_RMID,NULL);
  msgctl(msqid, IPC_RMID, NULL);
  cout << "Cleaned Up\n";
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */

void ctrlCSignal(int signal) {
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv) {
  cout << "Press CTRL + C to cleanup and exit\n";
  signal(SIGINT,ctrlCSignal);

  /*Initialize*/
  init(shmid, msqid, sharedMemPtr);

  /* Go to the main loop */
  mainLoop();
  cleanUp(shmid, msqid, sharedMemPtr);

  return 0;
}
