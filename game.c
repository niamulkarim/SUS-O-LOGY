#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

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
        if (p->status == ALIVE && is_villager_side(p) && p->role != ZOMBIE) {
            count++;
        }
    }
    return count;
}

int count_ghost_team(const GameState *gs) {
    int count = 0;
    for (int i = 0; i < gs->player_count; i++) {
        const Player *p = &gs->players[i];
        if (p->status != ALIVE) continue;
        if (p->role == GHOST || p->role == ZOMBIE) count++;
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

    if (max_votes > eligible_voters / 2 && max_votes > 0) {
        Player *eliminated = &gs->players[max_id];
        eliminated->status = ELIMINATED;
        eliminated->can_vote = 0;
        eliminated->can_chat = 0; /* Spectator Mode */

        /* If the eliminated Wizard was the current Wizard, trigger succession */
        if (max_id == gs->wizard_id) {
            wizard_succession(gs);
        }

        /* If Ghost eliminated: all Zombies instantly destroyed */
        if (eliminated->role == GHOST) {
            for (int i = 0; i < gs->player_count; i++) {
                if (gs->players[i].role == ZOMBIE && gs->players[i].status == ALIVE) {
                    /* Zombies vanish -- treated as eliminated observers too */
                    gs->players[i].status = ELIMINATED;
                    gs->players[i].can_chat = 0;
                    gs->players[i].can_vote = 0;
                }
            }
        }

        return max_id;
    }

    /* Tie / no majority */
    return -1;
}

/* ================= 6.4 Zombie Creation ================= */

int ghost_kill(GameState *gs, int target_id) {
    Player *target = find_player(gs, target_id);
    if (!target) return 0;
    if (target->status != ALIVE) return 0;
    if (target->role != VILLAGER && target->role != WIZARD) return 0; /* not Ghost, not existing Zombie */

    /* If the target is the current Wizard, succession happens BEFORE
     * turning them into a Zombie so a living Villager inherits the role. */
    int was_wizard = (target_id == gs->wizard_id);

    target->role = ZOMBIE;
    target->can_vote = 0;
    target->can_chat = 1;
    /* status stays ALIVE -- Zombies are alive, just not Villager-side anymore */

    if (was_wizard) {
        wizard_succession(gs);
    }

    return 1;
}

/* ================= Wizard Revival ================= */

int wizard_revive(GameState *gs, int target_id) {
    Player *target = find_player(gs, target_id);
    if (!target) return 0;
    /* Only ELIMINATED Villagers/Wizards may be revived. Zombies never
     * appear on the Wizard's revival list (enforced here, server-side). */
    if (target->status != ELIMINATED) return 0;
    if (target->role == ZOMBIE || target->role == GHOST) return 0;

    target->status = ALIVE;
    target->can_vote = 1;
    target->can_chat = 1;
    return 1;
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

/* ================= 6.2 Win Condition Check ================= */

int check_win_condition(GameState *gs) {
    Player *ghost = find_player(gs, gs->ghost_id);
    int lv = count_living_villagers(gs);
    int gt = count_ghost_team(gs);

    if (ghost && ghost->status != ALIVE) {
        gs->game_over = GAME_VILLAGER_WIN;
        return gs->game_over;
    }

    if (gt >= lv && lv > 0) {
        gs->game_over = GAME_GHOST_WIN;
        return gs->game_over;
    }

    if (lv == 0) {
        gs->game_over = GAME_GHOST_WIN;
        return gs->game_over;
    }

    gs->game_over = GAME_RUNNING;
    return gs->game_over;
}

/* ================= 3.4 / Section 11 Disconnect Handling ================= */

void handle_disconnect(GameState *gs, int player_id) {
    Player *p = find_player(gs, player_id);
    if (!p) return;

    p->status = DISCONNECTED;
    p->can_vote = 0;
    p->can_chat = 0;
    p->points -= 20; /* 4.1 disconnect penalty */

    if (player_id == gs->ghost_id) {
        /* Villagers win immediately */
        gs->game_over = GAME_VILLAGER_WIN;
    } else if (player_id == gs->wizard_id) {
        wizard_succession(gs);
    }
    /* Villager/Zombie disconnects: just removed, game continues (checked by caller) */
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

void score_zombie_survival(GameState *gs) {
    for (int i = 0; i < gs->player_count; i++) {
        if (gs->players[i].role == ZOMBIE && gs->players[i].status == ALIVE) {
            gs->players[i].points += 10;
        }
    }
}

void score_wizard_revive(GameState *gs) {
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
