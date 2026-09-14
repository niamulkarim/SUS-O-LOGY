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
    Status status;              /* Alive / Eliminated / Disconnected */
    int  can_vote;               /* 0 if Zombie(dead) or Eliminated */
    int  can_chat;                /* 0 if Eliminated (Spectator Mode) */
    int  points;                  /* Running score across rounds */
    int  is_human;                 /* 1 if this seat is controlled interactively */
    int  socket_fd;                /* Server-side socket fd (server only, unused in sim) */

    /* v4: set to 1 only when a HUMAN is turned into a Zombie by the Ghost.
     * Unlike a bot Zombie (which is immediately dead/ELIMINATED), an active
     * Zombie stays ALIVE, keeps can_vote, and gets a night kill action of
     * their own. Bots are never turned into an active Zombie -- for them,
     * ZOMBIE still means dead, exactly as in v2/v3. */
    int  is_active_zombie;
} Player;

typedef struct {
    Player players[MAX_PLAYERS];
    int player_count;
    int round;
    Phase phase;
    int ghost_id;      /* index into players[] of the Ghost */
    int wizard_id;      /* index into players[] of the current Wizard, -1 if none available */
    int human_id;         /* index into players[] of the interactive human player */
    int timer;
    int night_duration; /* T seconds */
    int day_duration;   /* 3T seconds, auto-calculated */
    int game_over;       /* GAME_RUNNING / GAME_VILLAGER_WIN / GAME_GHOST_WIN */

    /* v2: secret night actions, resolved together in resolve_night() */
    int pending_ghost_target;   /* who the Ghost chose to kill this night, -1 if none yet */
    int pending_wizard_target;  /* who the Wizard chose to protect this night, -1 if none yet */

    /* v4: the active (human) Zombie's independent night kill target, -1 if
     * none yet / not applicable. Resolved alongside the Ghost's kill in
     * resolve_night(), and can be blocked by the Wizard's protect same as
     * the Ghost's kill can. */
    int pending_zombie_target;

    /* v4: set to 1 the moment the human-Zombie is voted out during the day.
     * check_win_condition() treats this as an immediate Villager win,
     * regardless of whether the real Ghost is still alive. Kept as its own
     * flag (rather than recomputed each call) so a later, unrelated call to
     * check_win_condition() can't accidentally undo this result. */
    int zombie_vote_out_win;
} GameState;

/* ---- Role assignment (GDD 6.3) ---- */
void assign_roles(GameState *gs);

/* ---- Voting (GDD 6.6) ---- */
/* votes[i] = target player id that players[i] voted for, or -1 if no vote cast.
 * Only entries where players[i].can_vote == 1 are considered.
 * Returns the eliminated player's id, or -1 if the vote was a tie/no-majority. */
int run_day_vote(GameState *gs, const int *votes);

/* ---- Night actions (v2 protect-based Wizard) ---- */
/* Ghost secretly chooses a kill target. Validates target is alive VILLAGER
 * or WIZARD. Does NOT apply the kill yet -- resolved in resolve_night().
 * Returns 1 on success, 0 if invalid. */
int ghost_choose_kill(GameState *gs, int target_id);

/* Wizard secretly chooses someone to protect. Validates target is alive
 * VILLAGER or WIZARD (self-protection allowed). Does NOT apply anything yet.
 * Returns 1 on success, 0 if invalid. */
int wizard_choose_protect(GameState *gs, int target_id);

/* v4: the active (human) Zombie secretly chooses their own kill target,
 * independent of the Ghost. Validates target is alive VILLAGER or WIZARD
 * and not the Zombie themself. Does NOT apply anything yet -- resolved in
 * resolve_night(). Returns 1 on success, 0 if invalid. */
int zombie_choose_kill(GameState *gs, int target_id);

/* Resolves the night. Two separate attacks can land in the same night: the
 * Ghost's kill and the active Zombie's kill (v4). Each is checked against
 * the Wizard's single protect target independently -- the Wizard can only
 * save one of them if both target the same person, or one each if they
 * target different people (they still only get the one protect target).
 *
 * - Ghost's kill: if blocked, *protected_id is set to that target and
 *   nobody dies from it. Otherwise the target is turned into a ZOMBIE. If
 *   the target is the human player, they become an ACTIVE Zombie (stay
 *   ALIVE, keep can_vote, gain a night kill action -- v4). If the target is
 *   a bot, they become a dead Zombie exactly as in v2/v3 (ELIMINATED).
 * - Zombie's kill (v4, only relevant once the human is an active Zombie):
 *   if blocked, *protected_id is set to that target (unless the Ghost's
 *   target already used the Wizard's one protect). Otherwise the target is
 *   marked ELIMINATED outright (no further zombie-chain transformation).
 *
 * Returns the id of the player killed by the GHOST this night, or -1 if the
 * Ghost's attack didn't land. Sets *zombie_killed_id to the id killed by the
 * active Zombie this night, or -1 if none. */
int resolve_night(GameState *gs, int *protected_id, int *zombie_killed_id);

/* ---- Wizard succession (GDD 6.5) ---- */
/* Called whenever the current Wizard becomes ELIMINATED/DISCONNECTED/ZOMBIE.
 * Randomly promotes a living Villager to Wizard. Returns new wizard id, or -1
 * if no living Villager is available. */
int wizard_succession(GameState *gs);

/* ---- Win condition check (GDD 6.2, updated for v2 dead Zombies) ---- */
/* Updates gs->game_over. Returns the resulting game_over value. */
int check_win_condition(GameState *gs);
/* ---- Scoring (GDD 4.1) ---- */
void score_round_survival(GameState *gs);
void score_correct_vote(GameState *gs, int voter_id);
void score_incorrect_vote(GameState *gs, int voter_id);
void score_ghost_kill(GameState *gs);
void score_wizard_protect(GameState *gs);
void score_game_win(GameState *gs);

/* ---- Helpers ---- */
int  count_living_villagers(const GameState *gs); /* VILLAGER or WIZARD, alive */
int  count_ghost_team(const GameState *gs);        /* informational: ghost(alive) + zombies made */
int  is_villager_side(const Player *p);            /* role VILLAGER or WIZARD */
Player *find_player(GameState *gs, int id);

/* Fills order_out[0..player_count-1] with player indices sorted by
 * descending points (stable-ish; ties keep original relative id order). */
void get_leaderboard_order(const GameState *gs, int *order_out);

const char *role_name(Role r);
const char *status_name(Status s);

#endif /* GAME_H */
