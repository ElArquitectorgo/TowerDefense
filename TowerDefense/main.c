#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_image.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define DELAY 20
#define NUM_POINTS 50
#define ENEMY_SIZE 40
#define PATH_SIZE 900
#define NUM_TURRETS 4

typedef struct {
    float x, y;
} Point;

typedef struct enemie {
    float x;
    float y;
    int w;
    int h;
    struct enemie* next;
    int path_index;
    int health;
} Enemie;

typedef struct {
    float x;
    float y;
    int w;
    int h;
    Enemie* target;
    int delay;
} Turret;

typedef struct bullet {
    int path_index;
    Point trayectory[10];
    Enemie* target;
    struct bullet* next;
} Bullet;

Enemie* enemies_head;
Enemie* enemies_tail;

Bullet* bullets_head;
Bullet* bullets_tail;

Point path[PATH_SIZE];
Turret turrets[NUM_TURRETS];

typedef struct button {
    int x, y;
    int w, h;
    Uint8 r, g, b;
} Button;

Button b = { 50, 650, 50, 50, 0xff, 0xff, 0xff };

bool is_inside_button(Button* button, int x, int y) {
    return !(x < button->x || x > button->x + button->w ||
        y < button->y || y > button->y + button->h);
}

void render_button(SDL_Renderer* renderer, Button* button) {
    SDL_SetRenderDrawColor(renderer, button->r, button->g, button->b, 255);
    SDL_Rect rect = { button->x, button->y, button->w, button->h };
    SDL_RenderFillRect(renderer, &rect);
}

void render_enemies(SDL_Renderer* renderer, SDL_Texture* texture) {
    //SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 255);

    Enemie* track = enemies_head;

    while (track != NULL) {
        SDL_Rect rect;
        rect.x = track->x;
        rect.y = track->y;
        rect.w = track->w;
        rect.h = track->h;
        //SDL_RenderFillRect(renderer, &rect);
        SDL_RenderCopy(renderer, texture, NULL, &rect);

        track = track->next;
    }  
}

void render_turrets(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, 255);

    for (int i = 0; i < NUM_TURRETS; i++) {
        SDL_Rect rect;
        rect.x = turrets[i].x;
        rect.y = turrets[i].y;
        rect.w = turrets[i].w;
        rect.h = turrets[i].h;
        SDL_RenderFillRect(renderer, &rect);
        //SDL_RenderCopy(renderer, texture, NULL, &rect);
    }
}

void render_bullets(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 255);

    Bullet* track = bullets_head;

    while (track != NULL) {
        SDL_Rect rect;
        rect.x = track->trayectory[track->path_index].x;
        rect.y = track->trayectory[track->path_index].y;
        rect.w = 5;
        rect.h = 5;
        SDL_RenderFillRect(renderer, &rect);

        track = track->next;
    }
}

Point compute_bezier(Point P0, Point P1, Point P2, float t) {
    Point result;

    result.x = (1 - t) * (1 - t) * P0.x + 2 * t * (1 - t) * P1.x + t * t * P2.x;
    result.y = (1 - t) * (1 - t) * P0.y + 2 * t * (1 - t) * P1.y + t * t * P2.y;

    return result;
}

Point compute_linear_bezier(Point P0, Point P1, float t) {
    Point result;

    result.x = P0.x + (P1.x - P0.x) * t;
    result.y = P0.y + (P1.y - P0.y) * t;

    return result;
}

void get_curve(Point p0, Point p1, Point p2, Point* points) {
    for (int i = 0; i < NUM_POINTS; i++) {
        float t = (float)i / (NUM_POINTS - 1);
        Point p = compute_bezier(p0, p1, p2, t);
        points[i] = p;
    }
}

void get_line(Point p0, Point p1, Point* points)
{
    for (int i = 0; i < 10; i++)
    {
        float t = (float)i / (10 - 1);
        Point p = compute_linear_bezier(p0, p1, t);
        points[i] = p;
    }
}

void generate_path() {
    bool finished = false;
    int i = 0;

    while (!finished) {
        Point p;
        if (i < 150) {
            p.x = i;
            p.y = 200;
        }
        else if (i == 150) {
            Point p0 = { 150, 200 };
            Point p1 = { 200, 200 };
            Point p2 = { 200, 250 };

            Point points[NUM_POINTS];
            get_curve(p0, p1, p2, points);

            for (int j = 0; j < NUM_POINTS; j++) {
                p = points[j];
                path[i] = p;
                i++;
            }
            
        }
        else if (i > 200 && i < 300) {
            p.x = 200;
            p.y = i + 50;
        }
        else if (i == 300) {
            Point p0 = { 200, 350 };
            Point p1 = { 200, 400 };
            Point p2 = { 250, 400 };

            Point points[NUM_POINTS];
            get_curve(p0, p1, p2, points);

            for (int j = 0; j < NUM_POINTS; j++) {
                p = points[j];
                path[i] = p;
                i++;
            }
        }
        else {
            p.x = i - 100;
            p.y = 400;
        }
        path[i] = p;
        i++;

        if (i == PATH_SIZE) {
            finished = true;
        }
    }
}

void move_enemies() {
    Enemie* track = enemies_head;

    while (track != NULL) {
        if (track->path_index < PATH_SIZE) {
            track->x = path[track->path_index].x;
            track->y = path[track->path_index].y;
            track->path_index += 1;
        }
        track = track->next;
    }
}

void add_bullet(Point* points, Enemie* enemie) {
    Bullet* new = malloc(sizeof(Bullet));
    new->path_index = 0;
    new->target = enemie;
    new->next = NULL;

    for (int i = 0; i < 10; i++) {
        new->trayectory[i] = points[i];
    }

    if (bullets_head == NULL) {
        bullets_head = new;
        bullets_tail = new;
    }
    else {
        bullets_tail->next = new;
        bullets_tail = new;
    }
}

void add_enemy() {
    Enemie* new = malloc(sizeof(Enemie));
    new->x = 0;
    new->y = 200;
    new->w = ENEMY_SIZE;
    new->h = ENEMY_SIZE;
    new->path_index = 0;
    new->health = 30;
    new->next = NULL;

    if (enemies_head == NULL) {
        enemies_head = new;
        enemies_tail = new;
    }
    else {
        enemies_tail->next = new;
        enemies_tail = new;
    }
}

int add_enemies(void* data) {
    for (int i = 0; i < 10; i++) {
        SDL_Delay(1000);
        add_enemy();
    }   

    return 0;
}

int get_ranged_rand(int min, int max) {
    return ((double)rand() / RAND_MAX) * (max - min) + min;
}

void kill_random_enemie() {
    if (enemies_head == NULL) {
        return;
    }

    int r = get_ranged_rand(0, 10);
    printf("%d\n", r);

    if (r == 0) {
        Enemie* temp = enemies_head;
        enemies_head = enemies_head->next;
        free(temp);
        return;
    }

    Enemie* track = enemies_head;

    int i = 0;
    while (track->next != NULL) {
        if (i == r - 1) {
            Enemie* temp = track->next;
            track->next = track->next->next;
            free(temp);
            break;
        }
        track = track->next;
        i++;
    }
}

void kill(Enemie* enemie) {
    Enemie* track = enemies_head;

    if (track == enemie) {
        Enemie* temp = enemies_head;
        enemies_head = enemies_head->next;
        free(temp);
        return;
    }

    while (track != NULL) {
        if (track->next == enemie) {
            Enemie* temp = track->next;
            track->next = track->next->next;
            free(temp);
            break;
        }
        track = track->next;
    }
}

void init_turrets() {
    Turret t1 = {
        100,
        100,
        ENEMY_SIZE,
        ENEMY_SIZE,
        NULL,
        0
    };
    Turret t2 = {
        300,
        300,
        ENEMY_SIZE,
        ENEMY_SIZE,
        NULL,
        0
    };
    Turret t3 = {
        200,
        500,
        ENEMY_SIZE,
        ENEMY_SIZE,
        NULL,
        0
    };
    Turret t4 = {
        600,
        350,
        ENEMY_SIZE,
        ENEMY_SIZE,
        NULL,
        0
    };
    turrets[0] = t1;
    turrets[1] = t2;
    turrets[2] = t3;
    turrets[3] = t4;
}

void search_targets() {
    for (int i = 0; i < NUM_TURRETS; i++) {
        if (turrets[i].target != NULL) {
            float x = turrets[i].target->x;
            float y = turrets[i].target->y;

            float dx = turrets[i].x - x;
            float dy = turrets[i].y - y;

            if (sqrt(dx * dx + dy * dy) > 200) {
                turrets[i].target = NULL;
                break;
            }
        }
        else {
            Enemie* track = enemies_head;

            while (track != NULL) {
                float x = track->x;
                float y = track->y;

                float dx = turrets[i].x - x;
                float dy = turrets[i].y - y;

                float d = sqrt(dx * dx + dy * dy);

                if (d <= 200) {
                    turrets[i].target = track;
                    break;
                }
                track = track->next;
            }
        }
    }
}

void set_bullet_trayectory(Turret turret, Enemie* enemie) {
    Point p1 = { turret.x + turret.w / 2, turret.y + turret.h / 2 };
    Point p2;
    if (enemie->path_index + 10 < PATH_SIZE) {
        p2.x = path[enemie->path_index + 10].x + enemie->w / 2;
        p2.y = path[enemie->path_index + 10].y + enemie->h / 2;
    }
    else {
        p2.x = path[enemie->path_index].x + enemie->w / 2;
        p2.y = path[enemie->path_index].y + enemie->h / 2;
    }

    Point points[10];
    get_line(p1, p2, points);
    add_bullet(points, enemie);
}

void shoot() {
    for (int i = 0; i < NUM_TURRETS; i++) {
        if (turrets[i].target != NULL && turrets[i].delay == 0) {
            set_bullet_trayectory(turrets[i], turrets[i].target);
        }
    }
}

void update_turrets_delay() {
    for (int i = 0; i < NUM_TURRETS; i++) {
        if (turrets[i].delay == 0) {
            turrets[i].delay = 80;
        }
        else {
            turrets[i].delay -= 1;
        }
    }
}

void kill_bullet(Bullet* bullet) {
    Bullet* track = bullets_head;

    if (track == bullet) {
        Bullet* temp = bullets_head;
        bullets_head = bullets_head->next;
        free(temp);
        return;
    }

    while (track != NULL) {
        if (track->next == bullet) {
            Bullet* temp = track->next;
            track->next = track->next->next;
            free(temp);
            break;
        }
        track = track->next;
    }
}

void damage(Bullet* bullet) {
    bullet->target->health -= 10;
    if (bullet->target->health == 0) {
        kill(bullet->target);
    }
}

void update_bullets() {
    if (bullets_head == NULL) {
        return;
    }

    Bullet* track = bullets_head;
    Bullet* prev = NULL;

    while (track != NULL) {
        track->path_index += 1;

        if (track->path_index >= 10) {
            damage(track);

            Bullet* temp = track;

            if (prev == NULL) {
                bullets_head = track->next;
            }
            else {
                prev->next = track->next;
            }

            track = track->next;
            free(temp);
        }
        else {
            prev = track;
            track = track->next;
        }
    }
}

int main() {
    generate_path();
    init_turrets();
    SDL_Thread* thread = SDL_CreateThread(add_enemies, "Add enemies", (void*)0);
    
    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_INIT_VIDEO < 0) {
        fprintf(stderr, "ERROR: SDL_INIT_VIDEO");
    }

    window = SDL_CreateWindow(
        "Torre Defensa",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );

    if (!window) {
        fprintf(stderr, "ERROR: !window");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        fprintf(stderr, "!renderer");
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);


    SDL_Texture* enemyTexture = IMG_LoadTexture(renderer, "enemy.png");
    SDL_Texture* turretTexture = IMG_LoadTexture(renderer, "turret_base.png");
  
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_k:
                    kill_random_enemie();
                    break;
                }
            case SDL_MOUSEMOTION:
                if (is_inside_button(&b, event.button.x, event.button.y)) {
                    b.r = 0x00;
                }
                else {
                    b.r = 0xff;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                printf("%d, %d\n", event.button.x, event.button.y);
                bool is_inside = is_inside_button(&b, event.button.x, event.button.y);
                printf("%d", is_inside);
                break;
            }
        }

        SDL_RenderClear(renderer);

        //RENDER LOOP START
        render_button(renderer, &b);
        render_enemies(renderer, enemyTexture);
        render_turrets(renderer, turretTexture);
        render_bullets(renderer);
        move_enemies();
        search_targets();
        shoot();
        update_turrets_delay();
        update_bullets();

        //RENDER LOOP END
        SDL_SetRenderDrawColor(renderer, 0x11, 0x11, 0x11, 255);
        SDL_RenderPresent(renderer);
        
        SDL_Delay(DELAY);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}