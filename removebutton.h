#ifndef REMOVEBUTTON_H
#define REMOVEBUTTON_H

#include<QPushButton>

class RemoveButton : public QPushButton
{
    Q_OBJECT
public:
    RemoveButton();

private:
    uint row;

private slots:
    void clickToUint();

signals:
    void clicked(uint);
};

#endif // REMOVEBUTTON_H
