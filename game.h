#ifndef GAME_H
#define GAME_H

#define MAX_PLAYERS 10
#define MIN_PLAYERS 6
#define MAX_NAME_LEN 32

/* ---- Enums (per GDD section 8.2) ---- */

typedef enum { VILLAGER, GHOST, WIZARD, ZOMBIE } Role;
typedef enum { ALIVE, ELIMINATED, DISCONNECTED } Status;
typedef enum { PHASE_DAY, PHASE_NIGHT } Phase;

/* game_over values */
#define GAME_RUNNING     0
#define GAME_VILLAGER_WIN 1
#define GAME_GHOST_WIN   2

/* ---- Core structs (per GDD section 8.2) ---- */

typedef struct {
    int  id;                    /* Unique player ID (0 to N-1) */
    char name[MAX_NAME_LEN];    /* Display name entered at join */
    Role role;                  /* Current role -- can change (Wizard) */
    Status status;               /* Alive / Eliminated / Disconnected */
    int  can_vote;               /* 0 if Zombie or Eliminated */
    int  can_chat;                /* 0 if Eliminated (Spectator Mode) */
    int  points;                  /* Running score across rounds */
    int  socket_fd;                /* Server-side socket fd (server only, unused in sim) */
} Player;

typedef struct {
    Player players[MAX_PLAYERS];
    int player_count;
    int round;
    Phase phase;
    int ghost_id;      /* index into players[] of the Ghost, -1 if Ghost eliminated? Ghost is always tracked, even if dead this round for logic purposes we check status */
    int wizard_id;      /* index into players[] of the current Wizard */
    int timer;
    int night_duration; /* T seconds */
    int day_duration;   /* 3T seconds, auto-calculated */
    int game_over;       /* GAME_RUNNING / GAME_VILLAGER_WIN / GAME_GHOST_WIN */
} GameState;

/* ---- Role assignment (GDD 6.3) ---- */
void assign_roles(GameState *gs);

/* ---- Voting (GDD 6.6) ---- */
/* votes[i] = target player id that players[i] voted for, or -1 if no vote cast.
 * Only entries where players[i].can_vote == 1 are considered.
 * Returns the eliminated player's id, or -1 if the vote was a tie/no-majority. */
int run_day_vote(GameState *gs, const int *votes);

/* ---- Zombie creation (GDD 6.4) ---- */
/* Ghost kills target_id. Validates target is alive VILLAGER or WIZARD.
 * Returns 1 on success, 0 if the action was invalid and ignored. */
int ghost_kill(GameState *gs, int target_id);

/* ---- Wizard revival ---- */
/* Wizard revives target_id (must currently be status == ELIMINATED and
 * became a Zombie this round is NOT allowed -- only true eliminated Villagers
 * are revivable, per GDD 2.3: "Wizard CANNOT revive a Zombie").
 * Returns 1 on success, 0 if invalid. */
int wizard_revive(GameState *gs, int target_id);

/* ---- Wizard succession (GDD 6.5) ---- */
/* Called whenever the current Wizard becomes ELIMINATED/DISCONNECTED/ZOMBIE.
 * Randomly promotes a living Villager to Wizard. Returns new wizard id, or -1
 * if no living Villager is available. */
int wizard_succession(GameState *gs);

/* ---- Win condition check (GDD 6.2) ---- */
/* Updates gs->game_over. Returns the resulting game_over value. */
int check_win_condition(GameState *gs);

/* ---- Disconnect handling (GDD 3.4 / section 11) ---- */
void handle_disconnect(GameState *gs, int player_id);

/* ---- Scoring (GDD 4.1) ---- */
void score_round_survival(GameState *gs);
void score_correct_vote(GameState *gs, int voter_id);
void score_incorrect_vote(GameState *gs, int voter_id);
void score_ghost_kill(GameState *gs);
void score_zombie_survival(GameState *gs);
void score_wizard_revive(GameState *gs);
void score_game_win(GameState *gs);

/* ---- Helpers ---- */
int  count_living_villagers(const GameState *gs); /* VILLAGER or WIZARD, alive, not zombie */
int  count_ghost_team(const GameState *gs);        /* Ghost (if alive) + active Zombies */
int  is_villager_side(const Player *p);            /* role VILLAGER or WIZARD */
Player *find_player(GameState *gs, int id);

#endif /* GAME_H */
