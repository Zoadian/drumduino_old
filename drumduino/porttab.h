#ifndef PORTTAB_H
#define PORTTAB_H

#include <QWidget>
#include "ui_porttab.h"

class PortTab : public QWidget
{
	Q_OBJECT

public:
	PortTab(QWidget *parent = 0);
	~PortTab();

private:
	Ui::PortTab ui;
};

#endif // PORTTAB_H
