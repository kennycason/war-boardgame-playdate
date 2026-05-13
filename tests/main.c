#include <stdio.h>
#include "test.h"

int g_test_count    = 0;
int g_test_failed   = 0;
int g_section_count = 0;
int g_section_failed= 0;
const char* g_current_test = "";

void run_piece_tests(void);
void run_board_tests(void);
void run_move_tests(void);
void run_game_tests(void);
void run_ai_tests(void);
void run_history_tests(void);

int main(void) {
    run_piece_tests();
    run_board_tests();
    run_move_tests();
    run_game_tests();
    run_ai_tests();
    run_history_tests();

    printf("\n----------------------------------------\n");
    printf("Total: %d assertions, %d failed\n", g_test_count, g_test_failed);
    if (g_test_failed == 0) {
        printf(TEST_PASS_TAG " all tests passed\n");
        return 0;
    } else {
        printf(TEST_FAIL_TAG " %d failures\n", g_test_failed);
        return 1;
    }
}
