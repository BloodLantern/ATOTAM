#ifndef EDIT_H
#define EDIT_H

#include "../ATOTAM/map.h"

class Edit
{
public:
    Edit(Map* map);

    virtual ~Edit();
    virtual void unmake() = 0;
    virtual void make() = 0;

    bool getMade() const;
    void setMade(bool newMade);

protected:
    Map* map = nullptr;
    bool made = false;
};

#endif // EDIT_H
