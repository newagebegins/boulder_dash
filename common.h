#ifndef COMMON_H
#define COMMON_H

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))

struct Position {
    int x, y;

    Position(int X, int Y) {
        x = X;
        y = Y;
    }
};

#endif
