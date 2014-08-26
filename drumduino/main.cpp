#include "stdafx.h"
#include "drumduino.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication::setStyle(QStyleFactory::create("Fusion"));
    //QPalette p;
    //p = qApp->palette();
    //p.setColor(QPalette::Window, QColor(53,53,53));
    //p.setColor(QPalette::Button, QColor(53,53,53));
    //p.setColor(QPalette::Highlight, QColor(142,45,197));
    //p.setColor(QPalette::ButtonText, QColor(255,255,255));
    //qApp->setPalette(p);


	QApplication a(argc, argv);
	Drumduino w;
	w.show();
	return a.exec();
}
