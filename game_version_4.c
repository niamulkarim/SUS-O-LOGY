#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game_version_4.h"

/* ================= Name helpers ================= */

const char *role_name(Role r) {
    switch (r) {
        case VILLAGER: return "VILLAGER";
        case GHOST:    return "GHOST";
        case WIZARD:   return "WIZARD";
        case ZOMBIE:   return "ZOMBIE";
    }
    return "?";
}

const char *status_name(Status s) {
    switch (s) {
        case ALIVE:        return "ALIVE";
        case ELIMINATED:   return "ELIMINATED";
        case DISCONNECTED: return "DISCONNECTED";
    }
    return "?";
}

/* ================= Helpers ================= */

Player *find_player(GameState *gs, int id) {
    if (id < 0 || id >= gs->player_count) return NULL;
    return &gs->players[id];
}

int is_villager_side(const Player *p) {
    return (p->role == VILLAGER || p->role == WIZARD);
}

int count_living_villagers(const GameState *gs) {
    int count = 0;
    for (int i = 0; i < gs->player_count; i++) {
        const Player *p = &gs->players[i];
        if (p->status == ALIVE && is_villager_side(p)) {
            count++;
        }
    }
    return count;
}

/* Zombies eliminated immediately.
 * Only used for end-game statistics. */
int count_ghost_team(const GameState *gs) {
    int count = 0;
    for (int i = 0; i < gs->player_count; i++) {
        const Player *p = &gs->players[i];
        if (p->role == GHOST && p->status == ALIVE) count++;
        if (p->role == ZOMBIE) count++; /* counted regardless of status -- they were "made" */
    }
    return count;
}

/* ================= 6.3 Role Assignment ================= */

void assign_roles(GameState *gs) {
    int n = gs->player_count;
    int order[MAX_PLAYERS];
    for (int i = 0; i < n; i++) order[i] = i;

    /* Fisher-Yates shuffle */
    for (int i = n - 1; i >= 1; i--) {
        int j = rand() % (i + 1);
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    for (int i = 0; i < n; i++) {
        Player *p = &gs->players[order[i]];
        p->status = ALIVE;
        p->can_vote = 1;
        p->can_chat = 1;
        p->is_active_zombie = 0;
        if (i == 0) {
            p->role = GHOST;
            gs->ghost_id = order[i];
        } else if (i == 1) {
            p->role = WIZARD;
            gs->wizard_id = order[i];
        } else {
            p->role = VILLAGER;
        }
    }

    gs->round = 1;
    gs->phase = PHASE_DAY;
    gs->day_duration = gs->night_duration * 3;
    gs->game_over = GAME_RUNNING;
    gs->pending_ghost_target = -1;
    gs->pending_wizard_target = -1;
    gs->pending_zombie_target = -1;
    gs->zombie_vote_out_win = 0;
}

/* ================= 6.6 Voting Algorithm ================= */

int run_day_vote(GameState *gs, const int *votes) {
    int vote_count[MAX_PLAYERS] = {0};
    int eligible_voters = 0;

    for (int i = 0; i < gs->player_count; i++) {
        Player *voter = &gs->players[i];
        if (voter->status != ALIVE || voter->can_vote != 1) continue;
        eligible_voters++;

        int target = votes[i];
        if (target < 0 || target >= gs->player_count) continue; /* abstain */
        vote_count[target]++;
    }

    /* find max */
    int max_votes = -1, max_id = -1;
    for (int i = 0; i < gs->player_count; i++) {
        if (vote_count[i] > max_votes) {
            max_votes = vote_count[i];
            max_id = i;
        }
    }

    if (max_votes > 0) {
        Player *eliminated = &gs->players[max_id];
        eliminated->status = ELIMINATED;
        eliminated->can_vote = 0;
        eliminated->can_chat = 0; /* Spectator Mode */

        /* If the eliminated Wizard was the current Wizard, trigger succession */
        if (max_id == gs->wizard_id) {
            wizard_succession(gs);
        }

       /* Human Zombie voted out.
        * Immediately gives Villagers the win. */
        if (eliminated->role == ZOMBIE && eliminated->is_active_zombie) {
            gs->zombie_vote_out_win = 1;
        }

        return max_id;
    }

    /* Tie / no majority */
    return -1;
}

/* ================= v2: Night action collection ================= */

int ghost_choose_kill(GameState *gs, int target_id) {
    Player *target = find_player(gs, target_id);
    if (!target) return 0;
    if (target->status != ALIVE) return 0;
    if (target->role != VILLAGER && target->role != WIZARD) return 0;

    gs->pending_ghost_target = target_id;
    return 1;
}

int wizard_choose_protect(GameState *gs, int target_id) {
    Player *target = find_player(gs, target_id);
    if (!target) return 0;
    if (target->status != ALIVE) return 0;
    if (target->role != VILLAGER && target->role != WIZARD) return 0; /* can't protect Ghost/Zombie */

    gs->pending_wizard_target = target_id;
    return 1;
}

/* Human Zombie chooses their own target.
 * Target must be an alive Villager or Wizard, not themself. */
int zombie_choose_kill(GameState *gs, int target_id) {
    Player *target = find_player(gs, target_id);
    if (!target) return 0;
    if (target->status != ALIVE) return 0;
    if (target->role != VILLAGER && target->role != WIZARD) return 0;

   /* Find the active Zombie.
 * Prevent the Zombie from targeting themself. */
    for (int i = 0; i < gs->player_count; i++) {
        if (gs->players[i].is_active_zombie && target_id == i) return 0;
    }

    gs->pending_zombie_target = target_id;
    return 1;
}

/* ================= v2/v4: Night resolution ================= */

int resolve_night(GameState *gs, int *protected_id, int *zombie_killed_id) {
    *protected_id = -1;
    *zombie_killed_id = -1;

    int ghost_target  = gs->pending_ghost_target;
    int wizard_target = gs->pending_wizard_target;
    int zombie_target = gs->pending_zombie_target;
    gs->pending_ghost_target  = -1;
    gs->pending_wizard_target = -1;
    gs->pending_zombie_target = -1;

    int ghost_killed_id = -1;
    int wizard_target_used = 0; /* the Wizard only has one protect target to spend */

    /* -------- Ghost's kill -------- */
    if (ghost_target >= 0) {
        Player *target = find_player(gs, ghost_target);
        if (target && target->status == ALIVE) {
            if (wizard_target == ghost_target) {
                /* Wizard guessed right -- attack blocked, nobody dies */
                *protected_id = ghost_target;
                wizard_target_used = 1;
            } else {
                int was_wizard = (ghost_target == gs->wizard_id);

                if (target->is_human) {
                    /* Human turned Zombie stays alive.
                     * Can vote and gets a night kill action. */
                    target->role = ZOMBIE;
                    target->is_active_zombie = 1;
                   /* Keep human Zombie alive.
                    * They can still vote. */
                } else {
                   /* Bots become Zombies and die immediately. */
                    target->role = ZOMBIE;
                    target->status = ELIMINATED;
                    target->can_vote = 0;
                    target->can_chat = 0;
                }

                ghost_killed_id = ghost_target;

                if (was_wizard) {
                    wizard_succession(gs);
                }
            }
        }
    }

   /* Zombie gets an independent kill.
    * Skipped if targeting the Ghost's target. */
    if (zombie_target >= 0 && zombie_target != ghost_target) {
        Player *ztarget = find_player(gs, zombie_target);
        if (ztarget && ztarget->status == ALIVE) {
            if (!wizard_target_used && wizard_target == zombie_target) {
                if (*protected_id < 0) *protected_id = zombie_target;
            } else {
                int zt_was_wizard = (zombie_target == gs->wizard_id);

                /* The Zombie's victims just die -- no further zombie chain */
                ztarget->status = ELIMINATED;
                ztarget->can_vote = 0;
                ztarget->can_chat = 0;

                *zombie_killed_id = zombie_target;

                if (zt_was_wizard) {
                    wizard_succession(gs);
                }
            }
        }
    }

    return ghost_killed_id;
}

/* ================= 6.5 Wizard Succession ================= */

int wizard_succession(GameState *gs) {
    int candidates[MAX_PLAYERS];
    int n = 0;

    for (int i = 0; i < gs->player_count; i++) {
        Player *p = &gs->players[i];
        if (p->status == ALIVE && p->role == VILLAGER) {
            candidates[n++] = i;
        }
    }

    if (n == 0) {
        gs->wizard_id = -1;
        return -1;
    }

    int chosen = candidates[rand() % n];
    gs->players[chosen].role = WIZARD;
    gs->wizard_id = chosen;
    return chosen;
}

/* ================= 6.2 Win Condition Check (v2) ================= */

int check_win_condition(GameState *gs) {
    /* Human Zombie voted out.
     * Immediately gives Villagers the win. */
    if (gs->zombie_vote_out_win) {
        gs->game_over = GAME_VILLAGER_WIN;
        return gs->game_over;
    }

    Player *ghost = find_player(gs, gs->ghost_id);
    int lv = count_living_villagers(gs);

    if (ghost && ghost->status != ALIVE) {
        gs->game_over = GAME_VILLAGER_WIN;
        return gs->game_over;
    }

    /* No living Villagers/Wizards means Ghost wins.
     * One Villager/Wizard left also means Ghost wins to avoid a deadlock. */
    if (lv <= 1) {
        gs->game_over = GAME_GHOST_WIN;
        return gs->game_over;
    }

    gs->game_over = GAME_RUNNING;
    return gs->game_over;
}

/* ================= 4.1 Scoring ================= */

void score_round_survival(GameState *gs) {
    for (int i = 0; i < gs->player_count; i++) {
        if (gs->players[i].status == ALIVE) {
            gs->players[i].points += 10;
        }
    }
}

void score_correct_vote(GameState *gs, int voter_id) {
    Player *p = find_player(gs, voter_id);
    if (p) p->points += 20;
}

void score_incorrect_vote(GameState *gs, int voter_id) {
    Player *p = find_player(gs, voter_id);
    if (p) p->points -= 10;
}

void score_ghost_kill(GameState *gs) {
    Player *ghost = find_player(gs, gs->ghost_id);
    if (ghost) ghost->points += 15;
}

void score_wizard_protect(GameState *gs) {
    Player *wizard = find_player(gs, gs->wizard_id);
    if (wizard) wizard->points += 15;
}

void score_game_win(GameState *gs) {
    if (gs->game_over == GAME_GHOST_WIN) {
        for (int i = 0; i < gs->player_count; i++) {
            Player *p = &gs->players[i];
            if (p->role == GHOST || p->role == ZOMBIE) p->points += 50;
        }
    } else if (gs->game_over == GAME_VILLAGER_WIN) {
        for (int i = 0; i < gs->player_count; i++) {
            Player *p = &gs->players[i];
            if (p->status == ALIVE && is_villager_side(p)) p->points += 50;
        }
    }
}

/* ================= v2: Leaderboard sorting ================= */

void get_leaderboard_order(const GameState *gs, int *order_out) {
    int n = gs->player_count;
    for (int i = 0; i < n; i++) order_out[i] = i;

    /* Simple insertion marsi 
     * Sorts players by points, highest first. */
    for (int i = 1; i < n; i++) {
        int key = order_out[i];
        int key_points = gs->players[key].points;
        int j = i - 1;
        while (j >= 0 && gs->players[order_out[j]].points < key_points) {
            order_out[j + 1] = order_out[j];
            j--;
        }
        order_out[j + 1] = key;
    }
}
