#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"

#include "commondefs.h"

#include <osgViewer/GraphicsWindow>
#include <OpenThreads/ReadWriteMutex>
#include <QOpenGLWidget>
#include <QOpenGLWindow>

namespace osgViewer { class ViewerBase; }
namespace osgGA { class EventQueue; }
class QInputEvent;


mClass(uiOSG) ODOpenGLWidget : public QOpenGLWidget
{
public:
			ODOpenGLWidget(QWidget* parent=nullptr,
				       Qt::WindowFlags f=Qt::WindowFlags());
    virtual		~ODOpenGLWidget();

    osgViewer::GraphicsWindowEmbedded*
			getGraphicsWindow()	{ return graphicswindow_; }
    void		setViewer(osgViewer::ViewerBase*);

protected:

    void		initializeGL() override;
    void		paintGL() override;
    void		resizeGL(int w,int h) override;

    void		setKeyboardModifiers(QInputEvent*);

    void		keyPressEvent(QKeyEvent*) override;
    void		keyReleaseEvent(QKeyEvent*) override;
    void		mousePressEvent(QMouseEvent*) override;
    void		mouseReleaseEvent(QMouseEvent*) override;
    void		mouseDoubleClickEvent(QMouseEvent*) override;
    void		mouseMoveEvent(QMouseEvent*) override;
    void		wheelEvent(QWheelEvent*) override;

    osgGA::EventQueue*	getEventQueue() const;

private:
    osgViewer::GraphicsWindowEmbedded*
				graphicswindow_;
    osgViewer::ViewerBase*	viewer_		= nullptr;
    OpenThreads::ReadWriteMutex mutex_;

    bool			isfirstframe_	= true;
    double			scalex_		= 1;
    double			scaley_		= 1;
};


mClass(uiOSG) ODOpenGLWindow : public QOpenGLWindow
{
public:
			ODOpenGLWindow(QWidget* parent=nullptr);
    virtual		~ODOpenGLWindow();

    osgViewer::GraphicsWindowEmbedded*
			getGraphicsWindow()	{ return graphicswindow_; }
    void		setViewer(osgViewer::ViewerBase*);

    QWidget*		qWidget()		{ return qwidget_; }

protected:

    void		initializeGL() override;
    void		paintGL() override;
    void		resizeGL(int w,int h) override;

    void		setKeyboardModifiers(QInputEvent*);

    void		keyPressEvent(QKeyEvent*) override;
    void		keyReleaseEvent(QKeyEvent*) override;
    void		mousePressEvent(QMouseEvent*) override;
    void		mouseReleaseEvent(QMouseEvent*) override;
    void		mouseDoubleClickEvent(QMouseEvent*) override;
    void		mouseMoveEvent(QMouseEvent*) override;
    void		wheelEvent(QWheelEvent*) override;

    osgGA::EventQueue*	getEventQueue() const;

private:
    QWidget*			qwidget_;
    osgViewer::GraphicsWindowEmbedded*
				graphicswindow_;
    osgViewer::ViewerBase*	viewer_		= nullptr;
    OpenThreads::ReadWriteMutex mutex_;

    bool			isfirstframe_	= true;
    double			scalex_		= 1;
    double			scaley_		= 1;
};
