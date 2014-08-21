#include "stdafx.h"
#include "drumduino.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	drumduino w;
	w.show();
	return a.exec();
}
