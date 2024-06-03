#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>

// Structure pour stocker le sémaphore
struct Semaphore {
    sem_t sem;
};

// Tableau pour stocker les PID des processus fils
pid_t fils[4];

// Pointeur vers la structure du sémaphore
struct Semaphore* semaphore;

// Gestionnaire de signal pour SIGUSR1
void gestionnaire_SIGUSR1(int signum) {
    printf("Processus fils %d : Signal SIGUSR1 reçu !\n", getpid());
    // Effectuez vos tâches ici...
    sleep(2); // Simulation d'une tâche complexe
    sem_post(&semaphore->sem); // Libération du sémaphore
}

int main() {
    // Création de la mémoire partagée
    int fd = shm_open("/mon_semaphore", O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("Erreur lors de la création de la mémoire partagée");
        exit(1);
    }
    if (ftruncate(fd, sizeof(struct Semaphore)) == -1) {
        perror("Erreur lors du redimensionnement de la mémoire partagée");
        exit(1);
    }
    semaphore = mmap(NULL, sizeof(struct Semaphore), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (semaphore == MAP_FAILED) {
        perror("Erreur lors de l'attachement de la mémoire partagée");
        exit(1);
    }
    if (sem_init(&semaphore->sem, 1, 0) == -1) {
        perror("Erreur lors de l'initialisation du sémaphore");
        exit(1);
    }

    // Création des processus fils
    for (int i = 0; i < 4; i++) {
        fils[i] = fork();
        if (fils[i] == 0) {
            // Processus fils
            signal(SIGUSR1, gestionnaire_SIGUSR1);
            // Attente du signal de départ
            sem_wait(&semaphore->sem);
            // Effectuez vos tâches ici...
            printf("Processus fils %d : Tâches terminées.\n", getpid());
            exit(0);
        }
    }

    // Attente pour laisser les processus fils s'initialiser
    sleep(2);

    // Envoi du signal SIGUSR1 à tous les processus fils
    for (int i = 0; i < 4; i++) {
        kill(fils[i], SIGUSR1);
    }

    // Attente de la fin des processus fils
    for (int i = 0; i < 4; i++) {
        wait(NULL);
    }

    // Fermeture de la mémoire partagée
    sem_destroy(&semaphore->sem);
    munmap(semaphore, sizeof(struct Semaphore));
    shm_unlink("/mon_semaphore");

    printf("Processus père : Tous les fils ont terminé.\n");
    return 0;
}
