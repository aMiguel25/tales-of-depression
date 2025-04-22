#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080


#define ANIMATION_FRAMES 11  // Nombre total d'images pour l'animation

SDL_Texture *monitorAnimationFrames[ANIMATION_FRAMES];

void loadMonitorAnimation(SDL_Renderer *renderer) {
    for (int i = 0; i < ANIMATION_FRAMES; i++) {
        char path[64];
        snprintf(path, sizeof(path), "../assets/haunted/monitor/monitor%d.png", i);
        monitorAnimationFrames[i] = IMG_LoadTexture(renderer, path);
    }
}
int monitorAnimationStep = -1; // -1 = pas d'animation en cours
bool isClosingMonitor = false; // Pour savoir si on ferme ou ouvre

void renderMonitorAnimation(SDL_Renderer *renderer) {
    if (monitorAnimationStep >= 0 && monitorAnimationStep < ANIMATION_FRAMES) {
        SDL_RenderCopy(renderer, monitorAnimationFrames[monitorAnimationStep], NULL, NULL);
    }
}

typedef enum State {
    MENU,
    GAME,
    QUIT
} GameState;

typedef struct Interactible {
    int x, y, w, h;
    const char *label;
} Button;


Mix_Chunk *monitorOpenSound = NULL;
Mix_Chunk *monitorCloseSound = NULL;
Mix_Chunk *cameraSwitchSound = NULL;

// Initialiser le bouton bascule pour la tablette de sécurité
void initCameraSwitchButton(Button *button) {
    button->x = (SCREEN_WIDTH / 2) - 100;  // Centré sur l'écran
    button->y = SCREEN_HEIGHT - 100;  // Bas de l'écran
    button->w = 200;
    button->h = 50;
    button->label = "Tablette";  // Libellé du bouton
}

// Vérifie si la souris est sur le bouton
bool isMouseOverButton(int mouseX, int mouseY, Button button) {
    return mouseX >= button.x && mouseX <= button.x + button.w &&
           mouseY >= button.y && mouseY <= button.y + button.h;
}

// Affiche le bouton
void renderButton(SDL_Renderer *renderer, TTF_Font *font, Button button) {
    SDL_Color textColor = {255, 255, 255, 255}; // Couleur blanche
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, button.label, textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect buttonRect = {button.x, button.y, button.w, button.h}; // Taille correcte du bouton
    SDL_Rect textRect = {button.x + (button.w - textSurface->w) / 2, 
                         button.y + (button.h - textSurface->h) / 2, 
                         textSurface->w, textSurface->h};

    // Dessiner un fond bleu pour le bouton
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRect(renderer, &buttonRect);

    // Dessiner le texte du bouton
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}



typedef struct Camera {
    int x, y, w, h;      // Position et taille de la caméra
    const char *label;    // Le nom de la caméra (ex : "Bureau", "Couloir")
    SDL_Texture *texture; // La texture de la caméra (affichée quand la caméra est active)
} security_camera;


typedef struct MapCamera{
    int x, y, w, h;
    int cameraIndex;
} MapCamera;

//Initialise les caméras
void initMapCameras(MapCamera *mapCameras) {
    mapCameras[0] = (MapCamera){50, 50, 40, 40, 0};  // Bureau
    mapCameras[1] = (MapCamera){150, 50, 40, 40, 1}; // Couloir
}

//Affiche la carte du moniteur
void renderMap(SDL_Renderer *renderer, MapCamera *mapCameras, int numCameras) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Gris foncé pour la map
    SDL_Rect mapRect = {600, 400, 200, 200}; // Taille et position de la map
    SDL_RenderFillRect(renderer, &mapRect);

    // Dessiner les caméras sur la carte
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Rouge pour les caméras
    for (int i = 0; i < numCameras; i++) {
        SDL_Rect camRect = {mapRect.x + mapCameras[i].x, mapRect.y + mapCameras[i].y, 
                            mapCameras[i].w, mapCameras[i].h};
        SDL_RenderFillRect(renderer, &camRect);
    }
}

//Détecte qu'on a cliqué sur la carte du moniteur et change de caméra
void handleMapClick(int mouseX, int mouseY, MapCamera *mapCameras, int numCameras, int *activeCamera) {
    for (int i = 0; i < numCameras; i++) {
        if (mouseX >= mapCameras[i].x + 600 && mouseX <= mapCameras[i].x + 640 &&
            mouseY >= mapCameras[i].y + 400 && mouseY <= mapCameras[i].y + 440) {
            *activeCamera = mapCameras[i].cameraIndex;
            
            
        }
    }
}

// Fonction de gestion du changement de caméra
void handleCameraSwitchButtonClick(int mouseX, int mouseY, Button button, int *activeCamera, bool *isCameraActive) {
    if (isMouseOverButton(mouseX, mouseY, button)) {
        *isCameraActive = !*isCameraActive;  // Alterner entre afficher la caméra et le bureau
        if (*isCameraActive) {
            // Si on passe à la caméra, on assure que la première caméra est affichée
            *activeCamera = 0;
            Mix_PlayChannel(-1, monitorOpenSound, 0);
            monitorAnimationStep = 0; // Démarrer l'animation d'ouverture
            isClosingMonitor = false;
        } else {
            Mix_PlayChannel(-1, monitorCloseSound, 0);
            *activeCamera = -1; // Revenir à l'écran de bureau
        }
    }
}


// Initialiser les caméras au début
void initCameras(SDL_Renderer *renderer, security_camera *cameras, int *numCameras) {
    // Initialisation des caméras avec leurs positions et textures
    cameras[0].x = 100;
    cameras[0].y = 100;
    cameras[0].w = 200;
    cameras[0].h = 100;
    cameras[0].label = "Bureau";
    cameras[0].texture = IMG_LoadTexture(renderer, "../assets/haunted/cellar.png");

    cameras[1].x = 400;
    cameras[1].y = 100;
    cameras[1].w = 200;
    cameras[1].h = 100;
    cameras[1].label = "Couloir";
    cameras[1].texture = IMG_LoadTexture(renderer, "../assets/haunted/cellar2.png");

    *numCameras = 2; // Par exemple, 2 caméras
}

bool isMouseOverCamera(int mouseX, int mouseY, security_camera camera) {
    return mouseX >= camera.x && mouseX <= camera.x + camera.w && mouseY >= camera.y && mouseY <= camera.y + camera.h;
}

//gère le click sur le bouton
void handleCameraClick(int mouseX, int mouseY, security_camera *cameras, int numCameras, int *activeCamera) {
    for (int i = 0; i < numCameras; i++) {
        if (isMouseOverCamera(mouseX, mouseY, cameras[i])) {
            if (*activeCamera != i) { // Jouer le son uniquement si la caméra change
                Mix_PlayChannel(-1, cameraSwitchSound, 0);
            }
            *activeCamera = i;
        }
    }
}

//Uttilise le moniteur avec espace
void handleCameraSwitchKey(SDL_Event event, int *activeCamera, bool *isCameraActive) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        *isCameraActive = !*isCameraActive;  // Basculer entre caméra et bureau
        if (*isCameraActive) {
            *activeCamera = 0;  // Passer à la première caméra
        }
    }
}

void renderActiveCamera(SDL_Renderer *renderer, security_camera *cameras, int activeCamera) {
    if (activeCamera >= 0) {
        SDL_Rect cameraRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}; // Affiche la caméra plein écran
        SDL_RenderCopy(renderer, cameras[activeCamera].texture, NULL, &cameraRect);
    }
}







// Gère l'affichage du texte
void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y) {
    SDL_Color color = {255, 0, 0, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Affiche le menu avec son texte
void renderMenu(SDL_Renderer *renderer, SDL_Texture *background, TTF_Font *font, Button buttons[], int numButtons) {
    SDL_RenderClear(renderer);
    if (background) {
        SDL_RenderCopy(renderer, background, NULL, NULL);
    }

    renderText(renderer, font, "Tales Of Depression", 700, 100);  // Titre du jeu
    
    // Afficher les boutons du menu
    for (int i = 0; i < numButtons; i++) {
        renderText(renderer, font, buttons[i].label, buttons[i].x, buttons[i].y);
    }

    SDL_RenderPresent(renderer);
}

// Fonction pour vérifier si un clic est dans un bouton donné
bool isMouseOver(int mouseX, int mouseY, Button button) {
    return mouseX >= button.x && mouseX <= button.x + button.w && mouseY >= button.y && mouseY <= button.y + button.h;
}

// Fondu au noir avec fondu de musique
void fadeToBlack(SDL_Renderer *renderer, Mix_Music *music, int fadeDuration) {
    int fadeSteps = 100;  // Nombre de pas de fondu
    int fadeInterval = fadeDuration / fadeSteps;  // Intervalle entre chaque étape de fondu

    // Diminuer progressivement le volume de la musique
    for (int i = 0; i < fadeSteps; i++) {
        // Calculer le volume actuel (de 0 à 128)
        int volume = 128 - (i * (128 / fadeSteps));  
        Mix_VolumeMusic(volume);  // Appliquer le volume à la musique

        // Créer un fondu au noir
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, NULL);  // Remplir tout l'écran avec du noir
        
        SDL_RenderPresent(renderer);
        SDL_Delay(fadeInterval);  // Attendre l'intervalle de temps avant la prochaine étape du fondu
    }

    // Assurez-vous que la musique est complètement éteinte à la fin
    Mix_VolumeMusic(0);  
}

//Permet de tourner la caméra dans la pièce
int handleCameraMovement(SDL_Event *event) {
    int mouseX = SCREEN_WIDTH / 2; // Valeur par défaut au centre

    SDL_GetMouseState(&mouseX, NULL);
    int offsetX = (mouseX - (SCREEN_WIDTH / 2)) / 4; // Réduction du déplacement

    if (offsetX < -300) offsetX = -300;
    if (offsetX > 300) offsetX = 300;

    return offsetX;
}


void renderNight(SDL_Renderer *renderer, TTF_Font *font, int night) {
    char nightText[64];
    snprintf(nightText, sizeof(nightText), "Nuit: %d", night);
    renderText(renderer, font, nightText, 10, 10);  // Afficher le niveau en haut à gauche
}

// Fonction pour rendre le jeu en changeant entre la caméra et le bureau
int renderGame(SDL_Renderer *renderer, TTF_Font *font, int *night, int *survivalTime, Uint32 *lastUpdateTime,
    SDL_Texture *officeTexture, Button *cameraSwitchButton, security_camera *cameras, 
    int *activeCamera, bool *isCameraActive, SDL_Event *event, MapCamera *mapCameras, int numCameras) {

    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Noir
    SDL_RenderFillRect(renderer, NULL);  // Remplir toute la fenêtre

    int offsetX = handleCameraMovement(event);
    SDL_Rect officeRect = {-offsetX, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    if (officeTexture && !(*isCameraActive)) {
        SDL_RenderCopy(renderer, officeTexture, NULL, &officeRect);
    }

    renderNight(renderer, font, *night);

    Uint32 currentTime = SDL_GetTicks();
    if (currentTime > *lastUpdateTime + 1000) {
    (*survivalTime)--;
    *lastUpdateTime = currentTime;
    }

    char timeText[64];
    snprintf(timeText, sizeof(timeText), "Temps Restant: %d", *survivalTime);
    renderText(renderer, font, timeText, 700, 10);

    if (monitorAnimationStep >= 0) {
        renderMonitorAnimation(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(20); // Pause entre chaque frame
    
        monitorAnimationStep++; // Passer à l'image suivante
    
        if (monitorAnimationStep >= ANIMATION_FRAMES) {
            monitorAnimationStep = -1; // Fin de l'animation
            *isCameraActive = !isClosingMonitor;
            if (*isCameraActive) {
                *activeCamera = 0; // Passer à la première caméra
            } else {
                *activeCamera = -1; // Revenir au bureau
            }
        }
        
    }
    if (*isCameraActive) {
    renderActiveCamera(renderer, cameras, *activeCamera);
    renderMap(renderer, mapCameras, numCameras);
    }

    renderButton(renderer, font, *cameraSwitchButton);



    SDL_RenderPresent(renderer);

    if (*survivalTime <= 0) {
        (*night)++;
        *survivalTime = 10;
        return 0;
    }

    return 1;
}

//Lance le jeu en commençant par le menu 
void gameLoop(SDL_Renderer *renderer, SDL_Texture *background, TTF_Font *font, Button buttons[], int numButtons, Mix_Music *music, SDL_Texture *officeTexture) {
    
    GameState currentState = MENU; // Démarrer au menu
    Mix_PlayMusic(music, -1);

    SDL_Event event;
    int mouseX, mouseY;
    bool quit = false;

    int night = 1;  // Niveau de départ
    int survivalTime = 500;  // Temps de survie pour ce niveau (par exemple 500 unités de temps)
    Uint32 lastUpdateTime = SDL_GetTicks(); // Initialiser le temps


        // Initialiser le bouton pour changer de caméra
        Button cameraSwitchButton;
        initCameraSwitchButton(&cameraSwitchButton);
    

    while (!quit) {

        //MENU PRINCIPAL//
        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_QUIT) {
                currentState = QUIT;
                quit = true;
            }

            else if (event.type == SDL_MOUSEBUTTONDOWN) {

                // Vérifier si un bouton a été cliqué
                SDL_GetMouseState(&mouseX, &mouseY);

                for (int i = 0; i < numButtons; i++) {

                    if (isMouseOver(mouseX, mouseY, buttons[i])) {

                        // Gestion du Menu (démarrer le jeu ou quitter) //

                        if (strcmp(buttons[i].label, "Start Game") == 0) {
                            currentState = GAME;
                        }

                        else if (strcmp(buttons[i].label, "Quit") == 0) {
                            currentState = QUIT;
                            quit = true;
                        }
                    }
                }
            }
        }

        // Rendre selon l'état du jeu
        if (currentState == MENU) {
            renderMenu(renderer, background, font, buttons, numButtons);
            
        }
        //Lancement de la nuit x
        else if (currentState == GAME) {
            fadeToBlack(renderer, music, 5000);
            // Initialisation des caméras
            security_camera cameras[10];
            int numCameras;
            bool isCameraActive = false;
            initCameras(renderer, cameras, &numCameras);  // Initialiser les caméra
            int activeCamera = -1; // Aucune caméra n'est active au début
            int gameStatus = 1;
            // Initialisation des caméras sur la mini-map
            MapCamera mapCameras[10]; // Supposons qu'on puisse avoir jusqu'à 10 caméras
            initMapCameras(mapCameras);
            while (gameStatus == 1) {
                while (SDL_PollEvent(&event)) { 
                    if (event.type == SDL_QUIT) {
                        currentState = QUIT;
                        gameStatus = 0;
                    }
                    
                    else if (event.type == SDL_MOUSEBUTTONDOWN) {
                        printf("Clic détecté en jeu: %d, %d\n", event.button.x, event.button.y);
                        SDL_GetMouseState(&mouseX, &mouseY);
                        // Vérifier si une caméra a été cliquée
                        handleCameraSwitchButtonClick(mouseX, mouseY, cameraSwitchButton, &activeCamera, &isCameraActive);
                        
                        handleCameraClick(mouseX, mouseY, cameras, numCameras, &activeCamera);
                            
                        
                    }
                    
                    else{
                        handleCameraSwitchKey(event, &activeCamera, &isCameraActive);
                    }
                }
                gameStatus = renderGame(renderer, font, &night, &survivalTime, &lastUpdateTime, officeTexture,&cameraSwitchButton, cameras, &activeCamera,&isCameraActive,&event,mapCameras,numCameras);
                
                                
                
                // Afficher la caméra active si elle est définie
                renderActiveCamera(renderer, cameras, activeCamera);
                        // Faire un fade au noir si on quitte

                        if (isCameraActive) {
                            handleMapClick(mouseX, mouseY, mapCameras, numCameras, &activeCamera);
                        }

                if (currentState == QUIT) {
                    fadeToBlack(renderer, music, 2000);  // Fondu au noir avec une durée de 2 secondes

                }
            }
        }


    }
}

//Initialise les ressources
void init(SDL_Renderer **renderer, SDL_Window **window, TTF_Font **font, Mix_Music **music, SDL_Texture **officeTexture) {
    // Initialisation de SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("Erreur SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    // Création de la fenêtre
    *window = SDL_CreateWindow("Tales Of Depression", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!*window) {
        printf("Erreur SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    // Création du renderer
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        printf("Erreur SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    // Initialisation des bibliothèques externes
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        printf("Erreur Mix_OpenAudio: %s\n", Mix_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    if (TTF_Init() == -1) {
        printf("Erreur TTF_Init: %s\n", TTF_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    // Charger la police
    *font = TTF_OpenFont("../assets/blood_crow/bloodcrow.ttf", 24);
    if (!*font) {
        printf("Erreur TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    // Charger la musique
    *music = Mix_LoadMUS("../assets/haunted_sounds/corrupted.ogg");
    if (!*music) {
        printf("Erreur Mix_LoadMUS: %s\n", Mix_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    // Charger l’image de l’office
    *officeTexture = IMG_LoadTexture(*renderer, "../assets/haunted/desk.png");
    if (!*officeTexture) {
        printf("Erreur chargement image: %s\n", IMG_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    loadMonitorAnimation(*renderer);
}




void loadSounds(void) {
    monitorOpenSound = Mix_LoadWAV("../assets/haunted_sounds/monitor_open.wav");
    monitorCloseSound = Mix_LoadWAV("../assets/haunted_sounds/monitor_close.wav");
    cameraSwitchSound = Mix_LoadWAV("../assets/haunted_sounds/camera_switch.wav");

    if (!monitorOpenSound || !monitorCloseSound || !cameraSwitchSound) {
        printf("Erreur chargement sons: %s\n", Mix_GetError());
    }
}


void cleanup(void) {
    for (int i = 0; i < ANIMATION_FRAMES; i++) {
        SDL_DestroyTexture(monitorAnimationFrames[i]);
    }

}

int WinMain(int argc, char **argv) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    TTF_Font *font = NULL;
    Mix_Music *music = NULL;
    SDL_Texture *officeTexture = NULL;

    init(&renderer, &window, &font, &music, &officeTexture);
    loadSounds();
    // Définir les boutons du menu
    Button buttons[] = {
        {700, 400, 200, 50, "Start Game"},
        {700, 500, 200, 50, "Quit"}
    };

    // Charger le fond d'écran du menu
    SDL_Surface *bgSurface = IMG_Load("../assets/haunted/background.png");
    SDL_Texture *background = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface);



    // Lancer la boucle de jeu
    gameLoop(renderer, background, font, buttons, 2, music,officeTexture);

    // Nettoyer et quitter
    Mix_FreeMusic(music);
    Mix_CloseAudio();
    TTF_CloseFont(font);
    SDL_DestroyTexture(background);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}



        /*
        else if (currentState == GAME) {
            // Commencer le jeu ici

            SDL_RenderClear(renderer);
            renderText(renderer, font, "Game in Progress...", 700, 500);  // Chargemeent
            SDL_RenderPresent(renderer);
            
            SDL_Delay(16);
            
            while(1){
                Uint32 lastUpdateTime = SDL_GetTicks(); 
                int end = renderGame(renderer, font, &night, &survivalTime,&lastUpdateTime);
                if(end == 0){
                    break;
                }
            }

        }
        */