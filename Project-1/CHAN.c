#include "arxeio.h"


int sem_create(int num_semaphores);
void sem_init(int semid, int index, int value);
void P(int semid, int index);
void V(int semid, int index);

int main(int argc, char *argv[]){

    int shmid;
    int propab;       /* The propability to change message characters */
    key_t key;
    char *shm;
    char *buffer;     /* logical base address for buffer */
    int i,j,x;
    int flag;
    int semid;

    char message[MAXSIZE]; 
    char checksumm[MAXSIZE];

    /* Get the propability percentage from program arguments */
    if (argc == 1)
      propab = 10;                /* The default propability if there are no arguments = 10% */

    else if (argc == 2){
      propab = atoi(argv[1]);
      if (!((propab >= 0) && (propab <= 100))){
        perror("Propability must be in range [0-100] \n");
        exit(1); }

    }else{
      perror("Too many arguments. \n");
      exit(1);
    }

    key = 5678;
    srand(time(0));

    if ((shmid = shmget(key, SHARED_MEM_SIZE, 0666)) < 0){
      perror("Error in shmget");
      exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1){
      perror("Error in shmat");
      exit(1);
    }

    buffer =  shm;          /* logical buffer starts at shared segment */
    semid = sem_create(5);

    while(1){

      P(semid,sem2);      /* Wait for ENC? to send a message */

      /* If message == "TERM" => end CHAN then go to ENC1 or ENC2 */
      if ((buffer[ENDCHAN] == 1)){

        printf("CHAN ended.\n");

        if (buffer[ROI] == 0){

          buffer[ENDENC2] = 1;
          V(semid,sem3);

        }else{

          buffer[ENDENC1] = 1;
          V(semid,sem1);
        }
        exit(0);
      }

      /* Clear the arrays */
      memset(checksumm,'\0',sizeof(checksumm));
      memset(message,'\0',sizeof(message));

      if (buffer[ROI] == 0)
        i = BUFFER2;            /* If R0I == 0 (p1 sends to p2) read message and checksum from BUFFER2 */
      else
        i = BUFFER3;             /* Else (p2 sends to p1) read message and checksum from BUFFER3 */
          
      j = 0;

      /* Get the message from message + checksum (until the first '\0')*/
      while (buffer[i] != '\0'){
        message[j] = buffer[i];
        i++;
        j++;
      }

      message[j] = '\0';
      printf("The message you entered from CHAN: %s \n", message);
      
      /* Change some characters with '*' */
      for(int y = 0; y < strlen(message); y++){
          x = rand() % 100;

          /* If the number x is in propab then replace character with '*' */
          if (x < propab)
            message[y] = '*';
      }

      i++;
      j = 0;

      /* Get the checksum from message + checksum (until the second '\0')*/
      while (buffer[i] != '\0'){
        checksumm[j] = buffer[i];
        i++;
        j++;
      }

      i = 0;
      j = strlen(message) + 1;
      
      /* Put '\0' character between changed message and checksum , changed_message + '\0' + checksum */
      while (checksumm[i] != '\0'){
            message[j] = checksumm[i];
            i++;
            j++;
      }

      if (buffer[ROI] == 0)
        i = BUFFER3;          /* If R0I == 0 (p1 sends to p2) save the new message and checksum to BUFFER3 */
      else
        i = BUFFER2;         /* Else (p2 sends to p1) save the new message and checksum to BUFFER2 */
        
      j = 0;
      flag = 0;       /* Flag = 1 the first time you find '\0' */

      /* Copy the changed message and the checksum in BUFFER */
      /* Break the second time you find '\0' */
      while ((message[j] != '\0') || (flag == 0)){
        
        buffer[i] = message[j];
          
        if (message[j] == '\0')
          flag = 1;
          
        i++;
        j++;
      }
    
      printf("The message you entered (after the change) CHAN: %s \n", message);
      printf(" ------------------------------- \n");

      if (buffer[ROI] == 0)
        V(semid,sem3);        /* If R0I == 0 (p1 sends to p2) go to ENC2 to check if the new message has changed */ 
      else
        V(semid,sem1);    /* Else (p2 sends to p1) go to ENC1 to check if the new message has changed */
      
    }
}

/* --------------------  Semaphore Functions --------------------- */

int sem_create(int num_semaphores)
{  /* procedure to create specified number of semaphores */
   int semid;

   /* create new semaphore set of semaphores */
   if ((semid = semget (1234, num_semaphores, 0600)) < 0)
     {  perror ("error in creating semaphore");/* 0600 = read/alter by user */
        exit (1);
     }
   return semid;
}

void sem_init(int semid, int index, int value)
{  /* procedure to initialize specified semaphore to given value */
   if (semctl (semid, index, SETVAL, value) < 0)
     {  perror ("error in initializing first semaphore");
        exit (1);
     }
}

void P(int semid, int index)
{/* procedure to perform a P or wait operation on a semaphore of given index */
  struct sembuf sops[1];  /* only one semaphore operation to be executed */

   sops[0].sem_num = index;/* define operation on semaphore with given index */
   sops[0].sem_op  = -1;   /* subtract 1 to value for P operation */
   sops[0].sem_flg = 0;    /* type "man semop" in shell window for details */

   if (semop (semid, sops, 1) == -1)
     {  perror ("error in semaphore operation");
        exit (1);
     }
}

void V(int semid, int index)
{/* procedure to perform a V or signal operation on semaphore of given index */
   struct sembuf sops[1];  /* define operation on semaphore with given index */

   sops[0].sem_num = index;/* define operation on semaphore with given index */
   sops[0].sem_op  = 1;    /* add 1 to value for V operation */
   sops[0].sem_flg = 0;    /* type "man semop" in shell window for details */

   if (semop (semid, sops, 1) == -1)
     {  perror ("error in semaphore operation");
        exit (1);
     }
}
