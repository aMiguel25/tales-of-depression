#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define NUM_OPTIONS 8  

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    Mix_Music* music;
    Mix_Chunk* soundSelect;  // Son pour le changement d'option
    Mix_Chunk* soundClick;   //  Son pour la validation
    Mix_Chunk* soundBack;    //  Son pour le retour
    SDL_Texture* background;  //  Texture pour le fond
    int selectedOption;
} Menu;

const char* options[] = {
    "Nouvelle Partie", 
    "Charger", 
    "Sauvegarder", 
    "Extras", 
    "Crédits", 
    "Paramètres",  // Nouvelle option ici
    "Quitter"
};



#define MAX_ROOMS 4  // Maximum de pièces sélectionnables
#define ROOM_WIDTH 400
#define ROOM_HEIGHT 300

typedef struct {
    const char* name;
    SDL_Texture* texture;
    SDL_Rect rect;
    int cameraX; // Position de la "caméra" dans la pièce
} Room;

void loadRooms(Room rooms[], int* roomCount, SDL_Renderer* renderer) {
    *roomCount = 1; // Nuit 1 : 1 pièce, on augmentera à chaque nuit

    const char* roomNames[MAX_ROOMS] = {"Cuisine", "Grenier", "Salon", "Garage"};
    const char* roomImages[MAX_ROOMS] = {
        "../assets/haunted/rooms/kitchen.png",
        "../assets/haunted/rooms/attic.png",
        "../assets/haunted/rooms/livingroom.png",
        "../assets/haunted/rooms/garage.png"
    };

    for (int i = 0; i < *roomCount; i++) {
        rooms[i].name = roomNames[i];

        SDL_Surface* surface = IMG_Load(roomImages[i]);
        if (!surface) {
            printf("Erreur chargement image pièce: %s\n", IMG_GetError());
            continue;
        }

        rooms[i].texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        // Positionner les pièces sur l'écran
        rooms[i].rect.x = 200 + i * (ROOM_WIDTH + 50);
        rooms[i].rect.y = 300;
        rooms[i].rect.w = ROOM_WIDTH;
        rooms[i].rect.h = ROOM_HEIGHT;
    }
}

void renderRoomSelection(Room rooms[], int roomCount, SDL_Renderer* renderer, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255};

    for (int i = 0; i < roomCount; i++) {
        SDL_RenderCopy(renderer, rooms[i].texture, NULL, &rooms[i].rect);

        SDL_Surface* textSurface = TTF_RenderUTF8_Solid(font, rooms[i].name, white);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

        SDL_Rect textRect = {rooms[i].rect.x + 50, rooms[i].rect.y + ROOM_HEIGHT + 10, 200, 50};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }

    SDL_RenderPresent(renderer);
}

int handleRoomSelection(Room rooms[], int roomCount) {
    SDL_Event event;
    int mouseX, mouseY;

    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return -1;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_GetMouseState(&mouseX, &mouseY);
                    for (int i = 0; i < roomCount; i++) {
                        if (mouseX >= rooms[i].rect.x && mouseX <= rooms[i].rect.x + rooms[i].rect.w &&
                            mouseY >= rooms[i].rect.y && mouseY <= rooms[i].rect.y + rooms[i].rect.h) {
                            return i; // Retourne l’index de la pièce choisie
                        }
                    }
                }
                break;
        }
    }

    return -1;
}

bool initMenu(Menu* menu, SDL_Rect optionRects[NUM_OPTIONS]) {


    if (SDL_Init(SDL_INIT_VIDEO || SDL_INIT_AUDIO) < 0) {  
        printf("Erreur SDL: %s\n", SDL_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {  
        printf("Erreur SDL_image: %s\n", IMG_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        printf("Erreur TTF: %s\n", TTF_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {  
        printf("Erreur SDL_mixer: %s\n", Mix_GetError());
        return false;
    }

    menu->window = SDL_CreateWindow("Menu Principal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!menu->window) {
        printf("Erreur création fenêtre: %s\n", SDL_GetError());
        return false;
    }

    menu->renderer = SDL_CreateRenderer(menu->window, -1, SDL_RENDERER_ACCELERATED);
    if (!menu->renderer) {
        printf("Erreur création renderer: %s\n", SDL_GetError());
        return false;
    }

    menu->font = TTF_OpenFont("../assets/blood_crow/bloodcrow.ttf", 48);
    if (!menu->font) {
        printf("Erreur chargement police: %s\n", TTF_GetError());
        return false;
    }

        
    menu->music = Mix_LoadMUS("../assets/haunted_sounds/main_theme.ogg");
    if (!menu->music) {
        printf("Erreur chargement musique: %s\n", Mix_GetError());
    } 

    else {
        Mix_PlayMusic(menu->music, -1);  
    }



        //  Chargement des sons
        menu->soundSelect = Mix_LoadWAV("../assets/haunted_sounds/launch.wav");
        menu->soundClick = Mix_LoadWAV("../assets/haunted_sounds/click.wav");
        menu->soundBack = Mix_LoadWAV("../assets/haunted_sounds/return.wav");
    
        if (!menu->soundSelect || !menu->soundClick || !menu->soundBack) {
            printf("Erreur chargement des sons: %s\n", Mix_GetError());
        }

            //  Chargement de l'image de fond
    SDL_Surface* bgSurface = IMG_Load("../assets/haunted/background.png");
    if (!bgSurface) {
        printf("Erreur chargement image: %s\n", IMG_GetError());
    } else {
        menu->background = SDL_CreateTextureFromSurface(menu->renderer, bgSurface);
        SDL_FreeSurface(bgSurface);
    }

    menu->selectedOption = 0;

    // Initialisation des zones cliquables
    for (int i = 0; i < NUM_OPTIONS; i++) {
        optionRects[i].x = 300;
        optionRects[i].y = 200 + i * 80;
        optionRects[i].w = 200;  // Valeur arbitraire
        optionRects[i].h = 50;   // Valeur arbitraire
    }

    return true;
}

void renderMenu(Menu* menu, SDL_Rect optionRects[NUM_OPTIONS]) {
    SDL_SetRenderDrawColor(menu->renderer, 0, 0, 0, 255);
    SDL_RenderClear(menu->renderer);

    SDL_Color white = {255, 255, 255};
    SDL_Color red = {255, 0, 0};

        // ✅ Afficher l'image de fond
        if (menu->background) {
            SDL_RenderCopy(menu->renderer, menu->background, NULL, NULL);
        }

    for (int i = 0; i < NUM_OPTIONS; i++) {
        SDL_Color color = (i == menu->selectedOption) ? red : white;
        SDL_Surface* textSurface = TTF_RenderUTF8_Solid(menu->font, options[i], color);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(menu->renderer, textSurface);

        SDL_RenderCopy(menu->renderer, textTexture, NULL, &optionRects[i]);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }

    SDL_RenderPresent(menu->renderer);
}

void handleMenuInput(Menu* menu, SDL_Rect optionRects[NUM_OPTIONS], bool* running, bool* startGame, bool* inSettings) {
    SDL_Event event;
    int mouseX, mouseY;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                *running = false;
                break;

            case SDL_MOUSEMOTION:
                SDL_GetMouseState(&mouseX, &mouseY);
                for (int i = 0; i < NUM_OPTIONS; i++) {
                    if (mouseX >= optionRects[i].x && mouseX <= optionRects[i].x + optionRects[i].w &&
                        mouseY >= optionRects[i].y && mouseY <= optionRects[i].y + optionRects[i].h) {
                        
                        if(menu->selectedOption != i){
                            menu->selectedOption = i;
                            Mix_PlayChannel(-1, menu->soundSelect, 0);  
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    switch (menu->selectedOption) {
                        case 0:  // Nouvelle Partie
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            printf("Nouvelle Partie\n");
                            *startGame = true;
                            break;
                        case 1:  // Charger
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            printf("Charger partie\n");
                            break;
                        case 2:  // Sauvegarder
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            printf("Sauvegarder partie\n");
                            break;
                        case 3:  // Extras
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            printf("Extras\n");
                            break;
                        case 4:  // Crédits
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            printf("Crédits\n");
                            break;
                        case 5:  // Paramètres
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            printf("Accès aux Paramètres\n");
                            *inSettings = true;  // Entrée dans le sous-menu des paramètres
                            break;
                        case 6:  // Quitter
                            Mix_PlayChannel(-1, menu->soundClick, 0);  
                            *running = false;
                            break;
                    }
                }
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_z:
                        menu->selectedOption = (menu->selectedOption - 1 + NUM_OPTIONS) % NUM_OPTIONS;
                        Mix_PlayChannel(-1, menu->soundSelect, 0);
                        break;
                    case SDLK_s:
                        menu->selectedOption = (menu->selectedOption + 1) % NUM_OPTIONS;
                        Mix_PlayChannel(-1, menu->soundSelect, 0);
                        break;
                    case SDLK_RETURN:
                    Mix_PlayChannel(-1, menu->soundClick, 0);  
                        switch (menu->selectedOption) {
                            case 0:  // Nouvelle Partie
                                printf("Nouvelle Partie\n");
                                *startGame = true;
                                break;
                            case 1:  // Charger
                                printf("Charger partie\n");
                                break;
                            case 2:  // Sauvegarder
                                printf("Sauvegarder partie\n");
                                break;
                            case 3:  // Extras
                                printf("Extras\n");
                                break;
                            case 4:  // Crédits
                                printf("Crédits\n");
                                break;
                            case 5:  // Paramètres
                                printf("Accès aux Paramètres\n");
                                *inSettings = true;
                                break;
                            case 6:  // Quitter
                                *running = false;
                                break;
                            
                        }
                        break;
                    case SDLK_ESCAPE:
                        Mix_PlayChannel(-1, menu->soundBack, 0);  
                        *inSettings = false;  // Sort du sous-menu
                        break;
                }
                break;
        }
    }
}


void cleanupMenu(Menu* menu) {
    if (menu->background) {
        SDL_DestroyTexture(menu->background);
    }

    if (menu->music) {
        Mix_HaltMusic();
        Mix_FreeMusic(menu->music);
    }
    if (menu->soundSelect) Mix_FreeChunk(menu->soundSelect);
    if (menu->soundClick) Mix_FreeChunk(menu->soundClick);
    if (menu->soundBack) Mix_FreeChunk(menu->soundBack);

    Mix_CloseAudio();
    TTF_CloseFont(menu->font);
    IMG_Quit();
    SDL_DestroyRenderer(menu->renderer);
    SDL_DestroyWindow(menu->window);
    TTF_Quit();
    SDL_Quit();
}

void fadeToBlack(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int alpha = 0; alpha <= 255; alpha += 5) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
        SDL_RenderFillRect(renderer, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(10); // Ajuste la vitesse du fondu
    }
}


int handleCameraMovement(SDL_Event *event) {
    static int lastMouseX = SCREEN_WIDTH / 2;  // Sauvegarder la dernière position de la souris
    int mouseX;
    SDL_GetMouseState(&mouseX, NULL); // Récupérer la position actuelle de la souris

    // Calculer la différence entre la position actuelle et la précédente
    int offsetX = (mouseX - lastMouseX) / 4; // Réduction de la vitesse du mouvement (ajustable)

    // Limiter le déplacement horizontal
    if (offsetX < -300) offsetX = -300; // Limite de déplacement vers la gauche
    if (offsetX > 300) offsetX = 300;   // Limite de déplacement vers la droite

    // Mettre à jour la dernière position de la souris pour la prochaine itération
    lastMouseX = mouseX;

    return offsetX;
}

void startRoomGameplay(Room room, SDL_Renderer* renderer) {
    SDL_Event event;
    bool inRoom = true;
    int mouseX, mouseY;
    room.cameraX = 0;  // Position initiale de la caméra

    while (inRoom) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    inRoom = false;
                    break;
            }
        }

        // Récupérer le décalage de la caméra basé sur le mouvement de la souris
        int cameraOffset = handleCameraMovement(&event);

        // Déplacer la caméra horizontalement
        room.cameraX += cameraOffset;

        // Limiter le mouvement de la caméra si nécessaire
        int roomWidth = 2000; // Largeur de la pièce (à ajuster en fonction de ta scène)
        if (room.cameraX < 0) room.cameraX = 0;  
        if (room.cameraX > roomWidth - SCREEN_WIDTH) room.cameraX = roomWidth - SCREEN_WIDTH;

        // Effacer l'écran et afficher la scène
        SDL_RenderClear(renderer);
        
        SDL_Rect srcRect = { room.cameraX, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_Rect dstRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        
        // Afficher la scène
        SDL_RenderCopy(renderer, room.texture, &srcRect, &dstRect);

        // Mettre à jour l'affichage
        SDL_RenderPresent(renderer);
    }
}

int WinMain(int argc , char**argv) {
    Menu menu;
    SDL_Rect optionRects[NUM_OPTIONS];  
    bool running = true;
    bool startGame = false;
    bool inSettings = false;

    if (!initMenu(&menu, optionRects)) {
        return 1;
    }

    while (running) {
        if(!inSettings){
            handleMenuInput(&menu, optionRects, &running, &startGame,&inSettings);
            renderMenu(&menu, optionRects);
        }
        else{
            printf("Sous-menu des paramètres\n");
        }
        if (startGame) {
            printf("Lancement du jeu...\n");
            // Charger les pièces
            Room rooms[MAX_ROOMS];
            int roomCount = 0;
            loadRooms(rooms, &roomCount, menu.renderer);
            int chosenRoom = -1;
            while (chosenRoom == -1) {
                renderRoomSelection(rooms, roomCount, menu.renderer, menu.font);
                chosenRoom = handleRoomSelection(rooms, roomCount);
            }
            printf("Le joueur a choisi : %s\n", rooms[chosenRoom].name);
            fadeToBlack(menu.renderer);

            // Lancer la pièce sélectionnée
            startRoomGameplay(rooms[chosenRoom], menu.renderer);
            break; // Pour l'instant, on quitte après la sélection;
        }
    }

    cleanupMenu(&menu);
    return 0;
}