#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/md5.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define sem1 0   
#define sem2 1    
#define sem3 2      
#define sem4 3 
#define sem0 4

#define SHARED_MEM_SIZE 900         /* size of shared memory */

#define BUFFER1 0                     /* Buffer between P1 <-> ENC1 [0 - 200] */
#define BUFFER2 200                /* Buffer between ENC1 <-> CHAN [200 - 400] */
#define BUFFER3 400               /*  Buffer between CHAN <-> ENC2 [400 - 600] */
#define BUFFER4 600              /* Buffer P2 <-> ENC2 [600 - 800] */

#define ROI 810         /* Flag to check if P1 send to P2 or P2 sends to P1 */

#define ENDP1 820
#define ENDENC1 830
#define ENDCHAN 840
#define ENDENC2 850
#define ENDP2 860

#define MAXSIZE 200  /* Every buffer's size */

int sem_create(int num_semaphores);
void sem_init(int semid, int index, int value);
void P(int semid, int index);
void V(int semid, int index);
