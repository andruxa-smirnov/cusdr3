/*
 *	Windows part for CPU usage by (c) 2009 Ben Watson
 *
 *	taken from http://www.philosophicalgeek.com/2009/01/03/determine-cpu-usage-of-current-process-c-and-c/
 *
 *  adapted for cuSDR by (c) 2012  Hermann von Hasseln, DL3HVH
 *
 *
 *  Linux part for CPU usage by (c) by Fabian Holler
 *
 *  taken from https://github.com/fho/code_snippets/blob/master/c/getusage.c
 *
 *  adapted for cuSDR by (c) 2012 Andrea Montefusco, IW0HDV
 *
 */

#include <QtGlobal>   // needed in order to get Q_OS_LINUX macro defined

#include <QThread>
#include <QTimer>


#if defined(Q_OS_WIN32)

#pragma once

//#define _WIN32_WINNT 0�0501
#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#include <windows.h>

class CpuUsage {

public:
	CpuUsage(void);
	
	short  GetUsage();

private:
	ULONGLONG SubtractTimes(const FILETIME& ftA, const FILETIME& ftB);
	bool EnoughTimePassed();
	inline bool IsFirstRun() const { return (m_dwLastRun == 0); }
	
	//system total times
	FILETIME m_ftPrevSysKernel;
	FILETIME m_ftPrevSysUser;

	//process times
	FILETIME m_ftPrevProcKernel;
	FILETIME m_ftPrevProcUser;

	short m_nCpuUsage;
	ULONGLONG m_dwLastRun;
	
	volatile LONG m_lRunCount;
};

#elif defined(Q_OS_LINUX)

class CpuUsage : public QThread {
    Q_OBJECT

public:
    clock_t		ptick;
    double		ptime;
    QTimer*		timer;

    cusdr_cpuUsage();

private:
    int CLOCK_TICK;

private slots:
    void getCPUUsage();
};

#endif
