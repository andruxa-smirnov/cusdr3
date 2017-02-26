/**
* @file cusdr_receiver.h
* @brief cuSDR receiver header file
* @author Hermann von Hasseln, DL3HVH
* @version 0.1
* @date 2010-11-12
*/

/* Copyright (C)
*
* 2010 - Hermann von Hasseln, DL3HVH
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#ifndef CUSDR_RECEIVER_H
#define CUSDR_RECEIVER_H

//#include <QObject>
//#include <QtNetwork>

#include "cusdr_settings.h"
#include "cusdr_qComplex.h"
#include "cusdr_DttSP.h"
#include "cusdr_highResTimer.h"


class Receiver : public QObject {

	Q_OBJECT
	Q_PROPERTY(QSDR::_ServerMode serverMode READ getServerMode WRITE setServerMode)
	Q_PROPERTY(SocketState socketState READ socketState WRITE setSocketState)
	Q_ENUMS(SocketState)

public:
	Receiver(QObject *parent = 0, int rx = 0);
	~Receiver();

	enum SocketState { 
		
		RECEIVER_DETACHED, 
		RECEIVER_ATTACHED
	};

	void				setServerMode(QSDR::_ServerMode mode);
	QSDR::_ServerMode	getServerMode()	const;

	void 		setSocketState(SocketState state);
	SocketState socketState() const;


	QHostAddress	getPeerAddress()		{ return m_peerAddress; }
	int				getSocketDescriptor()	{ return m_socketDescriptor; }
	int				getReceiver()			{ return m_receiver; }
	int				getClient()				{ return m_client; }
	int				getIQPort()				{ return m_iqPort; }
	int				getBSPort()				{ return m_bsPort; }
	int				getID()					{ return m_receiverID; }
	int				getSampleRate()			{ return m_sampleRate; }
	int				getDisplayDelay()		{ return m_displayTime; }

	HamBand			getHamBand()			{ return m_hamBand; }
	//DSPMode			getDSPMode()			{ return m_dspMode; }
	AGCMode			getAGCMode()			{ return m_agcMode; }
	QList<int>		getMercuryAttenuators() { return m_mercuryAttenuators; }
	QList<DSPMode>	getDSPModeList()		{ return m_dspModeList; }

	qreal			getAGCGain()			{ return m_agcGain; }
	float			getAudioVolume()		{ return m_audioVolume; }
	long			getFrequency()			{ return m_frequency; }
	double			getFilterLo()			{ return m_filterLo; }
	double			getFilterHi()			{ return m_filterHi; }
	qreal			getdBmPanScaleMin()		{ return m_dBmPanScaleMin; }
	qreal			getdBmPanScaleMax()		{ return m_dBmPanScaleMax; }

	bool			getConnectedStatus()	{ return m_connected; }

	bool			initDttSPInterface();

    float	in[BUFFER_SIZE * 2];
    float	out[BUFFER_SIZE * 2];
	float	temp[BUFFER_SIZE * 4];
	float	spectrum[BUFFER_SIZE * 4];
	float	postSpectrum[BUFFER_SIZE * 4];

	QDttSP		*dttsp;
	HResTimer	*highResTimer;

	CPX			inBuf;
    CPX			outBuf;

public slots:
	void	setReceiverData(TReceiver data);
	void	setPeerAddress(QHostAddress addr);
	void	setSocketDescriptor(int value);
	void	setReceiver(int value);
	void	setClient(int value);
	void	setIQPort(int value);
	void	setBSPort(int value);
	void	setConnectedStatus(bool value);
	void	setID(int value);
	void	setSampleRate(int value);
	void	setHamBand(QObject* sender, int rx, bool byBtn, HamBand band);
	void	setDspMode(QObject* sender, int rx, DSPMode mode);
	void	setAGCMode(QObject* sender, int rx, AGCMode mode);
	void	setAGCGain(QObject* sender, int rx, int value);
	void	setAudioVolume(QObject* sender, int rx, float value);
	void	setFrequency(long frequency);
	void	setFilterFrequencies(QObject* sender, int rx, qreal low, qreal high);
	void	setLastFrequencyList(const QList<long> &frequencies);
	void	setdBmPanScaleMin(qreal value);
	void	setdBmPanScaleMax(qreal value);
	void	setMercuryAttenuators(const QList<int> &attenuators);

private slots:
	void	setSystemState(
					QObject* sender, 
					QSDR::_Error err, 
					QSDR::_HWInterfaceMode hwmode, 
					QSDR::_ServerMode mode, 
					QSDR::_DataEngineState state);

	void 	setFramesPerSecond(QObject *sender, int rx, int value);

private:
	Settings		*set;
	SocketState		m_socketState;

	QSDR::_ServerMode		m_serverMode;
	QSDR::_HWInterfaceMode	m_hwInterface;
	QSDR::_DataEngineState	m_dataEngineState;

	TReceiver 		m_receiverData;
	QHostAddress	m_peerAddress;
	quint16			m_peerPort;

	HamBand				m_hamBand;
	DSPMode				m_dspMode;
	AGCMode				m_agcMode;
	TDefaultFilterMode	m_filterMode;

	QList<long>			m_lastFrequencyList;
	QList<DSPMode>		m_dspModeList;
	QList<int>			m_mercuryAttenuators;

	int		m_socketDescriptor;
	int		m_receiver;
	int		m_client;
    int		m_iqPort;
    int		m_bsPort;
	int		m_receiverID;
	int		m_sampleRate;
	int		m_agcGain;
	int		m_displayTime;

	long	m_frequency;

	float	m_audioVolume;

	qreal	m_filterLo;
	qreal	m_filterHi;
	qreal	m_dBmPanScaleMin;
	qreal	m_dBmPanScaleMax;

	bool	m_connected;

	void	setupConnections();

signals:
	void	messageEvent(QString msg);
};

#endif  // CUSDR_RECEIVER_H
