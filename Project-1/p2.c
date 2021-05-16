#include "arxeio.h"


int main(){

    int shmid;
    key_t key;
    char *buffer;          /* logical base address for buffer */
    char input[MAXSIZE];     /* User's text */   
    char new_message[MAXSIZE];      /* Message after CHAN */
    
    /* Old checksum and new checksum for comparison */
    char old_checksum[MAXSIZE];
    char *new_checksum;

    char checksum[MAXSIZE];         /* input's checksum */
    char final_message[MAXSIZE];       /* The final message to display */
    char text[MAXSIZE];
    int i,j;

    /* Read from input */
    pid_t pid;          /* variable to record process id of child */

    /* semaphore elements */
    int semid;       /* identifier for a semaphore set */

    char *shared_memory;   /* shared memory base address */

    key = 5678;

    if ((shmid = shmget(key, SHARED_MEM_SIZE, 0666)) < 0){
      perror("Error in shmget");
      exit(1);
    }

    if ((shared_memory = shmat(shmid, NULL, 0)) == (char *) -1){
      perror("Error in shmat");
      exit(1);
    }
    
    buffer =  shared_memory; 
    semid = sem_create(5);

     /* fork() to create another process */
     if (-1 == (pid = fork())){ /* spawn child process */
         perror ("error in fork");
         exit (1);
     }

     if (0 == pid){

         /* -------------------------- P2 ---------------------------- */

         while(1){

            P(semid, sem4);    
         
            /* From P1 -> P2 P2: prints message */
            if (buffer[ROI] == 0){

               /* If message is TERM clear shared memory and exit */
               if (buffer[ENDP2] == 1){
                  printf("The message you entered from P2: TERM\n");
                  printf("P2 ended.\n");

                  /* Clear Shared Memory */
                  shmdt(shared_memory);
                  shmctl(shmid,IPC_RMID,NULL);

                  /* Clear semaphores */
                  if (semctl (semid, 0, IPC_RMID) < 0){   
                     perror ("error in removing semaphore from the system");
                     exit (1);
                  }
                  exit(0);
               }           
                
               strcpy(final_message, buffer + BUFFER4);
               printf("\nThe message you entered from P2: %s \n", final_message);

               /* Send message from P2 -> P1 */
               buffer[ROI] = 1;

               /* Clear all buffers (not the flags) */
               memset(shared_memory,'\0', ROI - 1);

               V(semid, sem4);            /* Start P2 again to read the new message*/

            /* If ROI == 1 get input and send it to enc2 */
            }else{
      
               /* Read input from the user */
               printf("\nEnter the message P2 -> P1: ");
               fgets(input, BUFFER2, stdin);
               fflush(stdin);

               if (strcmp(input, "TERM\n") == 0){
                  buffer[ENDENC2] = 1;
                  V(semid, sem3);   /* Start ENC1 */
                  printf("P2 ended.\n");
                  exit(0);
              }

               /* Remove new line character from the end of the string*/
               input[strlen(input) - 1] = '\0';

               strcpy (buffer + BUFFER4, input);
               strcpy (input, "");                    /* Clear the input to use it again */

               printf("\nWaiting for response from P1... \n");

               V(semid, sem3);            /* ENC2 starts */
            }
         }
         
     }else{

         /* -------------------------- ENC2 ---------------------------- */

         while(1){

            P(semid, sem3);    /* wait for P2 (ROI == 1) or CHAN (ROI == 0) to finish */

            if (buffer[ROI] == 0){

               if (buffer[ENDENC2] == 1){
                  buffer[ENDP2] = 1;
                  printf("ENC2 ended.\n");
                  V(semid, sem4);
                  exit(0);
               }

               i = BUFFER3;
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
            
               /* MP5 algorithm to create new_checksum */
               char hash[MD5_DIGEST_LENGTH];
               MD5 (new_message, sizeof(new_message), hash);
               new_checksum = MD5(new_message, sizeof(new_message), NULL);
               strcat(new_checksum,"\0");

               /* If they are the same send message to P2, if not resend the message */
               if (strcmp(old_checksum, new_checksum) == 0){

                  i = BUFFER4;
                  j = 0;
                  while (new_message[j] != '\0'){
                     buffer[i] = new_message[j];
                     i++;
                     j++; 
                  }

                  V(semid, sem4); /* Message is OK send it to P2 to print */

               /* Resend the message (old_checksum != new_checksum) */
               }else{
                  printf("Send the message from ENC1 again. \n\n");
                  V(semid,sem1);       /* Start ENC1 again to resend the message */
               }

            /* If ROI == 1 ENC2 SENDS MESSAGE TO CHANNEL */
            }else{

               if (buffer[ENDENC2] == 1){
                  buffer[ENDCHAN] = 1;
                  V(semid, sem2);            /* signal semaphore for space available */
                  printf("ENC2 ended.\n");
                  exit(0);
               }

               memset(text,'\0',sizeof(text));     /* Clear checksum */
               strcpy(text, buffer + BUFFER4);

               /* MP5 algorithm to create checksum*/
               char hash[MD5_DIGEST_LENGTH];

               memset(hash,'\0',sizeof(hash));
               memset(checksum,'\0',sizeof(checksum));     /* Clear checksum */

               MD5 (text, sizeof(text), hash);
               strcpy(checksum,(char*)hash);

               checksum[MD5_DIGEST_LENGTH] = '\0'; 
               /* ------------------------------ */

               i = 0;
               j = strlen(text) + 1;

               /* Put '\0' character between input and chechsum , input + '\0' + checksum */
               while (checksum[i] != '\0'){
                     text[j] = checksum[i];
                     i++;
                     j++;
               }
               
               /* Copy value 2 to buffer between enc2 and channel */
               i = BUFFER3;
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
               buffer[i] = '\0';

               V(semid, sem2);            /* signal semaphore for space available */
            }
         }
      }

   exit(0);
}


/* ------------------------------------------------------------------------------------------- */

int sem_create(int num_semaphores)
{  /* procedure to create specified number of semaphores */
   int semid;

   /* create new semaphore set of semaphores */
   if ((semid = semget (1234, num_semaphores, 0600)) < 0)
     {  perror ("error in creating semaphore"); /* 0600 = read/alter by user */
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