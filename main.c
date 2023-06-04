// file c
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/wait.h>
typedef int pipe_t[2];
typedef struct 
{
    int pidNipote ; /* contiene il pid del nipote*/
    char linea[250]; /* contiene ultima linea scritta dal nipote*/
    int lunghezza ; /* contiene lunghezza linea senza terminatore*/

}S_struct;
int main( int argc , char **argv)
{
/*----------------------------*/
int pid ; /* variabile per fork*/
int pidFiglio , status , ritorno; /* variabili per wait*/
int fd ; /*file descriptor*/
int q ; /* per indice processi figli, richiesat dal testo*/
S_struct s ;/* struct usata da figli e padre , richiesat dal testo*/
int i ; /* variabile per ciclo*/
int Q; /* indica numero processi figli , richiesta dal testo*/
pipe_t *piped ; /* array dinamico di pipe per comunicazione tra padre e figlio*/
pipe_t p ; /* pipe per comunicazione tra figlio e nipote*/
char BUFFER[250]; /* variabile che usa il figlio per leggere linea , richiesat dal testo*/
int j ; /* variabile usata nella lettuar della pipe da parte del figlio*/
int ret; /* variabile per conatnere valore di ritorno del figlio*/
int nw ; /* variabile per controllare a write*/
/*-----------------------------*/
/* controllo numero parametri*/
if(argc < 3)
{
printf("Errore nel numero di parametri passati\n");
exit(1);
}
Q= argc -1;
/* creo le pipe*/
piped=(pipe_t*)malloc(Q*sizeof(pipe_t));
if(piped==NULL)
{
    printf("Errore nella allocazione di memoria\n");
    exit(2);
}
for(i = 0 ; i < Q ; i++)
{
    if(pipe(piped[i]) < 0)
    {
        printf("Errore nella creazione della pipe %d-esima\n" , i);
        exit(3);
    }
}
/*processo padre crea Q processi figli , se processo figlio fallisce ritorna -1 , cioe 255 ,
infatti una linea per specifica del testo puo essree lunga al massimo 250 compreso terminatore*/
for(q=0 ; q < Q ; q++)
{
    if((pid=fork()) < 0)
    {
        printf("Errore nella fork\n");
        exit(4);
    }
    if(pid==0)
    {
        /* setto le pipe per comunicazione con padre*/
        for(i = 0 ; i < Q ; i++)
        {
            close(piped[i][0]);/* chiudo tute quelle in lettura , figlio deve solo scrivere*/
            if(i != q )
            close(piped[i][1]); /* chiudo quelle in scrittura , figlio scrvie solo nella sua*/
        }
        /* creo la pipe per comunicazione con nipote*/
        if(pipe(p) < 0)
        {
            printf("Errore nella creazione della pipe per comunicazione tra figlio di indice %d e nipote\n" ,q);
            exit(-1);
        }
        /* il file e utilizzato solo dal nipote quindi lo apro in esso*/
            if((pid=fork()) < 0)
            {
                printf("Errore nella fork del nipote associato al figlio di indice %d\n" ,q);
                exit(-1);
            }
            if(pid==0)
            {
                /* devo aprire il file associato*/
                fd=open(argv[q+1], O_RDONLY); 
                if(fd < 0)
                {
                    printf("Errore nella apertura del file %s\n" , argv[q+1]);
                    exit(-1);
                }
                /* chiudo pipe che non uso*/
                close(piped[q][1]);
                /* ora chiudo stdout e apro al suo posto la pipe p*/
                close(1); /* chius stdout*/
                dup(p[1]); /* duplico la p[1] al posto di stdout*/
                close(p[0]); /* chiudo partein lettura*/
                close(p[1]);/* chiudo parte in scrittura che tanto ho duplicato*/
                execlp("rev", "rev" ,argv[q+1] , (char *)0);

                exit(-1); /* se siamo qui la exec e fallita , quindi faccio ritornare -1 al nipote*/

            }/* fine codice nipote*/
        /* siamo di nuov nel figlio*/
        /* figlio apsetta  i nipoti*/
        if((pid=wait(&status)) < 0)
        {
            printf("Errore nella wait del procsso nipote generato dal figlio di indice %d\n" , q);
        }
        if((status & 0xFF) != 0)
        {
            printf("Processo nipote  associato al figlio di inidce %d  e di pid %d terminato in modo anomalo\n" ,q , pid);
        }
        else
        {
            ritorno=(int)((status >> 8) & 0xFF);
            if(ritorno == 0)
            {   /* stampa non richiesta dal testo*/
               printf("DEBUG-OK. Processo nipote di pid %d associato al figlio di indice %d è terminato ed ha esaguito la rev correttamente\n" , pid , q);

            }
            else
            {   /* stampa non richiesta dal testp*/
                printf("Processo nipote di pid %d associato al figlio di indice %d ha fallito la rev\n" , pid , q);
            }
        
        close(p[1]); /* chiudo la pipe in scrittura*/
        j = 0; /* azzero il contatore*/
        s.pidNipote=pid; /*inserisco valore del pid del nipote */
        while(read(p[0] ,&BUFFER[j],1))
        {
            if(BUFFER[j]=='\n')
            {
                
                s.lunghezza=j;
                j = 0 ; /* riazero contatore per prossima linea*/
            }
            else
            {
                j++;
            }
        }
        for(i = 0 ; i < s.lunghezza; i++)
        {
            s.linea[i]=BUFFER[i]; /* insreisco la linea nella struct*/
        }
       nw =  write(piped[q][1], &s , sizeof(S_struct)); /* invio la struct al padre*/
        if(nw != sizeof(S_struct))
        {
            /* contorllo per sicurezza*/
            printf("Errore nella scrittura della pipe del figli di inidce %d\n" , q);
            exit(-1);
        }
        ret = s.lunghezza + 1; /* conto anche il terminatore*/
   
        }
    exit(ret);
    }/* fine codice figlio*/
}/* fine cilo for di generazione figli*/

/* codice padre*/
/* setto le pipe*/
for(i = 0 ; i< Q ; i++)
{
    close(piped[i][1]); /* chiuto tutte quelle in scrittura*/
}
/* ora padre deve leggere la pipe*/
for(i = 0 ; i < Q ; i++)
{
    if(read(piped[i][0] , &s , sizeof(S_struct)) != 0)
    {
        
        s.linea[s.lunghezza]= '\0'; /* padre trasforma in stringa*/
        printf("L'ultima linea rovesciata prodotta dal nipote di pid %d dal file %s e di lunghezza %d (non compreso il terminatore) è : %s \n" ,s.pidNipote , argv[i+1] ,s.lunghezza , s.linea);
    }
}
/* padre aspetta i figli*/
for(q = 0 ; q < Q ;q++)
{
    if((pidFiglio=wait(&status)) < 0)
    {
        printf("Errore nella wait\n");
    }
    if((status & 0xFF) != 0)
    {
        printf("processo filgio di pid %d terminato in modo anomalo \n" , pidFiglio);
    }
    else
    {
        ritorno=(int)((status >> 8) & 0xFF);
        printf("processo figlio di pid %d ha ritornato %d (se 255 problemi)\n" , pidFiglio , ritorno);
    }
}
exit(0);
}/* fine del main*/