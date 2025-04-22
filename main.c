#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL_mixer.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define LEVEL_WIDTH 9000
#define GRAVITY 1
#define JUMP_STRENGTH -35
#define PLAYER_SPEED 3 
#define MAX_FALL_SPEED 5
#define CAMERA_SPEED 5

#define ENEMY_SPEED 3
#define ENEMY_FRAME_DELAY 8.5 

//3 tests unitaires: .txt(décrire le test) .c(le test)

//Plateformes
typedef struct {
    int x, y, width, height;
} Platform;



typedef enum {
    IDLE,
    WALK,
    RUN,
    JUMP,
    ATTACK,
    CROUCH
} PlayerState;

typedef struct {
    int x, y, width, height;
    int velocityY;
    bool onGround;
    bool facingRight;  // Pour savoir dans quelle direction il regarde
    PlayerState state;
    SDL_Texture **idleTextures;
    SDL_Texture **walkTextures;
    SDL_Texture **runTextures;
    SDL_Texture **jumpTextures;
    SDL_Texture **attackTextures;
    SDL_Texture **crouchTextures;
    int currentFrame;
    int frameCounter;
} Player;

//Liste musicale
const char *levelMusicFiles[] = {
    "../assets/sounds/test.wav",
    "../assets/sounds/misty_menace.ogg",
    "../assets/sounds/music_level3.ogg",
    "../assets/sounds/kremata.ogg",
    "../assets/sounds/kritter.death.wav" 
};



// Ennemi
typedef struct {
    int x, y, width, height;
    int velocityX, velocityY;
    SDL_Texture **textures;
    SDL_Texture **deathtextures;
    int currentFrame;
    int frameCounter;
    bool isDead;  // Nouveau champ pour l'état de l'ennemi (mort ou vivant)
    int deathFrame;  // Nouveau champ pour l'animation de la mort
    int deathDirection; 
} Enemy;


//Regarde les collisions entre le joueur et les plateformes
bool checkCollision(Player *player, Platform *platform) {
    return (player->x + player->width > platform->x &&
            player->x < platform->x + platform->width &&
            player->y + player->height > platform->y &&
            player->y < platform->y + platform->height);
}

// Fonction vérifiant la collision joueur-ennemi
bool checkEnemyCollision(Player *player, Enemy *enemy) {
    return (player->x + player->width > enemy->x &&
            player->x < enemy->x + enemy->width &&
            player->y + player->height > enemy->y &&
            player->y < enemy->y + enemy->height);
}
//Collisions kritters plateformes
bool checkCollisionEnemy(Enemy *enemy, Platform *platform) {
    return (enemy->x + enemy->width > platform->x &&
            enemy->x < platform->x + platform->width &&
            enemy->y + enemy->height > platform->y &&
            enemy->y + enemy->height + enemy->velocityX >= platform->y);
}


// Fonction vérifiant si le joueur saute sur l'ennemi
bool checkJumpOnEnemy(Player *player, Enemy *enemy) {
    // Le joueur doit être au-dessus de l'ennemi, et sa vitesse verticale doit être positive (il tombe)
    return (player->y < enemy->y && 
            player->y + player->height > enemy->y &&  // Le bas du joueur touche l'ennemi
            player->velocityY > 0);  // Le joueur tombe
}


//Collisions entre le joueur et les kritters
void handlePlayerEnemyCollision(Player *player, Enemy *enemies, int *enemyCount, Mix_Chunk *deathSound) {
    for (int i = 0; i < *enemyCount; i++) {
        if (enemies[i].isDead) {
            // Ignore l'ennemi si il est mort
            continue;
        }

        if (checkEnemyCollision(player, &enemies[i])) {
            // Si le joueur touche un ennemi par le côté, le joueur meurt (retour à une position de départ)
            if (!checkJumpOnEnemy(player, &enemies[i])) {
                printf("Le joueur est entré en collision avec un ennemi !\n");
                player->x = 100;  // Ramène le joueur à une position initiale
                player->y = 400;
                player->velocityY = 0;
            } else {
                // Si le joueur saute sur l'ennemi, on tue l'ennemi
                printf("L'ennemi a été tué en sautant dessus !\n");
                // Marquer l'ennemi comme mort
                enemies[i].isDead = true;
                enemies[i].deathFrame = 0;  // Réinitialiser l'animation de la mort
                
                // Déterminer la direction de la chute (bas-gauche ou bas-droit)
                if (player->x < enemies[i].x) { // Joueur à gauche de l'ennemi
                    enemies[i].deathDirection = -1;  // L'ennemi tombe vers la gauche
                } else { // Joueur à droite de l'ennemi
                    enemies[i].deathDirection = 1;  // L'ennemi tombe vers la droite
                }

            }
        }
    }
}

//Met à jours les kritters
void updateEnemies(Enemy *enemies, int enemyCount, Platform *platforms, int platformCount, Mix_Chunk *deathSound) {
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].isDead) {
            // Animation de mort (par exemple, 4 frames de la mort)
            enemies[i].frameCounter++;
            if (enemies[i].frameCounter >= ENEMY_FRAME_DELAY) {
                enemies[i].frameCounter = 0;
                enemies[i].deathFrame++;
                if (enemies[i].deathFrame >= 4) { // 4 frames de mort, puis on supprime l'ennemi
                    // L'ennemi disparaît après l'animation de la mort
                    enemies[i] = enemies[enemyCount - 1];  // Déplacer le dernier ennemi à la place de l'ennemi mort
                    enemyCount--;
                    i--;  // Re-vérifier à cette position
                    continue;
                }
            }
            // Jouer le son de la mort une seule fois
            if (enemies[i].deathFrame == 1) {
                Mix_PlayChannel(-1, deathSound, 0);  // Jouer le son de la mort
            }

            // Si l'ennemi est mort, il tombe en fonction de la direction (gauche ou droite)
            enemies[i].velocityY += GRAVITY;
            if (enemies[i].velocityY > MAX_FALL_SPEED) {
                enemies[i].velocityY = MAX_FALL_SPEED;
            }

            // Appliquer la direction de la chute (en fonction de deathDirection)
            enemies[i].x += enemies[i].deathDirection * ENEMY_SPEED+10; // L'ennemi tombe vers la gauche ou la droite

            // Mettre à jour la position verticale (la chute)
            enemies[i].y += enemies[i].velocityY;

        } else {
            // Animation normale et mise à jour si l'ennemi n'est pas mort
            enemies[i].velocityY += GRAVITY;

            if (enemies[i].velocityY > MAX_FALL_SPEED) {
                enemies[i].velocityY = MAX_FALL_SPEED;
            }

            enemies[i].y += enemies[i].velocityY;
            // Déplacement horizontal
            enemies[i].x += enemies[i].velocityX;
            // Vérification de la collision avec les plateformes
            bool onGround = false;
            for (int j = 0; j < platformCount; j++) {
                if (checkCollisionEnemy(&enemies[i], &platforms[j])) {
                    enemies[i].y = platforms[j].y - enemies[i].height;
                    enemies[i].velocityY = 0;  // Arrêter la chute
                    onGround = true;
                    break;
                }
            }

            // Si l'ennemi touche le sol, on l'empêche de tomber plus bas
            if (!onGround) {
                enemies[i].y += enemies[i].velocityY;
            }

            // Animation normale de l'ennemi
            enemies[i].frameCounter++;
            if (enemies[i].frameCounter >= ENEMY_FRAME_DELAY) {
                enemies[i].frameCounter = 0;
                enemies[i].currentFrame = (enemies[i].currentFrame + 1) % 7;  // Changer la frame entre 0 et 7
            }
        }
    }
}

//Render les kritters 
void renderEnemies(SDL_Renderer *renderer, Enemy *enemies, int enemyCount, int *cameraX) {
    for (int i = 0; i < enemyCount; i++) {
        SDL_Rect enemyRect = {enemies[i].x - *cameraX, enemies[i].y, enemies[i].width, enemies[i].height};

        if (enemies[i].isDead) {
            // Animation de mort
            if (enemies[i].deathFrame < 4 && enemies[i].deathtextures[enemies[i].deathFrame]) {
                SDL_RenderCopy(renderer, enemies[i].deathtextures[enemies[i].deathFrame], NULL, &enemyRect);
            }
        } else {
            // Animation normale
            if (enemies[i].textures[enemies[i].currentFrame]) {
                SDL_RenderCopy(renderer, enemies[i].textures[enemies[i].currentFrame], NULL, &enemyRect);
            } else {
                printf("EnnemyTexturesNotFoundException : Ennemy textures are missing\n");
            }
        }
    }
}

// Fonction pour initialiser et charger les ennemis pour un niveau précis
void loadLevelKritters(SDL_Renderer *renderer, Enemy **enemies, int *enemyCount, int level) {
    // Détruire les ennemis existants si nécessaire (libérer la mémoire)
    if (*enemies) {
        for (int i = 0; i < *enemyCount; i++) {
            for (int j = 0; j < 7; j++) {
                if ((*enemies)[i].textures[j]) {
                    SDL_DestroyTexture((*enemies)[i].textures[j]);
                }
            }
            free((*enemies)[i].textures);
            free((*enemies)[i].deathtextures);
        }
        free(*enemies);
    }

    // Initialiser le nombre d'ennemis pour le niveau spécifié
    if (level == 0) {  // Niveau 1
        *enemyCount = 3;
        *enemies = malloc(sizeof(Enemy) * (*enemyCount));
        

        // Initialiser les ennemis pour le niveau 1
        (*enemies)[0] = (Enemy){300, 400, 100, 100, ENEMY_SPEED, 0, NULL, 0, 0, 0, false, 0}; // deathDirection à 0
        (*enemies)[1] = (Enemy){400, 400, 100, 100, ENEMY_SPEED, 0, NULL, 0, 0, 0, false, 0}; // deathDirection à 0
        (*enemies)[2] = (Enemy){700, 400, 100, 100, ENEMY_SPEED, 0, NULL, 0, 0, 0, false, 0}; // deathDirection à 0
    } else if (level == 1) {  // Niveau 2
        *enemyCount = 2;
        *enemies = malloc(sizeof(Enemy) * (*enemyCount));

        // Initialiser les ennemis pour le niveau 2
        (*enemies)[0] = (Enemy){200, 500, 100, 100, ENEMY_SPEED, 0, NULL, 0, 0, 0, false, 0};
        (*enemies)[1] = (Enemy){600, 500, 100, 100, ENEMY_SPEED, 0, NULL, 0, 0, 0, false, 0};
    }
    
    // Charger les textures des ennemis
    for (int i = 0; i < *enemyCount; i++) {
        (*enemies)[i].textures = malloc(sizeof(SDL_Texture *) * 7);
        (*enemies)[i].deathtextures = malloc(sizeof(SDL_Texture *) * 4);  // 4 frames pour la mort

        // Charger les textures normales
        for (int j = 0; j < 7; j++) {
            char filename[60];
            snprintf(filename, sizeof(filename), "../assets/characters/kritter/walk/kritter%d.png", j + 1);
            (*enemies)[i].textures[j] = IMG_LoadTexture(renderer, filename);

            if (!(*enemies)[i].textures[j]) {
                printf("EnnemyTexturesNotFoundException : Ennemy textures did not load properly %s: %s\n", filename, SDL_GetError());
            }
        }

        // Charger les textures de mort
        for (int j = 0; j < 4; j++) {
            char filename2[60];
            snprintf(filename2, sizeof(filename2), "../assets/characters/kritter/death/kritter_death%d.png", j + 1);  // Noms des fichiers de mort
            (*enemies)[i].deathtextures[j] = IMG_LoadTexture(renderer, filename2);

            if (!(*enemies)[i].deathtextures[j]) {
                printf("EnnemyTexturesNotFoundException : Ennemy death textures did not load properly %s: %s\n", filename2, SDL_GetError());
            }
        }
    }
}









//Met à jour la caméra
void updateCamera(int *cameraX, int playerX) {
    int targetX = playerX - SCREEN_WIDTH / 2;
    if (targetX < 0) targetX = 0;
    if (targetX > LEVEL_WIDTH - SCREEN_WIDTH) targetX = LEVEL_WIDTH - SCREEN_WIDTH;
    *cameraX += (targetX - *cameraX) / CAMERA_SPEED;
}

// Fonction pour afficher la carte et gérer la sélection des niveaux
void showLevelSelection(SDL_Renderer *renderer, SDL_Texture *mapTexture, SDL_Texture *levelIconTexture, SDL_Rect *levelRects, int levelCount, SDL_Event *event, int *selectedLevel, Mix_Music *music) {
    SDL_RenderClear(renderer);

    // Jouer la musique de fond (si elle n'est pas déjà en cours)
    if (Mix_PlayingMusic() == 0) {
        // Charger la musique correspondante au niveau sélectionné
        const char *musicFile = levelMusicFiles[3];  // Choisir la musique basée sur le niveau sélectionné
        music = Mix_LoadMUS(musicFile);
        if (music != NULL) {
            Mix_PlayMusic(music, -1);  // Joue la musique en boucle
        } else {
            printf("Erreur de chargement de la musique : %s\n", musicFile);
        }
    }

    // Affichage de la carte
    SDL_RenderCopy(renderer, mapTexture, NULL, NULL);

    // Affichage des points de niveaux (représentés par des icônes ou des cercles)
    for (int i = 0; i < levelCount; i++) {
        // Afficher une icône pour chaque niveau
        SDL_Rect levelIconRect = {levelRects[i].x - 15, levelRects[i].y - 15, 30, 30};  // Décalage pour centrer l'icône
        SDL_RenderCopy(renderer, levelIconTexture, NULL, &levelIconRect);
    }

    SDL_RenderPresent(renderer);

    // Variables pour le fondu au noir
    SDL_Surface* blackSurface = SDL_CreateRGBSurface(0, 800, 600, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000);
    SDL_FillRect(blackSurface, NULL, SDL_MapRGB(blackSurface->format, 0, 0, 0));  // Surface noire

    int fadeDuration = 500; // Durée du fondu en millisecondes
    int fadeStep = 10;  // Vitesse du fondu
    int fadeAlpha = 0; // Début de l'alpha


    // Détection du clic pour choisir un niveau
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            *selectedLevel = -1;  // Quitter le jeu
            return;
        }

        if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            // Vérification si le clic est sur un point de niveau
            for (int i = 0; i < levelCount; i++) {
                int dx = mouseX - levelRects[i].x;
                int dy = mouseY - levelRects[i].y;
                if (dx * dx + dy * dy <= 15 * 15) {  // Si le clic est dans le rayon du point (ici rayon de 15)
                    *selectedLevel = i;

                     // Fondu de la musique et de l'écran
                     for (int j = 0; j < fadeDuration; j += fadeStep) {
                        fadeAlpha = (j * 255) / fadeDuration;  // Calcul de l'alpha en fonction du temps
                        SDL_SetSurfaceAlphaMod(blackSurface, fadeAlpha);

                        // Applique l'effet fondu sur l'écran
                        SDL_Texture* blackTexture = SDL_CreateTextureFromSurface(renderer, blackSurface);
                        SDL_RenderCopy(renderer, blackTexture, NULL, NULL);  // Rendu de la surface noire
                        SDL_RenderPresent(renderer);

                        SDL_Delay(fadeStep);
                    }

                    // Appliquer le fondu de la musique
                    Mix_FadeOutMusic(fadeDuration);  // Faire un fondu de la musique avant de changer de niveau

                    return;  // On a sélectionné un niveau, sortir de la fonction
                }
            }
        }
    }
    SDL_FreeSurface(blackSurface);
}

// Fonction pour initialiser et charger les textures et la musique pour un niveau précis
int loadLevelTextures(SDL_Renderer *renderer, SDL_Texture **background, SDL_Texture **platformTexture, Platform **platforms, int level, int *platformCount, Mix_Music **music) {
    // Décharger les anciennes textures si elles existent
    if (*background) {
        SDL_DestroyTexture(*background);
    }
    if (*platformTexture) {
        SDL_DestroyTexture(*platformTexture);
    }

    if (*music) {
        Mix_FreeMusic(*music);
        *music = NULL;
    }

    // Charger les nouvelles textures pour le niveau sélectionné
    if (level == 0) {  // Niveau 1
        *background = IMG_LoadTexture(renderer, "../assets/map/background.png");
        *platformTexture = IMG_LoadTexture(renderer, "../assets/map/platform.png");
        *platformCount = 2; //pour changer le nombre de plateforme par niveau
        *platforms = malloc(sizeof(Platform) * (*platformCount));

        // Définir les plateformes pour le niveau 1
        (*platforms)[0] = (Platform){100, 400, 800, 60}; //x,y,width,height;
        (*platforms)[1] = (Platform){400, 400, 800, 60};
  
     

    } else if (level == 1) {  // Niveau 2
        *background = IMG_LoadTexture(renderer, "../assets/map/level2_background.png");
        *platformTexture = IMG_LoadTexture(renderer, "../assets/map/level_2_platform.png");
        *platformCount = 4;
        *platforms = malloc(sizeof(Platform) * (*platformCount));

        // Définir les plateformes pour le niveau 2
        (*platforms)[0] = (Platform){100, 500, 350, 60};
        (*platforms)[1] = (Platform){500, 400, 150, 40};
        (*platforms)[2] = (Platform){900, 300, 250, 50};
        (*platforms)[3] = (Platform){1300, 500, 300, 50};
    }
    // Charger la musique
    *music = Mix_LoadMUS(levelMusicFiles[level]);
    if (!*music) {
        printf("Erreur chargement musique : %s\n", Mix_GetError());
    }
    // Ajoute d'autres niveaux si nécessaire
    return level;
}






void handleInput(Player *player) {
    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    bool moving = false;

    // Courir (Shift)
    bool running = keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT];

    if (keystate[SDL_SCANCODE_LEFT]) {
    player->x -= running ? PLAYER_SPEED * 2 : PLAYER_SPEED;
    player->facingRight = false;
    if (player->state != RUN && player->state != WALK) {
        player->state = running ? RUN : WALK;
    }
    moving = true;
}

if (keystate[SDL_SCANCODE_RIGHT]) {
    player->x += running ? PLAYER_SPEED * 2 : PLAYER_SPEED;
    player->facingRight = true;
    if (player->state != RUN && player->state != WALK) {
        player->state = running ? RUN : WALK;
    }
    moving = true;
}

// Si aucune touche de direction n'est enfoncée, et que le joueur est au sol, on le met en état IDLE
if (!moving && player->onGround) {
    player->state = IDLE;
}


    // Saut (Espace)
    if (keystate[SDL_SCANCODE_SPACE] && player->onGround) {
        player->velocityY = JUMP_STRENGTH;
        player->onGround = false;
        player->state = JUMP;
    }

    // Accroupissement (Ctrl)
    if ((keystate[SDL_SCANCODE_LCTRL] || keystate[SDL_SCANCODE_RCTRL]) && player->onGround) {
        if (!moving) player->state = CROUCH;
    }

    // Attaque (F)
    if (keystate[SDL_SCANCODE_F]) {
        player->state = ATTACK;
        moving = true;
    }


}


void updatePlayerAnimation(Player *player) {
    player->frameCounter++;
//        printf("FrameCounter: %d | CurrentFrame: %d | State: %d | onGround: %d\n",
//           player->frameCounter, player->currentFrame, player->state, player->onGround);
    if (player->frameCounter >= 16) {  // Changer de frame toutes les 10 itérations
        player->frameCounter = 0;
        player->currentFrame++;

        switch (player->state) {
            case IDLE:
                player->currentFrame %= 5; // 5 frames idle
                break;
            case WALK:
                player->currentFrame %= 16; // 16 frames marche
                break;
            case RUN:
                player->currentFrame %= 16; // 16 frames sprint
                break;
            case JUMP:
                if (player->onGround) {
                    player->state = IDLE;
                    player->currentFrame = 0;  // Si on touche le sol, revenir à idle
                    
                } else {
                    player->currentFrame %= 10; // Utiliser un nombre de frames correct pour éviter les crashs
                }
                break;
            case ATTACK:
                if (player->currentFrame >= 15) {
                    player->currentFrame = 0; // Réinitialiser l'attaque après l'animation
                    player->state = IDLE;
                }
                break;
            case CROUCH:
                player->currentFrame %= 4; // 4 frames accroupi
                break;
        }
    }
}



void freePlayerTextures(Player *player) {
    for (int i = 0; i < 5; i++) SDL_DestroyTexture(player->idleTextures[i]);
    for (int i = 0; i < 16; i++) SDL_DestroyTexture(player->walkTextures[i]);
    for (int i = 0; i < 16; i++) SDL_DestroyTexture(player->runTextures[i]);
    for (int i = 0; i < 10; i++) SDL_DestroyTexture(player->jumpTextures[i]);
    for (int i = 0; i < 15; i++) SDL_DestroyTexture(player->attackTextures[i]);
    for (int i = 0; i < 4; i++) SDL_DestroyTexture(player->crouchTextures[i]);

    free(player->idleTextures);
    free(player->walkTextures);
    free(player->runTextures);
    free(player->jumpTextures);
    free(player->attackTextures);
    free(player->crouchTextures);
}

void updatePlayerPhysics(Player *player, Platform *platforms, int platformCount) {
    if (!player->onGround) {
        // Appliquer la gravité uniquement si le joueur n'est pas au sol
        player->velocityY += GRAVITY;
        if (player->velocityY > MAX_FALL_SPEED) {  // Limiter la vitesse de chute
            player->velocityY = MAX_FALL_SPEED;
        }
        player->y += player->velocityY;
    }

    // Vérification des collisions avec les plateformes
    player->onGround = false; // Par défaut, le joueur n'est pas au sol

    for (int i = 0; i < platformCount; i++) {
        if (checkCollision(player, &platforms[i])) {
            // Collision détectée, le joueur est au sol
            player->y = platforms[i].y - player->height; // Positionner le joueur juste au-dessus de la plateforme
            player->velocityY = 0; // Arrêter la vitesse verticale
            player->onGround = true; // Le joueur est au sol
            break; // Si une plateforme est touchée, on s'arrête
        }
    }

    // Si le joueur touche le sol, réinitialiser l'état du saut
    if (player->onGround && player->state == JUMP) {
        player->state = IDLE; // Revenir à l'état de repos
    }

    // Vérification si le joueur est tombé trop bas (par exemple, au-delà du sol)
    if (player->y >= 400) {
        player->y = 400;  // Positionner le joueur au sol
        player->velocityY = 0;
        player->onGround = true;  // Le joueur est sur le sol
        if (player->state == JUMP) {
            player->state = IDLE;  // Retour à l'état normal après le saut
        }
    }
}


void renderPlayer(SDL_Renderer *renderer, Player *player) {
    SDL_Texture *texture = NULL;

    switch (player->state) {
        case IDLE: texture = player->idleTextures[player->currentFrame]; break;
        case WALK: texture = player->walkTextures[player->currentFrame]; break;
        case RUN: texture = player->runTextures[player->currentFrame]; break;
        case JUMP: texture = player->jumpTextures[player->currentFrame]; break;
        case ATTACK: texture = player->attackTextures[player->currentFrame]; break;
        case CROUCH: texture = player->crouchTextures[player->currentFrame]; break;
    }

    SDL_Rect dstRect = {player->x, player->y, player->width, player->height};

    // Si le joueur est tourné vers la gauche, on flip l'image
    SDL_RendererFlip flip = player->facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(renderer, texture, NULL, &dstRect, 0, NULL, flip);
}




void loadPlayer(SDL_Renderer *renderer, Player *player) {
    // Initialisation des attributs du joueur
    player->x = 100;
    player->y = 50;
    player->width = 50;  // Largeur du sprite
    player->height = 50; // Hauteur du sprite
    player->velocityY = 0;
    player->onGround = false;
    player->state = IDLE;
    player->facingRight = true;  
    player->currentFrame = 0;
    player->frameCounter = 0;

    // Allocation des textures
    player->idleTextures = malloc(sizeof(SDL_Texture *) * 5);
    player->walkTextures = malloc(sizeof(SDL_Texture *) * 16);
    player->runTextures = malloc(sizeof(SDL_Texture *) * 16);
    player->jumpTextures = malloc(sizeof(SDL_Texture *) * 10);
    player->attackTextures = malloc(sizeof(SDL_Texture *) * 15);
    player->crouchTextures = malloc(sizeof(SDL_Texture *) * 4);


    char *basePath = SDL_GetBasePath();
    if (!basePath) {
        printf("Erreur : Impossible de récupérer le chemin de base !\n");
        return;
    }
    
    // Charger les textures Idle
    for (int i = 0; i < 5; i++) {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s../assets/characters/player/idle/dixie_idle%d.png", basePath, i + 1);

        // Afficher le chemin complet pour débogage
        printf("Loading texture: %s\n", fullPath);

        player->idleTextures[i] = IMG_LoadTexture(renderer, fullPath);

        if (!player->idleTextures[i]) {
            printf("PlayerTexturesNotFoundException : Idle texture did not load properly %s: %s\n", fullPath, SDL_GetError());
        }
    }
    
    // Charger les textures de marche
    for (int i = 0; i < 16; i++) {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s../assets/characters/player/walk/dixie_walk%d.png", basePath, i + 1);
        player->walkTextures[i] = IMG_LoadTexture(renderer, fullPath);
    
        if (!player->walkTextures[i]) {
            printf("PlayerTexturesNotFoundException : Walk texture did not load properly %s: %s\n", fullPath, SDL_GetError());
        }
    }
    
    // Charger les textures de course
    for (int i = 0; i < 16; i++) {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s../assets/characters/player/sprint/dixie_sprint%d.png", basePath, i + 1);
        player->runTextures[i] = IMG_LoadTexture(renderer, fullPath);
    
        if (!player->runTextures[i]) {
            printf("PlayerTexturesNotFoundException : Run texture did not load properly %s: %s\n", fullPath, SDL_GetError());
        }
    }
    
    // Charger les textures de saut
    for (int i = 0; i < 10; i++) {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s../assets/characters/player/jump/dixie_jump%d.png", basePath, i + 1);
        player->jumpTextures[i] = IMG_LoadTexture(renderer, fullPath);
    
        if (!player->jumpTextures[i]) {
            printf("PlayerTexturesNotFoundException : Jump texture did not load properly %s: %s\n", fullPath, SDL_GetError());
        }
    }
    
    // Charger les textures d'attaque
    for (int i = 0; i < 15; i++) {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s../assets/characters/player/hit/dixie_hit%d.png", basePath, i + 1);
        player->attackTextures[i] = IMG_LoadTexture(renderer, fullPath);
    
        if (!player->attackTextures[i]) {
            printf("PlayerTexturesNotFoundException : Attack texture did not load properly %s: %s\n", fullPath, SDL_GetError());
        }
    }
    
    // Charger les textures d'accroupissement
    for (int i = 0; i < 4; i++) {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s../assets/characters/player/crouch/dixie_crouch%d.png", basePath, i + 1);
        player->crouchTextures[i] = IMG_LoadTexture(renderer, fullPath);
    
        if (!player->crouchTextures[i]) {
            printf("PlayerTexturesNotFoundException : Crouch texture did not load properly %s: %s\n", fullPath, SDL_GetError());
        }
    }
    
    // Libérer la mémoire après avoir fini toutes les utilisations
    SDL_free(basePath);

}


// Fonction pour démarrer un niveau
void startLevel(SDL_Renderer *renderer, SDL_Texture *background, SDL_Texture *platformTexture, Platform *platforms, int platformCount, Player *player, int *cameraX, Mix_Music *music,int level) {
    // Jeu pour un niveau spécifique
    

    Enemy *enemies = NULL;
    int enemyCount = 0;
    loadPlayer(renderer,player);
    loadLevelKritters(renderer, &enemies, &enemyCount,level);

    bool running = true;
    SDL_Event event;
    
     // Jouer la musique du niveau
     if (music) {
        Mix_PlayMusic(music, -1);  // -1 pour répéter la musique en boucle
   
    }  

    

    Mix_Chunk *kritter_death = Mix_LoadWAV(levelMusicFiles[4]);
    if (!kritter_death) {
        printf("Erreur de chargement du son de mort: %s\n", Mix_GetError());
    }

    while (running) {
        SDL_Delay(5);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        const Uint8 *keystate = SDL_GetKeyboardState(NULL);

        handleInput(player);




        if (player->y > SCREEN_HEIGHT) {
            player->x = 100;
            player->y = 100;
            player->velocityY = 0;
        }

        // Mise à jour de la caméra
        updateCamera(cameraX, player->x);

        updateEnemies(enemies, enemyCount, platforms, platformCount,kritter_death);
        
       
       updatePlayerAnimation(player);
        updatePlayerPhysics(player,platforms,platformCount);
//        printf("state: %d, currentFrame: %d, onGround: %d\n", player->state, player->currentFrame, player->onGround);

        // Affichage
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Affichage fond
        int bgWidth = SCREEN_WIDTH;
        for (int x = -(*cameraX); x < LEVEL_WIDTH; x += bgWidth) {
            SDL_Rect bgRect = {x, 0, bgWidth, SCREEN_HEIGHT};
            SDL_RenderCopy(renderer, background, NULL, &bgRect);
        }

        // Affichage des plateformes
        for (int i = 0; i < platformCount; i++) {
            SDL_Rect platformRect = {platforms[i].x - (*cameraX), platforms[i].y, platforms[i].width, platforms[i].height};
            SDL_RenderCopy(renderer, platformTexture, NULL, &platformRect);
        }



        renderPlayer(renderer,player);

        renderEnemies(renderer, enemies, enemyCount, cameraX);
        

      handlePlayerEnemyCollision(player, enemies, &enemyCount,kritter_death);
        
        
        SDL_RenderPresent(renderer);
    }

    //Libérations de mémoire
    /*
    for (int i = 0; i < enemyCount; i++) {
        SDL_DestroyTexture(enemies[i].textures[0]);
        SDL_DestroyTexture(enemies[i].textures[1]);
        free(enemies[i].textures);
        
    }
    free(enemies);
    */
    freePlayerTextures(player);
    // Arrêter la musique à la fin du niveau
    //Mix_FreeChunk(kritter_death);
    Mix_HaltMusic();
}

//Charge les ressources, gère les appels et nettoie à la fin du jeu    
int WinMain(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erreur SDL : %s\n", SDL_GetError());
        return 1;
    }



    SDL_Window *window = SDL_CreateWindow("Jeu SDL - Sélection des niveaux",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Erreur création fenêtre : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erreur création renderer : %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    if (Mix_OpenAudio(44100 , MIX_DEFAULT_FORMAT, 2, 2840) < 0) {
        printf("Erreur initialisation SDL_mixer : %s\n", Mix_GetError());
        return 1;
    }

    Mix_Music *music = NULL; // ost

    IMG_Init(IMG_INIT_PNG);

    // Charger la carte de sélection des niveaux
    SDL_Texture *mapTexture = IMG_LoadTexture(renderer, "../assets/map/map.png");
    SDL_Texture *levelIconTexture = IMG_LoadTexture(renderer, "../assets/map/level_icon.png"); // Icône pour les points de niveau
    
    SDL_Rect levelRects[] = {
        {100, 150, 0, 0},  // Niveau 1
        {400, 300, 0, 0},  // Niveau 2
        {700, 450, 0, 0},  // Niveau 3
    };
    int levelCount = sizeof(levelRects) / sizeof(SDL_Rect);

    Player player;

    // Boucle de sélection des niveaux
    int selectedLevel = -1;
    SDL_Event event;
    while (selectedLevel == -1) {
        showLevelSelection(renderer, mapTexture, levelIconTexture, levelRects, levelCount, &event, &selectedLevel, music);
    }

    // Initialiser les plateformes et textures pour le niveau choisi
    SDL_Texture *background = NULL;
    SDL_Texture *platformTexture = NULL;
    Platform *platforms = NULL;
    int platformCount = 0;

    int level = loadLevelTextures(renderer, &background, &platformTexture, &platforms, selectedLevel, &platformCount,&music);

    int cameraX = 0;

    // Lancer le niveau sélectionné
    startLevel(renderer, background, platformTexture, platforms, platformCount, &player, &cameraX, music,level);

    
    // Nettoyage
    free(platforms);
    SDL_DestroyTexture(mapTexture);
    SDL_DestroyTexture(levelIconTexture);
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(platformTexture);
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
    

    




    
    

  

    // Chargement d'une police TTF
//    TTF_Font *font = TTF_OpenFont("../../../usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 24);
   