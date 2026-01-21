#pragma once

/**
 * Item class
 *
 * These are the game items that can be collected during the game,
 * which modify weapons, stop ball movement, etc.
 * NOTE: This class is not fully implemented yet.
 */
class Item
{
private:
    int x, y;
    int id;

public:
    Item(int x, int y, int id);
    ~Item();

    int getX() const { return x; }
    int getY() const { return y; }
    int getId() const { return id; }
};