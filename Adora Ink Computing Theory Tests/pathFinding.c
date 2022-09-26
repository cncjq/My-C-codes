// pathFinding.c
// written by Jiaqiang Chen, Sep 2022

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BOARD_ROW 8
#define BOARD_COL 8
#define VAL_NUM 2
#define KNIGHT_DIRECTION 8
#define MAX_MARK 64

// Chess representation
enum Chess {
    EMPTY = '_',
    KING = 'K',
    KNIGHT = 'N'
};

// Struct for the position of king and knight, each contains 2 axises
typedef struct {
    int king[VAL_NUM];
    int knight[VAL_NUM];
} Position;

// The path struct, including coordinates, steps of path, and last position
typedef struct {
    int pathX;
    int pathY;
    int steps;
    int* prePosition;
} Path;

// Initial the chess board
char** init_board(void) {
    char** board = malloc(sizeof(char*) * BOARD_ROW);
    for (int i = 0; i < BOARD_ROW; i++) {
	board[i] = malloc(sizeof(char) * BOARD_COL);
	 for (int j = 0; j < BOARD_COL; j++) {
	    board[i][j] = EMPTY;
	 }
    }
    return board;
}

// Randomly place chess on the board
Position add_chess(char** chessBoard) {
    Position position;
    position.king[0] = rand() % BOARD_ROW;
    position.king[1] = rand() % BOARD_COL;
    chessBoard[position.king[0]][position.king[1]] = KING;
    position.knight[0] = rand() % BOARD_ROW;
    position.knight[1] = rand() % BOARD_COL;
    chessBoard[position.knight[0]][position.knight[1]] = KNIGHT;
    return position;
}

// check if a step goes outside the boundary
bool check_step(int x, int y) {
    if (x < 0 || y < 0 || x >= BOARD_ROW || y >= BOARD_COL) {
	return false;
    }
    return true;
}

// Looking for the last path to a certain position
Path* find_path(Path* path, char** board, Position position) {
    path->prePosition = realloc(path->prePosition, sizeof(int)*2);
    int direction[KNIGHT_DIRECTION][VAL_NUM] = {{1, -2}, {2, -1}, {2, 1}, 
	{1, 2}, {-1, 2}, {-2, 1},{-1, -2}, {-2, -1}};
    int visited[BOARD_ROW][BOARD_COL] = {{0}};
    path->steps = 0;
    int mark[MAX_MARK][VAL_NUM];
    int markNum = 0;
    visited[position.knight[0]][position.knight[1]] = 1;
    while (!visited[position.king[0]][position.king[1]]) {
	for (int i = 0; i < BOARD_ROW; i++) {
	    for (int j = 0; j < BOARD_COL; j++) {
		if (visited[i][j]) {
		    mark[markNum][0] = i;
		    mark[markNum][1] = j;
		    markNum++;
		}
	    }
	}
	while (markNum) {
	    markNum--;
	    for (int k = 0; k < KNIGHT_DIRECTION; k++) {
		if (check_step(mark[markNum][0]+direction[k][0], 
			    mark[markNum][1]+direction[k][1])) {
		    if (visited[mark[markNum][0]+direction[k][0]]
			    [mark[markNum][1]+direction[k][1]]) {
			continue;
		    } else {
			visited[mark[markNum][0]+direction[k][0]]
			    [mark[markNum][1]+direction[k][1]] = 1;
			if (visited[position.king[0]][position.king[1]]) {
			    path->prePosition[0] = mark[markNum][0];
			    path->prePosition[1] = mark[markNum][1];
			    break;
			}
		    }
		}
	    }
	    if (visited[position.king[0]][position.king[1]]) {
		break;
	    }
	}
	path->steps++;
    }
    return path;
}

void free_board(char** board) {
    for (int i = 0; i < BOARD_ROW; i++) {
	    free(board[i]);
    }
}

int main(int argc, char** argv) {
    char** chessBoard = init_board();
    Position position;
    position = add_chess(chessBoard);
    printf("The chess board is:\n");
    for (int i = 0; i < BOARD_ROW; i++) {
	printf("%s\n", chessBoard[i]);
    }
    printf("King's position: (%d %d)\n", position.king[0], position.king[1]);
    int finalPosition[VAL_NUM];
    finalPosition[0] = position.king[0];
    finalPosition[1] = position.king[1];
    printf("Knight's position: (%d %d)\n", position.knight[0],
	    position.knight[1]);
    Path* path = malloc(sizeof(Path));    
    int steps;
    path = find_path(path, chessBoard, position);
    steps = path->steps;
    int answer[steps][VAL_NUM];
    answer[0][0] = path->prePosition[0];
    answer[0][1] = path->prePosition[1];
    for (int i = 1; i < steps; i++) {
	position.king[0] = path->prePosition[0];
	position.king[1] = path->prePosition[1];
	path = find_path(path, chessBoard, position);
	answer[i][0] = path->prePosition[0];
	answer[i][1] = path->prePosition[1];
    }
    free(path->prePosition);
    free(path);
    printf("A set of path is:\n");
    printf("{");
    for (int i = 0; i < steps; i++) {
	printf("(%d, %d) ", answer[steps-1-i][0], answer[steps-1-i][1]);
    }
    printf("(%d, %d)}\n", finalPosition[0], finalPosition[1]);
    free_board(chessBoard);
    free(chessBoard);
    return 0;
}
