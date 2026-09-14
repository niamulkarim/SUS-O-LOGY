#ifndef GAME_H
#define GAME_H

#define MAX_PLAYERS 10
#define MIN_PLAYERS 6
#define MAX_NAME_LEN 32

typedef enum { VILLAGER, GHOST, WIZARD, ZOMBIE } Role;
typedef enum { ALIVE, ELIMINATED, DISCONNECTED } Status;
typedef enum { PHASE_DAY, PHASE_NIGHT } Phase;

#define GAME_RUNNING      0
#define GAME_VILLAGER_WIN 1
#define GAME_GHOST_WIN    2

typedef struct {
    int  id;
    char name[MAX_NAME_LEN];
    Role role;                   /* can change mid-game, e.g. Wizard succession or turning Zombie */
    Status status;
    int  can_vote;
    int  points;
    int  is_human;                /* 1 = you, 0 = bot */

    /* Only ever 1 for the human, and only once the Ghost turns them into a
     * Zombie. Unlike bots (who just die), the human stays alive, keeps
     * voting, and gets their own kill each night. */
    int  is_active_zombie;
} Player;

typedef struct {
    Player players[MAX_PLAYERS];
    int player_count;
    int round;
    Phase phase;
    int ghost_id;
    int wizard_id;       /* -1 if nobody's left to hold the role */
    int human_id;
    int timer;
    int night_duration;
    int day_duration;    /* always 3x night_duration */
    int game_over;

    /* Everyone's night choice gets stashed here first, then all resolved
     * together in resolve_night(). -1 means no choice made yet. */
    int pending_ghost_target;
    int pending_wizard_target;
    int pending_zombie_target;

    /* Flips to 1 the moment the human-Zombie gets voted out -- an instant
     * win for the villagers no matter what state the real Ghost is in. */
    int zombie_vote_out_win;
} GameState;

void assign_roles(GameState *gs);

/* votes[i] = who player i voted for, -1 = abstain. Returns who got
 * eliminated, or -1 on a tie. */
int run_day_vote(GameState *gs, const int *votes);

/* These three just record a choice -- nothing happens until resolve_night(). */
int ghost_choose_kill(GameState *gs, int target_id);
int wizard_choose_protect(GameState *gs, int target_id);
int zombie_choose_kill(GameState *gs, int target_id);

/* Runs both attacks (Ghost's + the active Zombie's) against the Wizard's
 * one protect. Returns who the Ghost killed (-1 if blocked/no target);
 * *zombie_killed_id gets who the Zombie killed, same deal. */
int resolve_night(GameState *gs, int *protected_id, int *zombie_killed_id);

/* Picks a new random Wizard when the old one dies. */
int wizard_succession(GameState *gs);

/* Checks/updates whether anyone's won yet. */
int check_win_condition(GameState *gs);

void score_round_survival(GameState *gs);
void score_correct_vote(GameState *gs, int voter_id);
void score_incorrect_vote(GameState *gs, int voter_id);
void score_ghost_kill(GameState *gs);
void score_wizard_protect(GameState *gs);
void score_game_win(GameState *gs);

int  count_living_villagers(const GameState *gs);
int  count_ghost_team(const GameState *gs);
int  is_villager_side(const Player *p);
Player *find_player(GameState *gs, int id);

void get_leaderboard_order(const GameState *gs, int *order_out);

const char *role_name(Role r);
const char *status_name(Status s);

#endif /* GAME_H */
