/**
* @file cusdr_receiver.cpp
* @brief cuSDR receiver class
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
#define LOG_RECEIVER

#include "cusdr_receiver.h"

Receiver::Receiver(QObject *parent, int rx)
	: QObject(parent)
	, set(Settings::instance())
	, m_filterMode(set->getCurrentFilterMode())
	, m_receiverID(rx)
{
	setReceiverData(set->getReceiverDataList().at(m_receiverID));

	inBuf.resize(BUFFER_SIZE);
	outBuf.resize(BUFFER_SIZE);

	//if (initDttSPInterface())
	//	RECEIVER_DEBUG << "DttSP for receiver: " << m_receiverID << " started.";

	setupConnections();

	highResTimer = new HResTimer();
	m_displayTime = (int)(1000000.0/set->getFramesPerSecond(m_receiverID));
}

Receiver::~Receiver() {
}

void Receiver::setupConnections() {

	CHECKED_CONNECT(
		set, 
		SIGNAL(systemStateChanged(
					QObject *, 
					QSDR::_Error, 
					QSDR::_HWInterfaceMode, 
					QSDR::_ServerMode, 
					QSDR::_DataEngineState)), 
		this, 
		SLOT(setSystemState(
					QObject *, 
					QSDR::_Error, 
					QSDR::_HWInterfaceMode, 
					QSDR::_ServerMode, 
					QSDR::_DataEngineState)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(mainVolumeChanged(QObject *, int, float)), 
		this, 
		SLOT(setAudioVolume(QObject *, int, float)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(dspModeChanged(QObject *, int, DSPMode)), 
		this, 
		SLOT(setDspMode(QObject *, int, DSPMode)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(hamBandChanged(QObject *, int, bool, HamBand)),
		this, 
		SLOT(setHamBand(QObject *, int, bool, HamBand)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(agcModeChanged(QObject *, int, AGCMode)), 
		this, 
		SLOT(setAGCMode(QObject *, int, AGCMode)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(agcGainChanged(QObject *, int, int)), 
		this, 
		SLOT(setAGCGain(QObject *, int, int)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(filterFrequenciesChanged(QObject *, int, qreal, qreal)), 
		this, 
		SLOT(setFilterFrequencies(QObject *, int, qreal, qreal)));

	CHECKED_CONNECT(
		set,
		SIGNAL(framesPerSecondChanged(QObject*, int, int)),
		this,
		SLOT(setFramesPerSecond(QObject*, int, int)));

}

void Receiver::setReceiverData(TReceiver data) {

	m_receiverData = data;

	m_sampleRate = m_receiverData.sampleRate;
	m_hamBand = m_receiverData.hamBand;
	//m_dspMode = m_receiverData.dspMode;
	m_dspModeList = m_receiverData.dspModeList;
	m_agcMode = m_receiverData.agcMode;
	m_agcGain = m_receiverData.acgGain;

	m_audioVolume = m_receiverData.audioVolume;

	if (m_filterLo != m_receiverData.filterLo || m_filterHi != m_receiverData.filterHi) {

		m_filterLo = m_receiverData.filterLo;
		m_filterHi = m_receiverData.filterHi;
	}

	m_lastFrequencyList = m_receiverData.lastFrequencyList;
	m_mercuryAttenuators = m_receiverData.mercuryAttenuators;

}

bool Receiver::initDttSPInterface() {

	dttsp = new QDttSP(this, m_receiverID);

	if (dttsp)
		dttsp->setDttSPStatus(true);
	else {

		RECEIVER_DEBUG << "could not start DttSP for receiver: " << m_receiverID;
		return false;
	}

	dttsp->dttspSetTRX(0, 0); // thread 0 is for receive; 1st arg = thread; 2nd arg: 0 = Rx,  1 = Tx

	int offset = 0;
	dttsp->dttspSetRingBufferOffset(0, offset);

	// 1st arg = thread; 2nd arg = RunMode: 0 = RUN_MUTE, 1 = RUN_PASS, 2 = RUN_PLAY, 3 = RUN_SWCH
	dttsp->dttspSetThreadProcessingMode(0, 2);

	// 1st arg = thread; 2nd arg = sub-receiver number; 3rd arg: 0 = inactive, 1 = active
	dttsp->dttspSetSubRXSt(0, 0, 1);

	 // 1st arg = thread; 2nd arg = sub-receiver number; 3rd arg = audio gain from 0.0 to 1.0
	dttsp->dttspSetRXOutputGain(0, 0, set->getMainVolume(m_receiverID));
	dttsp->dttspSetDSPBuflen(0, BUFFER_SIZE);

	//dttsp->setDttSPStatus(true);

	//dttsp->dttspSetSpectrumPolyphase(0, true);
	//dttsp->dttspSetSpectrumWindow(0, HANNING_WINDOW);

	dttsp->dttspSetSampleRate((double)m_sampleRate);
	dttsp->dttspSetRXOsc(0, 0, 0.0);


	return true;
}

void Receiver::setServerMode(QSDR::_ServerMode mode) {

	m_serverMode = mode;
}

QSDR::_ServerMode Receiver::getServerMode()	const {

	return m_serverMode;
}

void Receiver::setSocketState(SocketState state) {

	m_socketState = state;
}

Receiver::SocketState Receiver::socketState() const {

	return m_socketState;
}

void Receiver::setSystemState(
	QObject *sender,
	QSDR::_Error err,
	QSDR::_HWInterfaceMode hwmode,
	QSDR::_ServerMode mode,
	QSDR::_DataEngineState state)
{
	Q_UNUSED (sender)
	Q_UNUSED (err)

	//bool change = false;

	if (m_hwInterface != hwmode) {

		m_hwInterface = hwmode;
		//change = true;
	}

	if (m_serverMode != mode) {

		m_serverMode = mode;
		//change = true;
	}

	if (m_dataEngineState != state) {

		m_dataEngineState = state;
		//change = true;
	}
}

void Receiver::setPeerAddress(QHostAddress addr) {

	m_peerAddress = addr;
}

void Receiver::setSocketDescriptor(int value) {

	m_socketDescriptor = value;
}

void Receiver::setReceiver(int value) {

	m_receiver = value;
}

void Receiver::setClient(int value) {

	m_client = value;
}

void Receiver::setIQPort(int value) {

	m_iqPort = value;
}

void Receiver::setBSPort(int value) {

	m_bsPort = value;
}

void Receiver::setConnectedStatus(bool value) {

	m_connected = value;
}

void Receiver::setID(int value) {

	m_receiverID = value;
}

void Receiver::setSampleRate(int value) {

	m_sampleRate = value;
}

void Receiver::setHamBand(QObject *sender, int rx, bool byBtn, HamBand band) {

	Q_UNUSED(sender)
	Q_UNUSED(byBtn)

	if (m_receiver == rx) {
		
		if (m_hamBand == band) return;
		m_hamBand = band;
	}
}

void Receiver::setDspMode(QObject *sender, int rx, DSPMode mode) {

	Q_UNUSED(sender)

	if (m_receiver == rx) {

		if (m_dspMode == mode) return;
		m_dspMode = mode;
	}

	QString msg = "[receiver]: set mode for receiver %1 to %2";
	emit messageEvent(msg.arg(rx).arg(set->getDSPModeString(m_dspMode)));
}

void Receiver::setAGCMode(QObject *sender, int rx, AGCMode mode) {

	Q_UNUSED(sender)
	
	if (m_receiver == rx) {

		if (m_agcMode == mode) return;
		m_agcMode = mode;
	}
}

void Receiver::setAGCGain(QObject *sender, int rx, int value) {

	Q_UNUSED(sender)
	
	if (m_receiver == rx) {

		if (m_agcGain == value) return;
		m_agcGain = value;
	}
}

void Receiver::setAudioVolume(QObject *sender, int rx, float value) {

	Q_UNUSED(sender)
	
	if (m_receiver == rx) {

		if (m_audioVolume == value) return;
		m_audioVolume = value;
	}
}

void Receiver::setFilterFrequencies(QObject *sender, int rx, double low, double high) {

	Q_UNUSED(sender)
	
	if (m_receiver == rx) {

		if (m_filterLo == low && m_filterHi == high) return;
		m_filterLo = low;
		m_filterHi = high;
	}
}

void Receiver::setFrequency(long frequency) {

	if (m_frequency == frequency) return;
	m_frequency = frequency;

	HamBand band = getBandFromFrequency(set->getBandFrequencyList(), frequency);
	m_lastFrequencyList[(int) band] = m_frequency;
}

void Receiver::setLastFrequencyList(const QList<long> &fList) {

	m_lastFrequencyList = fList;
}

void Receiver::setdBmPanScaleMin(qreal value) {

	if (m_dBmPanScaleMin == value) return;
	m_dBmPanScaleMin = value;
}

void Receiver::setdBmPanScaleMax(qreal value) {

	if (m_dBmPanScaleMax == value) return;
	m_dBmPanScaleMax = value;
}

void Receiver::setMercuryAttenuators(const QList<int> &attenuators) {

	m_mercuryAttenuators = attenuators;
}

void Receiver::setFramesPerSecond(QObject *sender, int rx, int value) {

	Q_UNUSED(sender)

	if (m_receiverID == rx)
		m_displayTime = (int)(1000000.0/value);
}
