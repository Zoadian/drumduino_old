#include "stdafx.h"
#include "drumduino.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Drumduino w;
	w.show();
	return a.exec();
}
