#ifndef Q_zoom_h
#define Q_zoom_h
#include "ui_zoom.h"
#include "ADM_image.h"
#include "DIA_flyDialogQt4.h"
#include "DIA_flyZoom.h"
#include "zoom.h"
#if 1
class Ui_zoomWindow : public QDialog
{
	Q_OBJECT

protected: 
	int lock;

public:
	flyZoom *myFly;
	ADM_QCanvas *canvas;
	Ui_zoomWindow(QWidget* parent, zoom *param,ADM_coreVideoFilter *in);
	~Ui_zoomWindow();
	Ui_zoomDialog ui;

public slots:
	void gather(zoom *param);

private slots:
	void sliderUpdate(int foo);
	void valueChanged(int foo);
	void reset(bool f);

private:
        void resizeEvent(QResizeEvent *event);
};

#endif	// Q_zoom_h
#endif
