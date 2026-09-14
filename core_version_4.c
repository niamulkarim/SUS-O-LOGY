#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game_version_4.h"

#define SCREEN_W 1920
#define SCREEN_H 1080

//researve a slot for 5 sec 
#define INTRO_ANIM_SECONDS 5.0f

// for all the sceen 
typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_SETTINGS,       
    SCREEN_RULES,           
    SCREEN_PLAYER_SETUP,
    SCREEN_INTRO_ANIM,     
    SCREEN_ROLE_REVEAL,
    SCREEN_DAY_VOTE,
    SCREEN_DAY_RESULT,
    SCREEN_NIGHT_ACTION,
    SCREEN_NIGHT_RESULT,
    SCREEN_ZOMBIE_REVEAL,  
    SCREEN_GAME_OVER,
    SCREEN_LEADERBOARD
} AppScreen;


typedef struct {
    Texture2D menu;
    Texture2D settings;             
    Texture2D rules;                 
    Texture2D setup;
    Texture2D introAnim;             
    Texture2D roleRevealVillager;    
    Texture2D roleRevealGhost;      
    Texture2D roleRevealWizard;     
    Texture2D dayVote;
    Texture2D dayResult;
    Texture2D nightAction;
    Texture2D nightResult;
    Texture2D zombieReveal;         
    Texture2D gameOverVillagerWin;
    Texture2D gameOverGhostWin;
    Texture2D leaderboard;
    Texture2D start;
} Backgrounds;

#define INTRO_FPS 30
#define INTRO_FRAME_COUNT 150  
//150 frames ÷ 30 FPS = 5 seconds
Texture2D introFrames[INTRO_FRAME_COUNT]; //array 5sec anim -150 image - 30i/s 

static Backgrounds load_backgrounds(void) {
    Backgrounds bg;
    
    bg.menu                 = LoadTexture("assets/image/background/SUS.png");
    bg.settings              = LoadTexture("assets/image/background/settings.png");
    bg.rules                 = LoadTexture("assets/image/background/download.png");
    bg.setup                 = LoadTexture("assets/image/background/role_assign2.png");

    //for the introoo
    for (int i = 0; i < INTRO_FRAME_COUNT; i++) {
        char filename[100];
        sprintf(filename, "assets/image/background/intro/frame%03d.png", i + 1);
        introFrames[i] = LoadTexture(filename);
    }
    
    bg.roleRevealVillager    = LoadTexture("assets/image/background/villager_role.png");
    bg.roleRevealGhost       = LoadTexture("assets/image/background/ghost_role.png");
    bg.roleRevealWizard      = LoadTexture("assets/image/background/wizard_role.png");
    bg.dayVote               = LoadTexture("assets/image/background/day_voting.png");
    bg.dayResult             = LoadTexture("assets/image/background/day_voting.png");
    bg.nightAction           = LoadTexture("assets/image/background/night_action2.png");
    bg.nightResult           = LoadTexture("assets/image/background/night_action2.png");
    bg.zombieReveal          = LoadTexture("assets/image/background/turend.png");
    bg.gameOverVillagerWin   = LoadTexture("assets/image/background/villager_win.png");
    bg.gameOverGhostWin      = LoadTexture("assets/image/background/ghost_win.png");
    bg.leaderboard           = LoadTexture("assets/image/background/leaderboard.png");
    bg.start                 = LoadTexture("assets/image/background/start1.png");
   
    return bg;
}

static void unload_backgrounds(Backgrounds *bg) {
    UnloadTexture(bg->menu);
    UnloadTexture(bg->settings);
    UnloadTexture(bg->rules);
    UnloadTexture(bg->setup);
    //for the introooo
    for (int i = 0; i < INTRO_FRAME_COUNT; i++) {
    UnloadTexture(introFrames[i]);
    }
    UnloadTexture(bg->roleRevealVillager);
    UnloadTexture(bg->roleRevealGhost);
    UnloadTexture(bg->roleRevealWizard);
    UnloadTexture(bg->dayVote);
    UnloadTexture(bg->dayResult);
    UnloadTexture(bg->nightAction);
    UnloadTexture(bg->nightResult);
    UnloadTexture(bg->zombieReveal);
    UnloadTexture(bg->gameOverVillagerWin);
    UnloadTexture(bg->gameOverGhostWin);
    UnloadTexture(bg->leaderboard);
    UnloadTexture(bg->start);
}

//background immage setup ..all the pics are in perfect size 
static void draw_background(Texture2D tex) {
    if (tex.width > 0) {
        DrawTexture(tex, 0, 0, WHITE);
    }
} 

//create button + checks hovvering + checks clicks 
static bool button(Rectangle rect, const char *label, Vector2 mouse, int fontSize) {
    bool hover = CheckCollisionPointRec(mouse, rect);
    Color fill = hover ? (Color){230, 41, 55, 255} : (Color){110, 25, 40, 255};
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);
    int textWidth = MeasureText(label, fontSize);
    DrawText(label, (int)(rect.x + (rect.width - textWidth) / 2),
              (int)(rect.y + (rect.height - fontSize) / 2), fontSize, BLACK);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}//returns tf 

//buttons but with selected highlight used in day vottingg 
static bool button_selectable(Rectangle rect, const char *label, Vector2 mouse, int fontSize, bool selected) {
    bool hover = CheckCollisionPointRec(mouse, rect);
    Color fill;
    if (selected)      fill = (Color){230, 41, 55, 255};   //shk recommanded 
    else if (hover)    fill = (Color){128, 128, 128, 255};
    else               fill = (Color){110, 25, 40, 255};
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, selected ? 3 : 2, DARKGRAY);
    int textWidth = MeasureText(label, fontSize);
    DrawText(label, (int)(rect.x + (rect.width - textWidth) / 2),
              (int)(rect.y + (rect.height - fontSize) / 2), fontSize, BLACK);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}//returns tf 

//list of available players 
static int player_button_list(const GameState *gs, int excludeId, int villagerSideOnly,
                                Vector2 mouse, float startX, float startY, int selectedId) {
    int clicked = -1;
    float x = startX, y = startY;
    int col = 0;
    for (int i = 0; i < gs->player_count; i++) {
        const Player *p = &gs->players[i];
        if (i == excludeId) continue;
        if (p->status != ALIVE) continue;
        if (villagerSideOnly && !is_villager_side(p)) continue;

        Rectangle rect = { x, y, 360, 70 };
        if (button_selectable(rect, p->name, mouse, 28, i == selectedId)) clicked = i;

        y += 90;
        col++;
        if (col == 6) { col = 0; y = startY; x += 400; }
    }
    return clicked;
}//return the id of the clicked  player 

//bot votes for selecting ghost 
static int bot_pick_random_alive(const GameState *gs, int excludeId, int villagerSideOnly) {
    int candidates[MAX_PLAYERS], n = 0;
    for (int i = 0; i < gs->player_count; i++) {
        const Player *p = &gs->players[i];
        if (i == excludeId) continue;
        if (p->status != ALIVE) continue;
        if (villagerSideOnly && !is_villager_side(p)) continue;
        candidates[n++] = i;
    }
    if (n == 0) return -1;
    return candidates[rand() % n];//generates a random number 
}

//makes it easy when there is no need to take vote 
static void resolve_day_vote_now(GameState *gs, int humanChoice, int *lastEliminated) {
    int votes[MAX_PLAYERS];
    for (int i = 0; i < gs->player_count; i++) votes[i] = -1;

    for (int i = 0; i < gs->player_count; i++) {
        Player *p = &gs->players[i];
        if (p->status != ALIVE || p->can_vote != 1) continue;
        if (i == gs->human_id) {
            votes[i] = humanChoice;
        } else {
            votes[i] = bot_pick_random_alive(gs, i, 0);
        }
    }

    int eliminated = run_day_vote(gs, votes);
    *lastEliminated = eliminated;

    if (eliminated >= 0) {
        for (int i = 0; i < gs->player_count; i++) {
            if (votes[i] < 0) continue;
            if (gs->players[eliminated].role == GHOST) {
                if (votes[i] == eliminated) score_correct_vote(gs, i);
            } else {
                score_incorrect_vote(gs, i);
            }
        }
    }
}

//all the night actions 
static void resolve_night_now(GameState *gs, int humanGhostTarget, int humanWizardTarget,
                                int humanZombieTarget, int *lastKilled, int *lastProtected,
                                int *lastZombieKilled, bool *justBecameActiveZombie) {
    Player *ghost = find_player(gs, gs->ghost_id);
    if (ghost && ghost->status == ALIVE) {
        int target = (ghost->is_human) ? humanGhostTarget
                                        : bot_pick_random_alive(gs, gs->ghost_id, 1);
        if (target >= 0) ghost_choose_kill(gs, target);
    }

    Player *wizard = find_player(gs, gs->wizard_id);
    if (wizard && wizard->status == ALIVE) {
        int target = (wizard->is_human) ? humanWizardTarget
                                         : bot_pick_random_alive(gs, -1, 1);
        if (target >= 0) wizard_choose_protect(gs, target);
    }

    
    Player *human = &gs->players[gs->human_id];
    if (human->status == ALIVE && human->is_active_zombie) {
        if (humanZombieTarget >= 0) zombie_choose_kill(gs, humanZombieTarget);
    }

    int protectedId = -1, zombieKilled = -1;
    int killed = resolve_night(gs, &protectedId, &zombieKilled);
    *lastKilled = killed;
    *lastProtected = protectedId;
    *lastZombieKilled = zombieKilled;

     //Human killed → becomes Zombie → Ghost can no longer target them.
    *justBecameActiveZombie = (killed == gs->human_id);

    //ghost and wizzerd both can score 
    if (killed >= 0) score_ghost_kill(gs);
    if (protectedId >= 0) score_wizard_protect(gs);

    score_round_survival(gs);
}

//either human can vote or cannot 
static void advance_to_day_vote(GameState *gs, AppScreen *screen, int *lastEliminated, int *selectedVoteTarget) {
    *selectedVoteTarget = -1;
    Player *human = &gs->players[gs->human_id];
    if (human->status == ALIVE && human->can_vote) {
        *screen = SCREEN_DAY_VOTE;
    } else {
        resolve_day_vote_now(gs, -1, lastEliminated);
        *screen = SCREEN_DAY_RESULT;
    }
}

//maiin 

int main(void) {
    srand((unsigned int)time(NULL));
    //the seed changes based on the current time

    InitWindow(SCREEN_W, SCREEN_H, "SUS-0-LOGY");
    SetTargetFPS(60);
    InitAudioDevice();
    //SOUND INPUT 
    Sound introStartSound = LoadSound("assets/audio/sus_ending.wav");
    Sound introEndSound   = LoadSound("assets/audio/sus_start.wav");

    //my own implimentation......proud of it 
    Texture2D yesBackground      = LoadTexture("assets/image/background/start1.png");
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(yesBackground, 0, 0, WHITE);
    const char *loadingMsg = "Something Is Awakening, The Ritual is Underway.....";
    DrawText(loadingMsg, SCREEN_W/2 - MeasureText(loadingMsg, 56)/2,830, 56, RED);
    EndDrawing();
    UnloadTexture(yesBackground);
    WaitTime(0.5);
    
    Backgrounds bg = load_backgrounds();

    AppScreen screen = SCREEN_MAIN_MENU;
    PlaySound(introEndSound);
    GameState gs;
    memset(&gs, 0, sizeof(gs));//clean initial state 

    //
    char playerNameInputs[MAX_PLAYERS][MAX_NAME_LEN];
    for (int i = 0; i < MAX_PLAYERS; i++) playerNameInputs[i][0] = '\0';
    int activeNameField = -1; //keyboard focus 

    int setupPlayerCount = 7;
    int lastEliminated = -1;
    int lastKilled = -1, lastProtected = -1, lastZombieKilled = -1;
    int humanGhostTarget = -1, humanWizardTarget = -1, humanZombieTarget = -1;
    int selectedVoteTarget = -1; 
    bool justBecameActiveZombie = false; 
    float introAnimTimer = 0.0f; 
    bool quitRequested = false;

    while (!WindowShouldClose() && !quitRequested) {

        Vector2 mouse = GetMousePosition();

        //name input 
        if (screen == SCREEN_PLAYER_SETUP && activeNameField >= 0 && activeNameField < setupPlayerCount) {
            int key = GetCharPressed();
            while (key > 0) {
                int len = (int)strlen(playerNameInputs[activeNameField]);
                if (key >= 32 && key <= 125 && len < MAX_NAME_LEN - 1) {
                    playerNameInputs[activeNameField][len] = (char)key;
                    playerNameInputs[activeNameField][len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = (int)strlen(playerNameInputs[activeNameField]);
                if (len > 0) playerNameInputs[activeNameField][len - 1] = '\0';
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (screen) {

         
            case SCREEN_MAIN_MENU: {
                draw_background(bg.menu);
                if (button((Rectangle){SCREEN_W/2 - 150, 500, 300, 70}, "Draw Your Fate", mouse, 30)) {
                    screen = SCREEN_PLAYER_SETUP;
                }
                if (button((Rectangle){SCREEN_W/2 - 150, 585, 300, 70}, "Settings", mouse, 30)) {
                    screen = SCREEN_SETTINGS;
                }
                if (button((Rectangle){SCREEN_W/2 - 150, 670, 300, 70}, "Sacred Rules", mouse, 30)) {
                    screen = SCREEN_RULES;
                }
               
                if (button((Rectangle){SCREEN_W/2 - 150, 755, 300, 70}, "The Crews", mouse, 30)) {
                }
                if (button((Rectangle){SCREEN_W/2 - 150, 840, 300, 70}, "Achievements", mouse, 30)) {
                }
                if (button((Rectangle){SCREEN_W/2 - 150, 925, 300, 70}, "Walkaway", mouse, 30)) {
                    quitRequested = true;
                }

               //the tiny version namee at the bottom corner 
                DrawText("v4.4.1", SCREEN_W - 100, SCREEN_H - 40, 30, GRAY);
                break;
            }

            /* ============================================================ */
            case SCREEN_SETTINGS: {
                draw_background(bg.settings);
              
                if (button((Rectangle){SCREEN_W/2 - 200, 390, 400, 70}, "Sound: ON", mouse, 28)) {
                }
                if (button((Rectangle){SCREEN_W/2 - 200, 600, 400, 70}, "Fullscreen: OFF", mouse, 28)) {
                }

                if (button((Rectangle){SCREEN_W/2 - 130, 835, 300, 80}, "Back", mouse, 32)) {
                    screen = SCREEN_MAIN_MENU;
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_RULES: {
                draw_background(bg.rules);
               
                int y = 200;
              
                if (button((Rectangle){SCREEN_W/2 - 200, 980, 400, 80}, "Back", mouse, 32)) {
                    screen = SCREEN_MAIN_MENU;
                }
                break;
            }

            case SCREEN_PLAYER_SETUP: {
                draw_background(bg.setup);
                DrawText("SIGN THE LEDGER......", 80, 60, 32, WHITE);

                DrawText(TextFormat("Total players (incl. you): %d", setupPlayerCount),
                          80, 110, 26, WHITE);
                if (button((Rectangle){560, 105, 50, 50}, "+", mouse, 28)) {
                    if (setupPlayerCount < MAX_PLAYERS) setupPlayerCount++;
                }
                if (button((Rectangle){620, 105, 50, 50}, "-", mouse, 28)) {
                    if (setupPlayerCount > MIN_PLAYERS) setupPlayerCount--;
                    if (activeNameField >= setupPlayerCount) activeNameField = -1;
                }

                for (int i = 0; i < setupPlayerCount; i++) {
                    int col = i / 5;
                    int row = i % 5;
                    float x = 80 + col * 480;
                    float y = 180 + row * 90;

                    const char *label = (i == 0) ? "You:" : TextFormat("Player %d:", i + 1);
                    DrawText(label, (int)x, (int)y, 22, WHITE);

                    Rectangle box = { x, y + 26, 420, 50 };
                    bool focused = (activeNameField == i);
                    DrawRectangleRec(box, RAYWHITE);
                    DrawRectangleLinesEx(box, focused ? 3 : 2, focused ? (Color){100, 180, 240, 255} : DARKGRAY);
                    DrawText(playerNameInputs[i], (int)(box.x + 10), (int)(box.y + 12), 24, BLACK);

                    if (CheckCollisionPointRec(mouse, box) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        activeNameField = i;
                    }
                }

                if (button((Rectangle){1570, 700, 300, 70}, "Connect via WiFi", mouse, 26)) {
                }
                if (button((Rectangle){1570, 800, 300, 70}, "Connect via LAN", mouse, 26)) {
                }

                if (button((Rectangle){1570, 350, 300, 80}, "CONFIRM", mouse, 32)) {
                    memset(&gs, 0, sizeof(gs));
                    gs.player_count = setupPlayerCount;
                    gs.night_duration = 30;

                    for (int i = 0; i < gs.player_count; i++) {
                        gs.players[i].id = i;
                        gs.players[i].points = 0;
                        gs.players[i].is_human = (i == 0);

                        if (strlen(playerNameInputs[i]) > 0) {
                            strncpy(gs.players[i].name, playerNameInputs[i], MAX_NAME_LEN - 1);
                        } else if (i == 0) {
                            strncpy(gs.players[i].name, "You", MAX_NAME_LEN - 1);
                        } else {
                            snprintf(gs.players[i].name, MAX_NAME_LEN, "Bot %d", i);
                        }
                    }
                    gs.human_id = 0;

                    assign_roles(&gs);
                    
                    lastEliminated = -1;
                    lastKilled = -1; lastProtected = -1; lastZombieKilled = -1;
                    humanGhostTarget = -1; humanWizardTarget = -1; humanZombieTarget = -1;
                    selectedVoteTarget = -1;
                    justBecameActiveZombie = false;
                    introAnimTimer = 0.0f;

                    screen = SCREEN_INTRO_ANIM;
                    PlaySound(introStartSound);
                }
                break;
            }

            case SCREEN_INTRO_ANIM: {

                introAnimTimer += GetFrameTime();

                 int currentFrame = (int)(introAnimTimer * INTRO_FPS);

                if (currentFrame >= INTRO_FRAME_COUNT)
                     currentFrame = INTRO_FRAME_COUNT - 1;

            draw_background(introFrames[currentFrame]);


            //text on top 
            const char *title = "LOADING......";

            DrawText(title,
                 SCREEN_W/2 - MeasureText(title, 70)/2,
                420,
                70,
                MAROON);


            float remaining = INTRO_ANIM_SECONDS - introAnimTimer;

            if (remaining < 0)
                 remaining = 0;

            const char *countdown = TextFormat("%.1f", remaining);

            DrawText(countdown,
                      SCREEN_W/2 - MeasureText(countdown, 50)/2,
                     520,
                     50,
                     WHITE);


            /* After 5 seconds */
             if (introAnimTimer >= INTRO_ANIM_SECONDS) {
                   screen = SCREEN_ROLE_REVEAL;
                   // PlaySound(introEndSound);
             }

            break;
         }
           
            case SCREEN_ROLE_REVEAL: {
                Player *me = &gs.players[gs.human_id];

                /* v4: 3 distinct background/role-reveal screens, one per
                 * assignable starting role (Zombie is never a starting
                 * role, so it has no slot here). */
                Texture2D roleBg;
                switch (me->role) {
                    case GHOST:  roleBg = bg.roleRevealGhost;    break;
                    case WIZARD: roleBg = bg.roleRevealWizard;   break;
                    default:     roleBg = bg.roleRevealVillager; break;
                }
                draw_background(roleBg);

                const char *title = TextFormat("You are: %s", role_name(me->role));
                DrawText(title, SCREEN_W/2 - MeasureText(title, 60)/2, 300, 60, WHITE);

                const char *desc = "";
                switch (me->role) {
                    case VILLAGER: desc = ""; break;
                    case GHOST:    desc = ""; break;
                    case WIZARD:   desc = ""; break;
                    default: break;
                }
                DrawText(desc, SCREEN_W/2 - MeasureText(desc, 26)/2, 420, 26, WHITE);

                if (button((Rectangle){SCREEN_W/2 - 150, 600, 300, 80}, "DESCEND", mouse, 32)) {
                    advance_to_day_vote(&gs, &screen, &lastEliminated, &selectedVoteTarget);
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_DAY_VOTE: {
                draw_background(bg.dayVote);
                DrawText(TextFormat("DAY %d -- FIND THE ONE WHO HUNTS US", gs.round),
                          80, 60, 40, WHITE);

                /* v4: click selects (highlighted), doesn't vote immediately;
                 * a separate Confirm Vote button finalizes it. */
                int clicked = player_button_list(&gs, gs.human_id, 0, mouse, 80, 180, selectedVoteTarget);
                if (clicked >= 0) {
                    selectedVoteTarget = (selectedVoteTarget == clicked) ? -1 : clicked; /* click again to deselect */
                }

                bool haveSelection = (selectedVoteTarget >= 0);
                Rectangle confirmRect = { SCREEN_W/2 - 150, SCREEN_H - 150, 300, 80 };
                Color oldFill = haveSelection ? (Color){230, 41, 55, 255} : (Color){110, 25, 40, 255};
                DrawRectangleRec(confirmRect, oldFill);
                DrawRectangleLinesEx(confirmRect, 2, DARKGRAY);
                const char *confirmLabel = "JUDGE";
                int confirmTextW = MeasureText(confirmLabel, 32);
                DrawText(confirmLabel, (int)(confirmRect.x + (confirmRect.width - confirmTextW) / 2),
                          (int)(confirmRect.y + (confirmRect.height - 32) / 2), 32, BLACK);

                if (haveSelection && CheckCollisionPointRec(mouse, confirmRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    resolve_day_vote_now(&gs, selectedVoteTarget, &lastEliminated);
                    screen = SCREEN_DAY_RESULT;
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_DAY_RESULT: {
                draw_background(bg.dayResult);
                const char *msg;
                if (lastEliminated >= 0) {
                    msg = TextFormat("%s was been casted out. The Village Lost It's %s.",
                                      gs.players[lastEliminated].name,
                                      role_name(gs.players[lastEliminated].role));
                } else {
                    msg = "TIE -- no majority. The Village Couldn't Agree.";
                }
                DrawText(msg, SCREEN_W/2 - MeasureText(msg, 32)/2, 450, 32, WHITE);

                if (button((Rectangle){SCREEN_W/2 - 150, 600, 300, 80}, "DESCEND", mouse, 32)) {
                    check_win_condition(&gs);
                    if (gs.game_over != GAME_RUNNING) {
                        score_game_win(&gs);
                        screen = SCREEN_GAME_OVER;
                    } else {
                        Player *ghost = find_player(&gs, gs.ghost_id);
                        Player *wizard = find_player(&gs, gs.wizard_id);
                        Player *human = &gs.players[gs.human_id];
                        bool humanHasNightAction =
                            (ghost && ghost->is_human && ghost->status == ALIVE) ||
                            (wizard && wizard->is_human && wizard->status == ALIVE) ||
                            (human->is_active_zombie && human->status == ALIVE); /* v4 */

                        humanGhostTarget = -1;
                        humanWizardTarget = -1;
                        humanZombieTarget = -1; /* v4 */

                        if (humanHasNightAction) {
                            screen = SCREEN_NIGHT_ACTION;
                        } else {
                            resolve_night_now(&gs, -1, -1, -1, &lastKilled, &lastProtected,
                                                &lastZombieKilled, &justBecameActiveZombie);
                            screen = SCREEN_NIGHT_RESULT;
                        }
                    }
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_NIGHT_ACTION: {
                draw_background(bg.nightAction);
                Player *ghost = find_player(&gs, gs.ghost_id);
                Player *wizard = find_player(&gs, gs.wizard_id);
                Player *human = &gs.players[gs.human_id];

                if (ghost && ghost->is_human && ghost->status == ALIVE) {
                    DrawText(TextFormat("NIGHT %d -- Choose who joins the Death", gs.round),
                              80, 60, 36, WHITE);
                    int clicked = player_button_list(&gs, gs.ghost_id, 1, mouse, 80, 180, -1);
                    if (clicked >= 0) {
                        humanGhostTarget = clicked;
                        resolve_night_now(&gs, humanGhostTarget, -1, -1, &lastKilled, &lastProtected,
                                            &lastZombieKilled, &justBecameActiveZombie);
                        screen = SCREEN_NIGHT_RESULT;
                    }
                } else if (wizard && wizard->is_human && wizard->status == ALIVE) {
                    DrawText(TextFormat("NIGHT %d -- Shield The Innocent", gs.round),
                              80, 60, 36, WHITE);
                    int clicked = player_button_list(&gs, -1, 1, mouse, 80, 180, -1);
                    if (clicked >= 0) {
                        humanWizardTarget = clicked;
                        resolve_night_now(&gs, -1, humanWizardTarget, -1, &lastKilled, &lastProtected,
                                            &lastZombieKilled, &justBecameActiveZombie);
                        screen = SCREEN_NIGHT_RESULT;
                    }
                } else if (human->is_active_zombie && human->status == ALIVE) {
                    /* v4: the human's own independent Zombie kill action */
                    DrawText(TextFormat("NIGHT %d -- Choose who joins the death", gs.round),
                              80, 60, 36, WHITE);
                    int clicked = player_button_list(&gs, gs.human_id, 1, mouse, 80, 180, -1);
                    if (clicked >= 0) {
                        humanZombieTarget = clicked;
                        resolve_night_now(&gs, -1, -1, humanZombieTarget, &lastKilled, &lastProtected,
                                            &lastZombieKilled, &justBecameActiveZombie);
                        screen = SCREEN_NIGHT_RESULT;
                    }
                } else {
                    /* Shouldn't normally reach here, but handle gracefully */
                    DrawText("Waiting...", 80, 60, 36, WHITE);
                    if (button((Rectangle){SCREEN_W/2 - 150, 600, 300, 80}, "DESCEND", mouse, 32)) {
                        resolve_night_now(&gs, -1, -1, -1, &lastKilled, &lastProtected,
                                            &lastZombieKilled, &justBecameActiveZombie);
                        screen = SCREEN_NIGHT_RESULT;
                    }
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_NIGHT_RESULT: {
                draw_background(bg.nightResult);

                /* v4: up to two independent outcomes can happen the same
                 * night (the Ghost's kill and the active Zombie's kill), so
                 * both are reported as separate lines instead of a single
                 * either/or message. */
                int y = 420;
                bool anythingHappened = false;

                if (lastKilled >= 0) {
                    const char *msg = TextFormat("Ghost Strikes <%s> has fallen to the CURSE.", gs.players[lastKilled].name);
                    DrawText(msg, SCREEN_W/2 - MeasureText(msg, 30)/2, y, 30, WHITE);
                    y += 44;
                    anythingHappened = true;
                }
                if (lastZombieKilled >= 0) {
                    const char *msg = TextFormat("Zombie Strikes <%s> has fallen to the CURSE.", gs.players[lastZombieKilled].name);
                    DrawText(msg, SCREEN_W/2 - MeasureText(msg, 30)/2, y, 30, WHITE);
                    y += 44;
                    anythingHappened = true;
                }
                if (lastProtected >= 0) {
                    const char *msg = TextFormat("The Wizard shielded %s -- that attack failed!", gs.players[lastProtected].name);
                    DrawText(msg, SCREEN_W/2 - MeasureText(msg, 30)/2, y, 30, WHITE);
                    y += 44;
                    anythingHappened = true;
                }
                if (!anythingHappened) {
                    const char *msg = "Nothing happened last night.";
                    DrawText(msg, SCREEN_W/2 - MeasureText(msg, 32)/2, y, 32, WHITE);
                }

                if (button((Rectangle){SCREEN_W/2 - 150, 650, 300, 80}, "DESCEND", mouse, 32)) {
                    check_win_condition(&gs);
                    if (gs.game_over != GAME_RUNNING) {
                        score_game_win(&gs);
                        screen = SCREEN_GAME_OVER;
                    } else if (justBecameActiveZombie) {
                        /* v4: one-time reveal that the human is now a Zombie */
                        screen = SCREEN_ZOMBIE_REVEAL;
                    } else {
                        gs.round++;
                        advance_to_day_vote(&gs, &screen, &lastEliminated, &selectedVoteTarget);
                    }
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_ZOMBIE_REVEAL: {
                draw_background(bg.zombieReveal);

                const char *title = "YOU HAVE BECOME A ZOMBIE";
                DrawText(title, SCREEN_W/2 - MeasureText(title, 55)/2, 300, 55, MAROON);

                const char *lines[] = {
                    "The Ghost has turned you into a Zombie -- but you're still in the game.",
                    "You can still vote during the day.",
                    "Each night, you'll secretly choose your own kill target,",
                    "independent of the Ghost.",
                    "If the village votes you out, the Villagers win immediately."
                };
                int y = 430;
                for (int i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
                    DrawText(lines[i], SCREEN_W/2 - MeasureText(lines[i], 26)/2, y, 26, WHITE);
                    y += 36;
                }

                if (button((Rectangle){SCREEN_W/2 - 150, 700, 300, 80}, "DESCEND", mouse, 32)) {
                    justBecameActiveZombie = false;
                    gs.round++;
                    advance_to_day_vote(&gs, &screen, &lastEliminated, &selectedVoteTarget);
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_GAME_OVER: {
                bool villagerWin = (gs.game_over == GAME_VILLAGER_WIN);
                draw_background(villagerWin ? bg.gameOverVillagerWin : bg.gameOverGhostWin);

                const char *result = villagerWin ? "THE VILLAGE PREVAILS" : "THE VILLAGE HAS FALLEN";
                DrawText(result, SCREEN_W/2 - MeasureText(result, 70)/2, 450, 70,
                          villagerWin ? DARKGREEN : MAROON);

                if (button((Rectangle){SCREEN_W/2 - 150, 600, 300, 80}, "RECKONING", mouse, 32)) {
                    screen = SCREEN_LEADERBOARD;
                }
                break;
            }

            /* ============================================================ */
            case SCREEN_LEADERBOARD: {
                draw_background(bg.leaderboard);
                DrawText("FINAL RECKONING", SCREEN_W/2 - MeasureText("FINAL RECKONING", 50)/2, 60, 50, WHITE);

                int order[MAX_PLAYERS];
                get_leaderboard_order(&gs, order);

                for (int rank = 0; rank < gs.player_count; rank++) {
                    Player *p = &gs.players[order[rank]];
                    const char *line = TextFormat("#%d  %-10s  %-9s  %d pts%s",
                                                    rank + 1, p->name, role_name(p->role), p->points,
                                                    p->is_human ? "  (YOU)" : "");
                    DrawText(line, 200, 160 + rank * 50, 28, WHITE);
                }

                if (button((Rectangle){SCREEN_W/2 - 320, 900, 300, 80}, "Another Night", mouse, 32)) {
                    screen = SCREEN_PLAYER_SETUP;
                }
                if (button((Rectangle){SCREEN_W/2 + 20, 900, 300, 80}, "Walkaway", mouse, 32)) {
                    quitRequested = true;
                }
                break;
            }
        }

        EndDrawing();
    }
    
    unload_backgrounds(&bg);
    UnloadSound(introStartSound);
    UnloadSound(introEndSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
