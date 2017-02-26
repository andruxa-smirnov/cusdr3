/**
* @file  main.cpp
* @brief main
* @author Hermann von Hasseln, DL3HVH
* @version 0.1
* @date 2010-09-08
*/

/*   
 *   Copyright 2010, 2011, 2012 Hermann von Hasseln, DL3HVH
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "Util/cusdr_splash.h"
#include "cusdr_settings.h"

//#if defined(Q_OS_WIN32)
//	#include "Util/cusdr_cpuUsage.h"
//#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
	#include "Util/cusdr_cpuUsage.h"
#endif

#include "cusdr_mainWidget.h"
//#include "fftw3.h"

//#include <QtGui>
//#include <QApplication>
//#include <QMessageBox>
//#include <QDebug>
//#include <QTime>
//#include <QTextBrowser>
//#include <QThread>
//#include <QtOpenGL/QGLFramebufferObject>

//#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
//#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
//#endif

// CPU usage
#if defined(Q_OS_WIN32)
	DWORD WINAPI WatchItThreadProc(LPVOID lpParam);
	CpuUsage usage;

	DWORD WINAPI WatchItThreadProc(LPVOID lpParam) {

		Q_UNUSED(lpParam)

		DWORD dummy;
		while (true) {

			short cpuUsage = usage.GetUsage();
			Settings::instance()->setCPULoad(cpuUsage);

			Sleep(1000);
		}
		return dummy;
	}
#elif defined(Q_OS_LINUX)
#include <unistd.h>

void *cpu_usage_thread_proc (void *) {
	
	CpuUsage usage;

	while (true) {

		short cpuUsage = usage.GetUsage();
		Settings::instance()->setCPULoad(cpuUsage);

		sleep(1);
	}
	return 0;
}
#endif

/*void cuSDRMessageHandler(QtMsgType type, const char *msg) {

	Q_UNUSED(type)

    QString txt;
    QDateTime date;

    txt = msg;
    txt.prepend(": ");
    txt.prepend(date.currentDateTime().toString());

    QFile outFile("cuSDR.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl << flush;
}*/

void cuSDRMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

	Q_UNUSED(type);
	Q_UNUSED(context);

		QString txt;
	QDateTime date;

	txt = msg;
	txt.prepend(": ");
	txt.prepend(date.currentDateTime().toString());

	QFile outFile("cuSDR.log");
	outFile.open(QIODevice::WriteOnly | QIODevice::Append);
	QTextStream ts(&outFile);
	ts << txt << endl << flush;

	/*
	#ifdef DEBUG
	QByteArray localMsg = msg.toLocal8Bit();
	switch (type) {
	case QtDebugMsg:
	fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
	break;
	case QtWarningMsg:
	fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
	break;
	case QtCriticalMsg:
	fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
	break;
	case QtFatalMsg:
	fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
	abort();
	}
	#endif
	*/
}

void runFFTWWisdom() {

	QString directory = QDir::currentPath();	
	QDir currentDir = QDir(directory);

	errno_t err;
	FILE* wisdomFile;

	qDebug() << "Init::\tcurrent path: " << qPrintable(currentDir.absolutePath());
	
	if (currentDir.exists("fftwf_wisdom")) {
	
		qDebug() << "Init::\tfftwf_wisdom exists.";

		/*errno_t err;
		FILE* wisdomFile;*/
		err = fopen_s(&wisdomFile, "fftwf_wisdom", "r");
		if (err == 0) {

			qDebug() << "Init::\tfound fftwf_wisdom - this will be quick!";
			fftwf_import_wisdom_from_file(wisdomFile);
			fclose(wisdomFile);
		}
		else {

			qDebug() << "Init::\tfound fftwf_wisdom but could not open.";
			return;
		}

	}
	else {
	
		qDebug() << "Init::\tfftw_wisdom does not exist - planning FFT...";
	
		//string fname = "F:\\Eigene Dokumente\\Visual Studio 2010\\Projects\\cuSDR64\\fftwf_wisdom";
		//FILE* wis = fopen (fname.c_str(), "r");
	
		/*if (wis) {

			qDebug() << "Init::\tfound fftwf_wisdom - this will be quick!";
			fftwf_import_wisdom_from_file(wis);
			fclose (wis);
		}
		else {*/
			
			int size = 64;
		
			QString str = "Init::\tplanning FFT size %1 ...";

			fftwf_complex* cpxbuf;
			fftwf_plan plan_fwd;
			fftwf_plan plan_rev;

			while (size <= MAX_FFTSIZE) {
			//while (size <= 256) {

				qDebug() << qPrintable(str.arg(size));
				cpxbuf = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * size);
			
				plan_fwd = fftwf_plan_dft_1d(size , cpxbuf, cpxbuf, FFTW_FORWARD, FFTW_PATIENT);
				fftwf_execute(plan_fwd);
				fftwf_destroy_plan(plan_fwd);

				plan_rev = fftwf_plan_dft_1d(size, cpxbuf, cpxbuf, FFTW_BACKWARD, FFTW_PATIENT);
				fftwf_execute(plan_rev);
				fftwf_destroy_plan(plan_rev);

				size *= 2;
			}
			//FILE* wis = fopen (fname.c_str(), "w");
			//if (wis) {
			
			err = fopen_s(&wisdomFile, "fftwf_wisdom", "w");
			if (err == 0) {
				
				qDebug() << "Init::\texporting fftwf_wisdom for quick measurement.";
				fftwf_export_wisdom_to_file(wisdomFile);
				qDebug() << "Init::\texported fftwf_wisdom.";
				fclose(wisdomFile);
			}
			else {

				qDebug() << "Init::\tcould not write fftwf_wisdom.";
			}
		}
}

int main(int argc, char *argv[]) {

	#ifndef DEBUG
		qInstallMessageHandler(cuSDRMessageHandler);
	#endif

    QApplication app(argc, argv);

    Settings::instance(&app);

    app.setApplicationName(Settings::instance()->getTitleStr());
    app.setApplicationVersion(Settings::instance()->getVersionStr());

	class SleeperThread : public QThread {
	
	public:
		static void msleep(unsigned long msecs) {QThread::msleep(msecs);}
	};

	
    QPixmap splash_pixmap(":/img/cusdrLogo2.png");
	
	CSplashScreen* splash = new CSplashScreen(splash_pixmap);

	splash->setGeometry(
				QStyle::alignedRect(
				Qt::LeftToRight,
				Qt::AlignCenter,
				splash->size(),
				app.desktop()->availableGeometry()));

	splash->show();

    float splash_transparency = 0;
    QTime splash_fade_timer;
    splash_fade_timer.start();

    while (splash_transparency < 1) {

        splash_transparency = (float)splash_fade_timer.elapsed() / 500;//1000;
        if (splash_transparency > 1) splash_transparency = 1;
        splash->setWindowOpacity(splash_transparency);
        splash->repaint();
		app.processEvents();
    }

	//*************************************************************************
	// load settings
	splash->showMessage(
		"\n      " + 
		Settings::instance()->getTitleStr() + " " +
		Settings::instance()->getVersionStr() +
		QObject::tr(":   Loading settings .."),
        Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

	SleeperThread::msleep(500);
    
    Settings::instance()->setSettingsFilename(QCoreApplication::applicationDirPath() +
        "/" + Settings::instance()->getSettingsFilename());

    Settings::instance()->setSettingsLoaded(Settings::instance()->loadSettings() >= 0);

    if (Settings::instance()->getSettingsLoaded()) {

        splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   Settings loaded."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

		SleeperThread::msleep(100);
    }
    else {

        splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   Settings not loaded."),
			Qt::AlignTop | Qt::AlignLeft, Qt::red);

		//splash->showMessage("\n      " + QObject::tr(copyright), Qt::AlignBottom | Qt::AlignLeft, Qt::black);
    }

	// ****************************
	// check for OpenGL

	splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   Checking for OpenGL V 2.0 ..."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

	SleeperThread::msleep(100);

	if (!QGLFormat::hasOpenGL() && QGLFormat::OpenGL_Version_2_0) {

		qDebug() << "Init::\tOpenGL not found!";
		splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   not found!"),
			Qt::AlignTop | Qt::AlignLeft, Qt::red);

		splash->hide();

		QMessageBox::critical(0, 
			QApplication::applicationName(), 
			QApplication::applicationName() + "   requires OpenGL v2.0 or later to run.", 
			QMessageBox::Abort);

		return -1;
	}

	if (!(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0)) {

		qDebug() << "Init::\tOpenGL found, but appears to be less than OGL v2.0.";
		splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   found but appears to be less than OGL v2.0"),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

		splash->hide();

		QMessageBox::critical(0, 
			QApplication::applicationName(), 
			QApplication::applicationName() + "   requires OpenGL v2.0 or later to run.", 
			QMessageBox::Ok);

		return -1;
	}
	//if (QGLFormat::OpenGL_Version_2_0)
	//	qDebug() << "OpenGL version > 2.0";

	qDebug() << "Init::\tOpenGL found.";
	splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   OpenGL found."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

	SleeperThread::msleep(100);

	if (!QGLFramebufferObject::hasOpenGLFramebufferObjects()) {

		qDebug() << "Init::Framebuffer Objects not found!\n";
		splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   fbuffers not found!"),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

		Settings::instance()->setFBOPresence(false);
		SleeperThread::msleep(100);
		return -1;
		//splash->hide();
	}
	else {

		qDebug() << "Init::\tFramebuffer Objects found.";
		splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   OpenGL Frame Buffer support found."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

		Settings::instance()->setFBOPresence(true);
	}
	SleeperThread::msleep(100);

	// ****************************
	// check for OpenCL devices
	/*QList<QCLDevice> clDevices = QCLDevice::allDevices();
	
	splash->showMessage(
			"\n      " + 
			Settings::instance()->titleStr() + " " +
			Settings::instance()->versionStr() + 
			QObject::tr(":   Checking for OpenCL devices..."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);
	SleeperThread::msleep(100);

	if (clDevices.length() == 0) {
	
		qDebug() << "Init:: no OpenCL devices found!\n";
		splash->showMessage(
			"\n      " + 
			Settings::instance()->titleStr() + " " +
			Settings::instance()->versionStr() + 
			QObject::tr(":   no OpenCL devices found!"),
			Qt::AlignTop | Qt::AlignLeft, Qt::red);

		SleeperThread::msleep(1000);
	}
	else {

		qDebug() << "Init:: found" << clDevices.length() << "OpenCL device(s).";
		QString clNo = QString::number(clDevices.length());
		splash->showMessage(
			"\n      " + 
			Settings::instance()->titleStr() + " " +
			Settings::instance()->versionStr() + 
			QObject::tr(":   found ") +	clNo +
			QObject::tr(" OpenCL device(s)."),
			Qt::AlignTop | Qt::AlignLeft, Qt::green);

		SleeperThread::msleep(200);
	}*/

	//for (int i = 0; i < clDevices.length(); i++)
	//	qDebug() << clDevices.at(i);


	//QCLContext context;
	////if (!context.create(QCLDevice::CPU)) {
	////if (!context.create()) {
	//if (!context.create(QCLDevice::GPU)) {

	//	qDebug() << "Could not create OpenCL context.";
	//  return 1;
	//}
	//else
	//	qDebug() << "OpenCL context for the GPU created.";

	//QCLVector<int> input1 = context.createVector<int>(2048);
	//QCLVector<int> input2 = context.createVector<int>(2048);
	//   
	//for (int index = 0; index < 2048; ++index) {

	//    input1[index] = index;
	//    input2[index] = 2048 - index;
	//}

	//QCLVector<int> output = context.createVector<int>(2048);

	//qDebug() << "build OpenCL program from source...";
	//QCLProgram program = context.buildProgramFromSourceFile(":/cl/vectoradd.cl");
	//qDebug() << "done.";
	//QCLKernel kernel = program.createKernel("vectorAdd");

	//kernel.setGlobalWorkSize(2048);
	//kernel(input1, input2, output);

	//for (int index = 0; index < 2048; ++index) {

	//   if (output[index] != 2048) {

	//        qDebug() << "Answer at index %1 is %2, should be %3." <<  index << output[index] << 2048;
	//        return 1;
	//   }
	//}

	//qDebug() << "Answer is correct:" << 2048;

	// cpu usage
#if defined(Q_OS_WIN32)
	CreateThread(NULL, 0, WatchItThreadProc, NULL, 0, NULL);
#endif

#if defined(Q_OS_LINUX)
#include <pthread.h>

	pthread_t cpu_usage_thread;
	pthread_create(&cpu_usage_thread, 0, cpu_usage_thread_proc, 0);
#endif

	// ****************************
	// setup main window

	splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   setting up main window .."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

	qDebug() << "Init::\tmain window setup ...";
	MainWindow mainWindow;
	mainWindow.setup();
	qDebug() << "Init::\tmain window setup done.";

	splash->showMessage(
			"\n      " + 
			Settings::instance()->getTitleStr() + " " +
			Settings::instance()->getVersionStr() +
			QObject::tr(":   Displaying main window .."),
			Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

	SleeperThread::msleep(300);
	
	//*************************************************************************
	// FFTW wisdom

	QString directory = QDir::currentPath();	
	QDir currentDir = QDir(directory);

	errno_t err;
	FILE* wisdomFile;

	qDebug() << "Init::\tcurrent path: " << qPrintable(currentDir.absolutePath());

	if (currentDir.exists("fftwf_wisdom")) {
	
		qDebug() << "Init::\tfftwf_wisdom exists.";

		err = fopen_s(&wisdomFile, "fftwf_wisdom", "r");
		if (err == 0) {

			qDebug() << "Init::\tfound fftwf_wisdom - this will be quick!";
			splash->showMessage(
				"\n      " +
				Settings::instance()->getTitleStr() + " " +
				Settings::instance()->getVersionStr() +
				QObject::tr(":   found fftwf_wisdom - loading .."),
				Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

			fftwf_import_wisdom_from_file(wisdomFile);
			fclose(wisdomFile);
		}
		else {

			qDebug() << "Init::\tfound fftwf_wisdom but could not open.";
			return -1;
		}

	}
	else {
	
		qDebug() << "Init::\tfftw_wisdom does not exist - planning FFT...";
	
		QString msg = "First time FFT planning. Please be patient, this may take up to 10 min!";
		mainWindow.showWarningDialog(msg);

		int size = 64;
		QString str = "planning FFT size %1 ...";

		fftwf_complex* cpxbuf;
		fftwf_plan plan_fwd;
		fftwf_plan plan_rev;

		while (size <= MAX_FFTSIZE) {
		//while (size <= 2048) {

			qDebug() << qPrintable(str.arg(size));
			splash->showMessage(
				"\n      " +
				Settings::instance()->getTitleStr() + " " +
				Settings::instance()->getVersionStr() +
				QObject::tr(":   ") + str.arg(size),
				Qt::AlignTop | Qt::AlignLeft, Qt::yellow);

			cpxbuf = (fftwf_complex *) fftwf_malloc(sizeof(fftwf_complex) * size);
			
			plan_fwd = fftwf_plan_dft_1d(size , cpxbuf, cpxbuf, FFTW_FORWARD, FFTW_PATIENT);
			fftwf_execute(plan_fwd);
			fftwf_destroy_plan(plan_fwd);

			plan_rev = fftwf_plan_dft_1d(size, cpxbuf, cpxbuf, FFTW_BACKWARD, FFTW_PATIENT);
			fftwf_execute(plan_rev);
			fftwf_destroy_plan(plan_rev);

			size *= 2;
		}
			
		err = fopen_s(&wisdomFile, "fftwf_wisdom", "w");
		if (err == 0) {
				
			qDebug() << "Init::\texporting fftwf_wisdom for quick measurement.";
			fftwf_export_wisdom_to_file(wisdomFile);
			qDebug() << "Init::\texported fftwf_wisdom.";
			fclose(wisdomFile);
		}
		else {

			qDebug() << "Init::\tcould not write fftwf_wisdom.";
		}
	}

	mainWindow.show();
    app.processEvents();

	/*splash_fade_timer.restart();
	while (splash_transparency > 0)
	{
		splash_transparency = 1 - (float)splash_fade_timer.elapsed() / 300;
		if (splash_transparency < 0) splash_transparency = 0;
		if (splash_transparency > 1) splash_transparency = 1;
		splash->setWindowOpacity(splash_transparency);
		splash->repaint();
	}*/
	//splash->hide();

	delete splash;

	mainWindow.update();
	mainWindow.setFocus();

	qDebug() << "Init::\trunning application ...\n";
	
    return app.exec();
}
