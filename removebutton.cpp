#include "removebutton.h"

RemoveButton::RemoveButton()
{
    connect(this, SIGNAL(clicked()), this, SLOT(clickToUint()));
}

void RemoveButton::clickToUint(){
    emit clicked(row);
}
