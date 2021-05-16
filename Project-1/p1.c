#include "arxeio.h"


int main(){

   key_t key;
   int shmid;
   char *buffer;     /* logical base address for buffer */
   int i,j;

   /* Read from input */
   pid_t pid;          /* variable to record process id of child */

   char *shared_memory;   /* shared memory base address */
  
   /* semaphore elements */
   int semid;                 /* identifier for a semaphore set */

   /* Message after CHAN */
   char new_message[MAXSIZE];

   /* Old checksum and new checksum for comparison */
   char old_checksum[MAXSIZE];
   char *new_checksum;
   
   char input[MAXSIZE];         /* User's text */
   char checksum[MAXSIZE];      /* input's checksum */
   char text[MAXSIZE];
   char final_message[MAXSIZE];       /* The final message to display */
  
   memset(text,'\0',sizeof(text));
   key = 5678;

   if ((shmid = shmget(key, SHARED_MEM_SIZE, IPC_CREAT | 0666)) < 0){
      perror("error shmget\n");
      exit (1);
   }

   if ((shared_memory = shmat(shmid, NULL, 0)) == (char *) -1){
    perror("error shmat\n");
    exit (1);
   }

   /* Clear the shared memory*/
   memset(shared_memory,'\0',SHARED_MEM_SIZE);

   /* set up pointers to appropriate places in shared memory segment */
   buffer =  shared_memory; /* logical buffer starts at shared segment */

   /* Initialize Everything with 0 */
   buffer[ENDP1] = 0;
   buffer[ENDENC1] = 0;
   buffer[ENDP2] = 0;
   buffer[ENDENC2] = 0;

   /* First message will always be from P1 -> P2  =>  ROI = 0 */
   buffer[ROI] = 0;

   /* Create and initialize semaphores */
   semid = sem_create(5);
   sem_init(semid, sem1, 0);   // p1 => enc1
   sem_init(semid, sem2, 0);   // enc1 => chan
   sem_init(semid, sem3, 0);   // chan => enc2
   sem_init(semid, sem4, 0);   // enc2 => p2
   sem_init(semid, sem0, 1);   // enc1 => p1.

    /* fork() to create another process */
    if (-1 == (pid = fork())){ /* spawn child process */
        perror ("error in fork");
        exit (1);
    }

    if (0 == pid){

        /* -------------------------- ENC1 ---------------------------- */
        while(1){

          P(semid, sem1);     /* wait for P1 (ROI == 0) or CHAN (ROI == 1) to finish */

          if (buffer[ROI] == 0){

            if (buffer[ENDENC1] == 1){
              buffer[ENDCHAN] = 1;
              V(semid, sem2);            /* signal semaphore for space available */
              printf("ENC1 ended.\n");
              exit(0);
            }

            memset(text,'\0',sizeof(text));     /* Clear checksum */
            memset(checksum,'\0',sizeof(checksum));     /* Clear checksum */

            strcpy(text, buffer);
            
            /* MP5 algorithm to create checksum*/
            char hash[MD5_DIGEST_LENGTH];
            MD5 (text, sizeof(text), hash);
            strcpy(checksum,(char*)hash);
            checksum[MD5_DIGEST_LENGTH] = '\0';

            i = 0;
            j = strlen(text) + 1;

              /* Put '\0' character between input and chechsum , input + '\0' + checksum */
              while (checksum[i] != '\0'){
                    text[j] = checksum[i];
                    i++;
                    j++;
              }

              /* Copy value 2 to checksum */
              i = BUFFER2;
              j = 0;
              int flag = 0;

              /* Break the second time you find '\0' */
              while ((text[j] != '\0') || (flag == 0)){
                
                buffer[i] = text[j];
                
                if (text[j] == '\0')
                  flag = 1;
                
                i++;
                j++;
              }
            
              V(semid, sem2);            /* signal semaphore for space available */

          /* If ROI == 1*/
          }else{

            if (buffer[ENDENC1] == 1){
              buffer[ENDP1] = 1;
              printf("ENC1 ended.\n");
              V(semid, sem0);
              exit(0);
            }

            i = BUFFER2;
            j = 0;

            /* Clear the new array */
            memset(new_message,'\0',sizeof(new_message));
            memset(old_checksum,'\0',sizeof(old_checksum));
        
            /* Get the message from changed_message + checksum (until the first '\0')*/
            while (buffer[i] != '\0'){
              new_message[j] = buffer[i];
              i++;
              j++;
            }

            new_message[j] = '\0';
            
            i++;
            j = 0;

            /* Get the checksum from changed_message + checksum (until the second '\0')*/
            while (buffer[i] != '\0'){
              old_checksum[j] = buffer[i];
              i++;
              j++;
            }

            old_checksum[j] = '\0';
           
            /* MP5 algorithm to create the new_messages's new_checksum */
            char hash[MD5_DIGEST_LENGTH];
            MD5 (new_message, sizeof(new_message), hash);
            new_checksum = MD5(new_message, sizeof(new_message), NULL);
            /* ------------------------------ */

            strcat(new_checksum,"\0");

            /* If they are the same send input in P1 for printing */
            if (strcmp(old_checksum, new_checksum) == 0){

              i = BUFFER1;
              j = 0;
              while (new_message[j] != '\0'){
                  buffer[i] = new_message[j];
                  i++;
                  j++; 
              }
             
              V(semid, sem0);                   /* Start P1 to print the message */

            /* Message changed -> resend it */
            }else{
              printf("Send the message from ENC2 again. \n");
              V(semid,sem3);                /* Start ENC2 again to resend the message */
            }
          }
        }

    }else{

        /* -------------------------- P1 ---------------------------- */
        while(1){

          P(semid, sem0);

          if (buffer[ROI] == 0){
    
            /* Read input from the user */
            printf("\nEnter the message P1 -> P2: ");
            fgets(input, MAXSIZE, stdin);
            fflush(stdin);

            /* If input == "TERM" P1 sends the message to ENC1 and then ends */
            if (strcmp(input, "TERM\n") == 0){
              buffer[ENDENC1] = 1;
              V(semid, sem1);           /* Start ENC1 */
              printf("P1 ended.\n");
              exit(0);
            }

            /* Remove new line character '\n' from the end of the string*/
            input[strlen(input) - 1] = '\0';

            strcpy (buffer, input);
            strcpy (input, "");         /* Clear the input to use it again */

            printf("\nWaiting for response from P2... \n");

            V(semid, sem1);   /* Start ENC1 */

          /* If ROI == 1 */
          }else{

             /* If message is TERM clear shared memory and exit */
             if (buffer[ENDP1] == 1){

                printf("The message you entered from TERM\n");
                printf("P1 ended.\n");

                /*  Clear Shared Memory */
                shmdt(shared_memory);
                shmctl(shmid,IPC_RMID,NULL);

                /* Clear semaphores */
                if (semctl (semid, 0, IPC_RMID) < 0){   
                    perror ("error in removing semaphore from the system");
                    exit (1);
                }
                exit(0);
              }      

              /* Print the original message (from P2)  */
              strcpy(final_message, buffer + BUFFER1);
              printf("\nThe message you entered from P1: %s \n", final_message);

              /* Send message from P2 -> P1 */
              buffer[ROI] = 0;

              /* Clear all buffers (not the flags) */
              memset(shared_memory,'\0', ROI - 1);

              V(semid, sem0);            /* Start P1 again, this time to read the new messgae */
          }
        }
      }

  exit(0);
}

/* --------------------  Semaphore Functions --------------------- */

int sem_create(int num_semaphores)
{  /* procedure to create specified number of semaphores */
   int semid;

   /* create new semaphore set of semaphores */
   if ((semid = semget (1234, num_semaphores, IPC_CREAT | 0600)) < 0)
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