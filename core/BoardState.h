#pragma once

#include <vector>
#include <cstdint>
#include <utility>

// Lightweight, thread-safe snapshot of the board for the GUI.
// This struct lives in core/ so it can be shared between core and GUI
// without any GTK dependency.

struct BoardState {
    int boardSize = 15;

    enum Cell : uint8_t { EMPTY = 0, BLACK = 1, WHITE = 2 };

    std::vector<Cell> cells;         // boardSize * boardSize
    int lastMoveX = -1;             // highlight the last move
    int lastMoveY = -1;
    std::vector<std::pair<int,int>> moveOrder;  // ordered (x,y) for numbering

    BoardState() : boardSize(15), cells(15*15, EMPTY) {}

    Cell at(int x, int y) const {
        if (x < 0 || x >= boardSize || y < 0 || y >= boardSize) return EMPTY;
        return cells[y * boardSize + x];
    }

    void set(int x, int y, Cell c) {
        if (x >= 0 && x < boardSize && y >= 0 && y < boardSize)
            cells[y * boardSize + x] = c;
    }

    void resize(int size) {
        boardSize = size;
        cells.assign(size * size, EMPTY);
        moveOrder.clear();
        lastMoveX = lastMoveY = -1;
    }
};
