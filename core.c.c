#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game.h"

static const char *role_name(Role r) {
    switch (r) {
        case VILLAGER: return "VILLAGER";
        case GHOST:    return "GHOST";
        case WIZARD:   return "WIZARD";
        case ZOMBIE:   return "ZOMBIE";
    }
    return "?";
}

static const char *status_name(Status s) {
    switch (s) {
        case ALIVE:        return "ALIVE";
        case ELIMINATED:   return "ELIMINATED";
        case DISCONNECTED: return "DISCONNECTED";
    }
    return "?";
}

static void print_players(const GameState *gs) {
    printf("---- Player List (Round %d) ----\n", gs->round);
    for (int i = 0; i < gs->player_count; i++) {
        const Player *p = &gs->players[i];
        printf("  [%d] %-10s role=%-9s status=%-12s vote=%d chat=%d pts=%d\n",
               p->id, p->name, role_name(p->role), status_name(p->status),
               p->can_vote, p->can_chat, p->points);
    }
}

/* Very simple bot: alive, non-zombie players vote for a random living
 * non-self target. This is just to exercise the logic end-to-end. */
static void bots_vote(GameState *gs, int *votes) {
    for (int i = 0; i < gs->player_count; i++) {
        votes[i] = -1;
        Player *p = &gs->players[i];
        if (p->status != ALIVE || p->can_vote != 1) continue;

        int alive_others[MAX_PLAYERS], n = 0;
        for (int j = 0; j < gs->player_count; j++) {
            if (j != i && gs->players[j].status == ALIVE) alive_others[n++] = j;
        }
        if (n > 0) votes[i] = alive_others[rand() % n];
    }
}

int main(void) {

    srand((unsigned int)time(NULL));

    GameState gs;
    memset(&gs, 0, sizeof(gs));
    printf("how many player you want to play?\n");

    scanf("%d", &gs.player_count);

    char names[MAX_PLAYERS][MAX_NAME_LEN];
    for (int i = 0; i < gs.player_count; i++)
{
    printf("Enter name of player %d: ", i + 1);
    scanf("%31s", names[i]);
}

    //gs.player_count = 6; /* 6-10 supported per GDD 1.4 */
    gs.night_duration = 30; /* T seconds, configurable */

    for (int i = 0; i < gs.player_count; i++) {
        gs.players[i].id = i;
        strncpy(gs.players[i].name, names[i], MAX_NAME_LEN - 1);
        gs.players[i].points = 0;
    }

    assign_roles(&gs);
    printf("Roles assigned. day_duration=%d (3x night_duration=%d)\n",
           gs.day_duration, gs.night_duration);
    print_players(&gs);

    /* ROUND LOOP -- mirrors GDD 6.1 Master Game Loop Algorithm */
    while (gs.game_over == GAME_RUNNING) {
        printf("\n===================== DAY PHASE %d =====================\n", gs.round);
        gs.phase = PHASE_DAY;

        int votes[MAX_PLAYERS];

        bots_vote(&gs, votes);

        int eliminated_id = run_day_vote(&gs, votes);
        if (eliminated_id >= 0) {
            Player *e = &gs.players[eliminated_id];
            printf("Vote result: %s eliminated. Role revealed: %s\n", e->name, role_name(e->role));

            /* Score voters: correct if they voted for the Ghost that got eliminated */
            for (int i = 0; i < gs.player_count; i++) {
                if (votes[i] < 0) continue;
                if (gs.players[eliminated_id].role == GHOST) {
                    if (votes[i] == eliminated_id) score_correct_vote(&gs, i);
                } else {
                    score_incorrect_vote(&gs, i);
                }
            }
        } else {
            printf("Vote result: TIE / no majority -- no elimination.\n");
        }

        check_win_condition(&gs);
        if (gs.game_over != GAME_RUNNING) {
            score_game_win(&gs);
            break;
        }

        printf("\n===================== NIGHT PHASE %d =====================\n", gs.round);
        gs.phase = PHASE_NIGHT;

        /* Ghost picks a random living Villager-side target */
        Player *ghost = find_player(&gs, gs.ghost_id);
        if (ghost && ghost->status == ALIVE) {
            int candidates[MAX_PLAYERS], n = 0;
            for (int i = 0; i < gs.player_count; i++) {
                Player *p = &gs.players[i];
                if (p->status == ALIVE && (p->role == VILLAGER || p->role == WIZARD)) {
                    candidates[n++] = i;
                }
            }
            if (n > 0) {
                int target = candidates[rand() % n];
                if (ghost_kill(&gs, target)) {
                    printf("Night result: %s is now a ZOMBIE.\n", gs.players[target].name);
                    score_ghost_kill(&gs);
                }
            }
        }

        /* Wizard revives a random eliminated (non-zombie) villager, if any */
        Player *wizard = find_player(&gs, gs.wizard_id);
        if (wizard && wizard->status == ALIVE) {
            int candidates[MAX_PLAYERS], n = 0;
            for (int i = 0; i < gs.player_count; i++) {
                Player *p = &gs.players[i];
                if (p->status == ELIMINATED && p->role != ZOMBIE && p->role != GHOST) {
                    candidates[n++] = i;
                }
            }
            if (n > 0) {
                int target = candidates[rand() % n];
                if (wizard_revive(&gs, target)) {
                    printf("Night result: %s was revived.\n", gs.players[target].name);
                    score_wizard_revive(&gs);
                }
            }
        }

        score_round_survival(&gs);
        score_zombie_survival(&gs);
        print_players(&gs);

        check_win_condition(&gs);
        if (gs.game_over != GAME_RUNNING) {
            score_game_win(&gs);
            break;
        }

        gs.round++;
    }

    printf("\n===================== GAME OVER =====================\n");
    printf("Result: %s\n", gs.game_over == GAME_VILLAGER_WIN ? "VILLAGER TEAM WINS" : "GHOST TEAM WINS");
    printf("\n----- Final Leaderboard -----\n");
    print_players(&gs);

    return 0;
}
