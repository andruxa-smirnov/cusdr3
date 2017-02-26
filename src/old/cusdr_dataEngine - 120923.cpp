/**
* @file  cusdr_dataEngine.cpp
* @brief cuSDR data engine class
* @author Hermann von Hasseln, DL3HVH
* @version 0.1
* @date 2011-02-02
*/

/*
 *   
 *   Copyright 2010 Hermann von Hasseln, DL3HVH
 *
 *	 using original C code by John Melton, G0ORX/N6LYT and Dave McQuate, WA8YWQ
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
//#define TESTING

#define LOG_DATA_ENGINE
#define LOG_DATA_PROCESSOR
#define LOG_FUTUREWATCHER

#include "cusdr_dataEngine.h"

//#include <winsock2.h>


/*!
	\class DataEngine
	\brief The DataEngine class implements the main SDR functionality.
*/
/*!
	\brief Implements interfaces to the HPSDR hardware and various Server and DSP functionality.
	- set up HW interfaces to Metis or other resp.
	- initializes Metis.
	- set up parameters for HPSDR hardware.
	- implements the data receiver thread.
	- implements the data processor thread.
	- implements the wide band data processor thread.
	- implements the audio receiver thread.
	- implements the audio processor thread.
	- implements the interface to the Chirp WSPR decoding functionality.
*/
DataEngine::DataEngine(QObject *parent)
	: QObject(parent)
	, settings(Settings::instance())
	, m_audioEngine(new AudioEngine())
	, m_serverMode(settings->getCurrentServerMode())
	, m_hwInterface(settings->getHWInterface())
	, m_dataEngineState(QSDR::DataEngineDown)
	, m_meterType(SIGNAL_STRENGTH)
	//, m_meterType(AVG_SIGNAL_STRENGTH)
	, m_restart(false)
	, m_networkDeviceRunning(false)
	, m_soundFileLoaded(false)
	, m_audioProcessorRunning(false)
	, m_chirpInititalized(false)
	, m_netIOThreadRunning(false)
	, m_dataRcvrThreadRunning(false)
	, m_chirpDataProcThreadRunning(false)
	, m_dataProcThreadRunning(false)
	, m_audioRcvrThreadRunning(false)
	, m_audioProcThreadRunning(false)
	, m_frequencyChange(false)
	, m_wbSpectrumAveraging(settings->getSpectrumAveraging())
	, m_hamBandChanged(true)
	//, m_alexStatesChanged(true)
	, m_chirpThreadStopped(true)
	, m_chirpGateBit(true)
	, m_chirpBit(false)
	, m_chirpStart(false)
	, m_chirpStartSample(0)
	, m_hpsdrDevices(0)
	, m_metisFW(0)
	, m_hermesFW(0)
	, m_mercuryFW(0)
	, m_configure(10)
	, m_timeout(5000)
	, m_remainingTime(0)
	, m_idx(IO_HEADER_SIZE)
	, m_RxFrequencyChange(0)
	, m_forwardPower(0)
	, m_rxSamples(0)
	, m_chirpSamples(0)
	, m_spectrumSize(settings->getSpectrumSize())
	, m_sendState(0)
	, m_sMeterCalibrationOffset(0.0f)//(35.0f)
	//, m_specAveragingCnt(settings->getSpectrumAveragingCnt())
{
	qRegisterMetaType<QAbstractSocket::SocketError>();

	clientConnected = false;

//	if (m_specAveragingCnt > 0)
//		m_scale = 1.0f / m_specAveragingCnt;
//	else
//		m_scale = 1.0f;


	//currentRx = 0;
	m_hpsdrIO = 0;
	m_dataReceiver = 0;
	m_dataProcessor = 0;
	m_wbDataProcessor = 0;
	m_audioReceiver = 0;
	m_audioProcessor = 0;
	m_chirpProcessor = 0;
	m_wbAverager = 0;

	settings->setMercuryVersion(0);
	settings->setPenelopeVersion(0);
	settings->setMetisVersion(0);
	settings->setHermesVersion(0);
	
	setupConnections();
}

DataEngine::~DataEngine() {
}

void DataEngine::setupConnections() {

	CHECKED_CONNECT(
		settings,
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

//	CHECKED_CONNECT(
//		settings,
//		SIGNAL(spectrumSizeChanged(QObject *, int)),
//		this,
//		SLOT(setSpectrumSize(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(rxListChanged(QList<Receiver *>)),
		this,
		SLOT(rxListChanged(QList<Receiver *>)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(numberOfRXChanged(QObject *, int)), 
		this, 
		SLOT(setNumberOfRx(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(currentReceiverChanged(QObject *,int)),
		this, 
		SLOT(setCurrentReceiver(QObject *, int)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(hamBandChanged(QObject *, int, bool, HamBand)),
		this,
		SLOT(setHamBand(QObject *, int, bool, HamBand)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(sampleRateChanged(QObject *, int)), 
		this, 
		SLOT(setSampleRate(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(mercuryAttenuatorChanged(QObject *, HamBand, int)),
		this, 
		SLOT(setMercuryAttenuator(QObject *, HamBand, int)));

//	CHECKED_CONNECT(
//		settings,
//		SIGNAL(mercuryAttenuatorsChanged(QObject *, QList<int>)),
//		this,
//		SLOT(setMercuryAttenuators(QObject *, QList<int>)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(ditherChanged(QObject *, int)), 
		this, 
		SLOT(setDither(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(randomChanged(QObject *, int)), 
		this, 
		SLOT(setRandom(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(src10MhzChanged(QObject *, int)), 
		this, 
		SLOT(set10MhzSource(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(src122_88MhzChanged(QObject *, int)), 
		this, 
		SLOT(set122_88MhzSource(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(micSourceChanged(QObject *, int)), 
		this, 
		SLOT(setMicSource(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(classChanged(QObject *, int)), 
		this, 
		SLOT(setMercuryClass(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(timingChanged(QObject *, int)), 
		this, 
		SLOT(setMercuryTiming(QObject *, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(clientDisconnectedEvent(int)), 
		this, 
		SLOT(setClientDisconnected(int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(clientNoConnectedChanged(QObject*, int)), 
		this, 
		SLOT(setClientConnected(QObject*, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(rxConnectedStatusChanged(QObject*, int, bool)), 
		this, 
		SLOT(setRxConnectedStatus(QObject*, int, bool)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(audioRxChanged(QObject*, int)), 
		this, 
		SLOT(setAudioReceiver(QObject*, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(framesPerSecondChanged(QObject*, int, int)),
		this, 
		SLOT(setFramesPerSecond(QObject*, int, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(searchMetisSignal()), 
		this, 
		SLOT(searchHpsdrNetworkDevices()));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(spectrumAveragingChanged(bool)), 
		this, 
		SLOT(setWbSpectrumAveraging(bool)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(networkDeviceNumberChanged(int)), 
		this, 
		SLOT(setHPSDRDeviceNumber(int)));

//	CHECKED_CONNECT(
//		settings,
//		SIGNAL(alexConfigurationChanged(const QList<TAlexConfiguration> &)),
//		this,
//		SLOT(setAlexConfiguration(const QList<TAlexConfiguration> &)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(alexConfigurationChanged(quint16)),
		this,
		SLOT(setAlexConfiguration(quint16)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(alexStateChanged(HamBand, const QList<int> &)),
		this,
		SLOT(setAlexStates(HamBand, const QList<int> &)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(pennyOCEnabledChanged(bool)),
		this,
		SLOT(setPennyOCEnabled(bool)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(rxJ6PinsChanged(const QList<int> &)),
		this,
		SLOT(setRxJ6Pins(const QList<int> &)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(txJ6PinsChanged(const QList<int> &)),
		this,
		SLOT(setTxJ6Pins(const QList<int> &)));

	CHECKED_CONNECT(
		m_audioEngine,
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	CHECKED_CONNECT(
		m_audioEngine,
		SIGNAL(formatChanged(QObject *, const QAudioFormat )), 
		settings, 
		SLOT(setAudioFormat(QObject *, const QAudioFormat )));

	CHECKED_CONNECT(
		m_audioEngine,
		SIGNAL(formatChanged(QObject *, const QAudioFormat )), 
		this, 
		SLOT(setAudioFileFormat(QObject *, const QAudioFormat )));

	CHECKED_CONNECT(
		m_audioEngine, 
		SIGNAL(playPositionChanged(QObject *, qint64)),
		settings,
		SLOT(setAudioPosition(QObject *, qint64)));

	CHECKED_CONNECT(
		m_audioEngine, 
		SIGNAL(playPositionChanged(QObject *, qint64)),
		this,
		SLOT(setAudioFilePosition(QObject *, qint64)));

	CHECKED_CONNECT(
		m_audioEngine, 
		SIGNAL(bufferChanged(QObject *, qint64, qint64, const QByteArray)),
		settings,
		SLOT(setAudioBuffer(QObject *, qint64, qint64, const QByteArray)));

	CHECKED_CONNECT(
		m_audioEngine, 
		SIGNAL(bufferChanged(QObject *, qint64, qint64, const QByteArray)),
		this,
		SLOT(setAudioFileBuffer(QObject *, qint64, qint64, const QByteArray)));

	CHECKED_CONNECT(
		m_audioEngine, 
		SIGNAL(audiofileBufferChanged(const QList<qreal>)),
		this,
		SLOT(setAudioFileBuffer(const QList<qreal>)));

	/*CHECKED_CONNECT(
		m_metisDialog,
		SIGNAL(),
		this,
		SLOT());*/
}
 
//********************************************************
// start/stop data engine
bool DataEngine::startDataEngineWithoutConnection() {

	sendMessage("no HPSDR-HW interface.");

	if (io.inputBuffer.length() > 0) {

		initReceivers();
		if (!m_dataReceiver)	createDataReceiver();
		if (!m_dataProcessor)	createDataProcessor();
		
		// data receiver thread
		if (!startDataReceiver(QThread::NormalPriority)) {

			DATA_ENGINE_DEBUG << "data receiver thread could not be started.";
			return false;
		}

		switch (m_serverMode) {

			case QSDR::ExternalDSP:
			//case QSDR::InternalDSP:
			case QSDR::DttSP:
			case QSDR::QtDSP:
			case QSDR::ChirpWSPR:
			case QSDR::NoServerMode:
			case QSDR::DemoMode:
				return false;

			case QSDR::ChirpWSPRFile:			

				if (!m_chirpInititalized) createChirpDataProcessor();				

				m_chirpProcessor->generateLocalChirp();
				
				if (!startChirpDataProcessor(QThread::NormalPriority)) {
					
					DATA_ENGINE_DEBUG << "data processor thread could not be started.";
					return false;
				}
		
				m_chirpDspEngine = new QDSPEngine(this, 0, 2*BUFFER_SIZE);
				//m_fft = new QFFT(this, 2*BUFFER_SIZE);

				cpxIn.resize(2*BUFFER_SIZE);
				cpxOut.resize(2*BUFFER_SIZE);

				RX.at(0)->setConnectedStatus(true);
				settings->setRxList(RX);

				m_rxSamples = 0;
				m_chirpSamples = 0;

				break;
		}

		// IQ data processing thread
		if (!startDataProcessor(QThread::NormalPriority)) {

			DATA_ENGINE_DEBUG << "data processor thread could not be started.";
			return false;
		}

		settings->setSystemState(
							this, 
							QSDR::NoError, 
							m_hwInterface, 
							m_serverMode, 
							QSDR::DataEngineUp);
		return true;
	}
	else {

		sendMessage("no data available - data file loaded?.");
		return false;
	}
}

bool DataEngine::findHPSDRDevices() {

	if (!m_hpsdrIO) createHpsdrIO();

	// HPSDR network IO thread
	if (!startHpsdrIO(QThread::NormalPriority)) {

		DATA_ENGINE_DEBUG << "HPSDR network IO thread could not be started.";
		return false;
	}

	io.networkIOMutex.lock();
	io.devicefound.wait(&io.networkIOMutex);

	m_hpsdrDevices = settings->getHpsdrNetworkDevices();
	if (m_hpsdrDevices == 0) {

		io.networkIOMutex.unlock();
		stopHpsdrIO();
		sendMessage("no device found. HPSDR hardware powered? Network connection established?");

		settings->setSystemState(
						this, 
						QSDR::HwIOError, 
						m_hwInterface, 
						m_serverMode,
						QSDR::DataEngineDown);
	}
	else {

		if (m_hpsdrDevices > 1)
			settings->showNetworkIODialog();

		QList<TNetworkDevicecard> metisList = settings->getMetisCardsList();
		m_message = "found %1 network device(s).";
		sendMessage(m_message.arg(metisList.count()));
				
		for (int i = 0; i < metisList.count(); i++) {					
			m_message = "Device %1 @ %2 [%3].";
			sendMessage(m_message.arg(i).arg(metisList.at(i).ip_address.toString()).arg((char *) &metisList.at(i).mac_address));
		}

		io.hpsdrDeviceIPAddress = settings->getCurrentMetisCard().ip_address;
		io.hpsdrDeviceName = settings->getCurrentMetisCard().boardName;
		DATA_ENGINE_DEBUG << "using HPSDR network device at " << qPrintable(io.hpsdrDeviceIPAddress.toString());

		m_message = "using device @ %2.";
		sendMessage(m_message.arg(io.hpsdrDeviceIPAddress.toString()));
		Sleep(100);

		// stop the discovery thread
		io.networkIOMutex.unlock();
		stopHpsdrIO();

		if (getFirmwareVersions()) return true;
	}

	return false;
}

bool DataEngine::getFirmwareVersions() {

	m_fwCount = 0;

	// as it says..
	initReceivers();

	if (!m_dataReceiver) createDataReceiver();
		
	if (!m_dataProcessor) createDataProcessor();

	switch (m_serverMode) {

		case QSDR::DttSP:
			
			for (int i = 0; i < settings->getNumberOfReceivers(); i++) {

				if (! RX.at(i)->initDttSPInterface())
					return false;

				RX.at(i)->setConnectedStatus(true);
				setDttSPMainVolume(this, i, 0.0f);
				setFrequency(this, true, i, settings->getFrequencies().at(i));
			}
				
			// turn time stamping off
			setTimeStamp(this, false);

			settings->setRxList(RX);
			connectDTTSP();

			break;

		case QSDR::QtDSP:

			for (int i = 0; i < settings->getNumberOfReceivers(); i++) {

				if (! RX.at(i)->initQtDSPInterface())
					return false;

				RX.at(i)->setConnectedStatus(true);
				setFrequency(this, true, i, settings->getFrequencies().at(i));
			}

			// turn time stamping off
			setTimeStamp(this, false);

			settings->setRxList(RX);
			connectQTDSP();

			break;

		default:

			sendMessage("no valid server mode.");
						
			settings->setSystemState(
							this, 
							QSDR::ServerModeError, 
							m_hwInterface, 
							m_serverMode, 
							QSDR::DataEngineDown);

			return false;
	}	// end switch (m_serverMode)

	// data receiver thread
	if (!startDataReceiver(QThread::HighPriority)) {//  ::NormalPriority)) {

		DATA_ENGINE_DEBUG << "data receiver thread could not be started.";
		return false;
	}

	// IQ data processing thread
	if (!startDataProcessor(QThread::NormalPriority)) {

		DATA_ENGINE_DEBUG << "data processor thread could not be started.";
		return false;
	}

	setSampleRate(this, settings->getSampleRate());

	// pre-conditioning
	for (int i = 0; i < io.receivers; i++)
		sendInitFramesToNetworkDevice(i);
				
	if ((m_serverMode == QSDR::DttSP || m_serverMode == QSDR::QtDSP))
		networkDeviceStartStop(0x01); // 0x01 for starting Metis without wide band data
		
	m_networkDeviceRunning = true;

	settings->setSystemState(
					this, 
					QSDR::NoError, 
					m_hwInterface, 
					m_serverMode, 
					QSDR::DataEngineUp);

	// just give it a little time to get the firmware versions
	Sleep(100);

	m_metisFW = settings->getMetisVersion();
	m_mercuryFW = settings->getMercuryVersion();
	m_hermesFW = settings->getHermesVersion();

	if (m_metisFW != 0 &&  io.hpsdrDeviceName == "Hermes") {

		stop();

		QString msg = "Metis selected, but Hermes found!";
		settings->showWarningDialog(msg);
		return false;
	}

	if (m_hermesFW != 0 && io.hpsdrDeviceName == "Metis") {

		stop();

		QString msg = "Hermes selected, but Metis found!";
		settings->showWarningDialog(msg);
		return false;
	}

	if (m_mercuryFW < 33 && settings->getNumberOfReceivers() > 4 && io.hpsdrDeviceName == "Metis") {

		stop();

		QString msg = "Mercury FW < V3.3 has only 4 receivers!";
		settings->showWarningDialog(msg);
		return false;
	}

	if (m_hermesFW < 18 && settings->getNumberOfReceivers() > 2 && io.hpsdrDeviceName == "Hermes") {

		stop();

		QString msg = "Hermes FW < V1.8 has only 2 receivers!";
		settings->showWarningDialog(msg);
		return false;
	}

	// if we have 4096 * 16 bit = 8 * 1024 raw consecutive ADC samples, m_wbBuffers = 8
	// we have 16384 * 16 bit = 32 * 1024 raw consecutive ADC samples, m_wbBuffers = 32
	int wbBuffers = 0;
	if (m_mercuryFW > 32 || m_hermesFW > 16)
		wbBuffers = BIGWIDEBANDSIZE / 512;
	else
		wbBuffers = SMALLWIDEBANDSIZE / 512;

	settings->setWidebandBuffers(this, wbBuffers);

	return true;
}

bool DataEngine::start() {

	m_fwCount = 0;
	m_sendState = 0;

	// as it says..
	initReceivers();

	if (!m_dataReceiver) createDataReceiver();
		
	if (!m_dataProcessor) createDataProcessor();
		
	//if ((m_serverMode == QSDR::DttSP || m_serverMode == QSDR::QtDSP) && !m_wbDataProcessor && settings->getWideBandData())
	if ((m_serverMode == QSDR::DttSP || m_serverMode == QSDR::QtDSP) && !m_wbDataProcessor)
		createWideBandDataProcessor();
		
	if (!m_audioProcessor) 
		createAudioProcessor();

	if ((m_serverMode == QSDR::ChirpWSPR) && !m_chirpProcessor)
		createChirpDataProcessor();
		
		switch (m_serverMode) {

			case QSDR::ExternalDSP:
		
				/*
				//CHECKED_CONNECT(
				//	settings,
				//	SIGNAL(frequencyChanged(QObject*, bool, int, long)),
				//	this,
				//	SLOT(setFrequency(QObject*, bool, int, long)));

				//if (!m_audioProcessorRunning) {

				//	//if (!m_audioProcessor)	createAudioProcessor();
				//	if (!m_audioReceiver)	createAudioReceiver();

				//	m_audioProcThread->start();
				//	if (m_audioProcThread->isRunning()) {
				//				
				//		m_audioProcThreadRunning = true;
				//		DATA_ENGINE_DEBUG << "Audio processor process started.";
				//	}
				//	else {

				//		m_audioProcThreadRunning = false;
				//		settings->setSystemState(
				//						this, 
				//						QSDR::AudioThreadError, 
				//						m_hwInterface, 
				//						m_serverMode,
				//						QSDR::DataEngineDown);
				//		return false;
				//	}
				//			
				//	io.audio_rx = 0;
				//	io.clientList.append(0);

				//	m_audioProcessorRunning = true;
				//	settings->setSystemState(
				//						this, 
				//						QSDR::NoError, 
				//						m_hwInterface, 
				//						m_serverMode,
				//						QSDR::DataEngineUp);
				//}
				 */
				return false;

			case QSDR::DttSP:
			
				for (int i = 0; i < settings->getNumberOfReceivers(); i++) {

					if (! RX.at(i)->initDttSPInterface())
						return false;

					setDttSPMainVolume(this, i, RX.at(i)->getAudioVolume());
					setFrequency(this, true, i, settings->getFrequencies().at(i));
					RX.at(i)->setConnectedStatus(true);
				}
				
				// turn time stamping off
				setTimeStamp(this, false);

				settings->setRxList(RX);
				connectDTTSP();

				break;

			case QSDR::QtDSP:

				for (int i = 0; i < settings->getNumberOfReceivers(); i++) {

					if (! RX.at(i)->initQtDSPInterface())
						return false;

					setQtDSPMainVolume(this, i, RX.at(i)->getAudioVolume());
					setFrequency(this, true, i, settings->getFrequencies().at(i));
					RX.at(i)->setConnectedStatus(true);
				}

				// turn time stamping off
				setTimeStamp(this, false);

				settings->setRxList(RX);
				connectQTDSP();

				break;

			case QSDR::ChirpWSPR:
			//case QSDR::ChirpWSPRFile:

				if (! RX.at(0)->initDttSPInterface())
					return false;

				// turn time stamping on
				setTimeStamp(this, true);

				if (!startChirpDataProcessor(QThread::NormalPriority)) {
					
					DATA_ENGINE_DEBUG << "data processor thread could not be started.";
					return false;
				}

				RX.at(0)->setConnectedStatus(true);
				settings->setRxList(RX);

				setFrequency(this, true, 0, settings->getFrequencies().at(0));

				CHECKED_CONNECT(
							settings,
							SIGNAL(frequencyChanged(QObject *, bool, int, long)),
							this,
							SLOT(setFrequency(QObject *, bool, int, long)));

				break;

			default:

				sendMessage("no valid server mode.");
						
				settings->setSystemState(
								this, 
								QSDR::ServerModeError, 
								m_hwInterface, 
								m_serverMode, 
								QSDR::DataEngineDown);

				return false;
		}	// end switch (m_serverMode)

		// Wide band data processing thread
		//if (settings->getWideBandData()) {

			if (m_serverMode != QSDR::ChirpWSPR && !startWideBandDataProcessor(QThread::NormalPriority)) {

				DATA_ENGINE_DEBUG << "wide band data processor thread could not be started.";
				return false;
			}
		//}

		// data receiver thread
		if (!startDataReceiver(QThread::HighPriority)) {//  ::NormalPriority)) {

			DATA_ENGINE_DEBUG << "data receiver thread could not be started.";
			return false;
		}

		// IQ data processing thread
		if (!startDataProcessor(QThread::NormalPriority)) {

			DATA_ENGINE_DEBUG << "data processor thread could not be started.";
			return false;
		}

		// audio processing thread
		if (!startAudioProcessor(QThread::NormalPriority, m_serverMode)) {

			DATA_ENGINE_DEBUG << "audio processor thread could not be started.";
			return false;
		}

		// start Sync,ADC and S-Meter timers
		m_SyncChangedTime.start();
		m_ADCChangedTime.start();
		m_smeterTime.start();

		// just give them a little time..
		Sleep(100);

		setSampleRate(this, settings->getSampleRate());

		// pre-conditioning
		//for (int i = 0; i < io.receivers; i++)
		//	sendInitFramesToNetworkDevice(i);
		sendInitFramesToNetworkDevice(0);
				
		if ((m_serverMode == QSDR::DttSP || m_serverMode == QSDR::QtDSP) && settings->getWideBandData())
			networkDeviceStartStop(0x03); // 0x03 for starting Metis with wide band data
		else
			networkDeviceStartStop(0x01); // 0x01 for starting Metis without wide band data
		
		m_networkDeviceRunning = true;

		// start the "frames-per-second" timer
		for (int i = 0; i < settings->getNumberOfReceivers(); i++)
			RX.at(i)->highResTimer->start();

		settings->setSystemState(
			this, 
			QSDR::NoError, 
			m_hwInterface, 
			m_serverMode, 
			QSDR::DataEngineUp);

		return true;
	//} // if (m_hpsdrDevices == 0)
}

void DataEngine::stop() {

	if (m_dataEngineState == QSDR::DataEngineUp) {
		
		switch (m_hwInterface) {

			case QSDR::Metis:
			case QSDR::Hermes:
				
				// turn time stamping off
				setTimeStamp(this, false);
				Sleep(10);

				networkDeviceStartStop(0);
				m_networkDeviceRunning = false;

				sendMessage("Metis stopped.");

				stopAudioProcessor();
				stopDataReceiver();
				stopDataProcessor();
				stopChirpDataProcessor();
				if (m_wbDataProcessor)
					stopWideBandDataProcessor();
				
				//Sleep(100);
				settings->clearMetisCardList();
				//m_metisDialog->clear();
				//m_networkIO->clear();
				DATA_ENGINE_DEBUG << "device cards list cleared.";
				break;

			case QSDR::NoInterfaceMode:

				stopDataReceiver();
				
				DATA_ENGINE_DEBUG << "data queue count: " << io.data_queue.count();
				DATA_ENGINE_DEBUG << "chirp queue count: " << io.chirp_queue.count();

				stopDataProcessor();
				stopChirpDataProcessor();
		}

		while (!io.au_queue.isEmpty())
			io.au_queue.dequeue();

		//QCoreApplication::processEvents();

		// save receiver settings and clear receiver list
		foreach (Receiver *rx, RX) {

			rx->setConnectedStatus(false);
			//rx->setProperty("socketState", "RECEIVER_DETACHED");

			if (m_serverMode == QSDR::DttSP || m_serverMode == QSDR::ChirpWSPR) { // clear DttSP

				disconnectDTTSP();
				rx->dttsp->setDttSPStatus(false);
				rx->dttsp->dttspReleaseUpdate();
				rx->dttsp->dttspDestroySDR();
				DATA_ENGINE_DEBUG << "DttSP deleted.";
			}
			else if (m_serverMode == QSDR::QtDSP) { // clear QtDSP

				disconnectQTDSP();
				rx->qtdsp->setQtDSPStatus(false);
				rx->deleteQtDSP();
				//rx->qtdsp->deleteLater();
				DATA_ENGINE_DEBUG << "QtDSP deleted.";
			}
		}

		qDeleteAll(RX.begin(), RX.end());
		RX.clear();
		settings->setRxList(RX);
		DATA_ENGINE_DEBUG << "receiver list cleared.";

		settings->setSystemState(
			this, 
			QSDR::NoError, 
			m_hwInterface, 
			m_serverMode, 
			QSDR::DataEngineDown);
	}

	m_rxSamples = 0;
	m_chirpSamples = 0;
	m_restart = true;
	m_found = 0;
	m_hpsdrDevices = 0;

	settings->setMercuryVersion(0);
	settings->setPenelopeVersion(0);
	settings->setMetisVersion(0);
	settings->setHermesVersion(0);

	//settings->setPeakHold(false);
	//settings->resetWidebandSpectrumBuffer();

	/*disconnect(
		SIGNAL(iqDataReady(int)),
		this,
		SLOT(dttSPDspProcessing(int)));*/

	disconnect(
		settings, 
		SIGNAL(frequencyChanged(QObject*, bool, int, long)), 
		this, 
		SLOT(setFrequency(QObject*, bool, int, long)));

	DATA_ENGINE_DEBUG << "shut down done.";
}

bool DataEngine::initDataEngine() {

#ifdef TESTING
	return start();
#endif

	if (m_hwInterface == QSDR::NoInterfaceMode) {
		
		return startDataEngineWithoutConnection();
	}
	else {
			
		if (findHPSDRDevices()) {
		
			if (m_mercuryFW > 0 || m_hermesFW > 0) {

				stop();

				DATA_ENGINE_DEBUG << "got firmware versions:";
				DATA_ENGINE_DEBUG << "	Metis firmware:  " << m_metisFW;
				DATA_ENGINE_DEBUG << "	Mercury firmware:  " << m_mercuryFW;
				DATA_ENGINE_DEBUG << "	Hermes firmware: " << m_hermesFW;
				DATA_ENGINE_DEBUG << "stopping and restarting data engine.";

				return start();
			}
			else
				DATA_ENGINE_DEBUG << "did not get firmware versions!";
		}
	}
	return false;
}

void DataEngine::initReceivers() {

	int noOfReceivers = settings->getNumberOfReceivers();

	for (int i = 0; i < noOfReceivers; i++) {
	
		Receiver *r = new Receiver(this, i);
		r->setID(i);
		r->setConnectedStatus(false);
		//r->setProperty("serverMode", m_serverMode);
		//r->setProperty("socketState", "RECEIVER_DETACHED");

		CHECKED_CONNECT(r, SIGNAL(messageEvent(QString)), this, SLOT(sendReceiverMessage(QString)));

		RX.append(r);
    }

	settings->setRxList(RX);

	m_txFrame = 0;
	
	io.currentReceiver = 0;

	io.receivers = noOfReceivers;
	for (int i = 0; i < io.receivers; ++i) m_rx.append(i);

	io.timing = 0;
	m_configure = io.receivers + 1;

	// init cc Rc parameters
	io.ccRx.mercuryFirmwareVersion = 0;
	io.ccRx.penelopeFirmwareVersion = 0;
	io.ccRx.networkDeviceFirmwareVersion = 0;

	io.ccRx.ptt    = false;
	io.ccRx.dash   = false;
	io.ccRx.dot    = false;
	io.ccRx.lt2208 = false;
	io.ccRx.ain1   = 0;
	io.ccRx.ain2   = 0;
	io.ccRx.ain3   = 0;
	io.ccRx.ain4   = 0;
	io.ccRx.ain5   = 0;
	io.ccRx.ain6   = 0;
	io.ccRx.hermesI01 = false;
	io.ccRx.hermesI02 = false;
	io.ccRx.hermesI03 = false;
	io.ccRx.hermesI04 = false;
	io.ccRx.mercury1_LT2208 = false;
	io.ccRx.mercury2_LT2208 = false;
	io.ccRx.mercury3_LT2208 = false;
	io.ccRx.mercury4_LT2208 = false;

	// init cc Tx parameters
	io.ccTx.currentBand = RX.at(0)->getHamBand();
	io.ccTx.mercuryAttenuators = RX.at(0)->getMercuryAttenuators();
	io.ccTx.mercuryAttenuator = RX.at(0)->getMercuryAttenuators().at(io.ccTx.currentBand);
	io.ccTx.dither = settings->getMercuryDither();
	io.ccTx.random = settings->getMercuryRandom();
	io.ccTx.duplex = 1;
	io.ccTx.mox = false;
	io.ccTx.ptt = false;
	io.ccTx.alexStates = settings->getAlexStates();
	io.ccTx.vnaMode = false;
	io.ccTx.alexConfig = settings->getAlexConfig();
	io.ccTx.timeStamp = 0;
	io.ccTx.commonMercuryFrequencies = 0;
	io.ccTx.pennyOCenabled = settings->getPennyOCEnabled();
	io.ccTx.rxJ6pinList = settings->getRxJ6Pins();
	io.ccTx.txJ6pinList = settings->getTxJ6Pins();

	setAlexConfiguration(io.ccTx.alexConfig);

	io.rxClass = settings->getRxClass();
	io.mic_gain = 0.26F;
	io.rx_freq_change = -1;
	io.tx_freq_change = -1;
	io.clients = 0;
	io.sendIQ_toggle = true;
	io.rcveIQ_toggle = false;
	io.alexForwardVolts = 0.0;
	io.alexReverseVolts = 0.0;
	io.alexForwardPower = 0.0;
	io.alexReversePower = 0.0;
	io.penelopeForwardVolts = 0.0;
	io.penelopeForwardPower = 0.0;
	io.ain3Volts = 0.0;
	io.ain4Volts = 0.0;
	io.supplyVolts = 0.0f;


	//*****************************
	// C&C bytes
	for (int i = 0; i < 5; i++) {

		io.control_in[i]  = 0x00;
		io.control_out[i] = 0x00;
	}

	// C0
	// 0 0 0 0 0 0 0 0
	//               |
	//               +------------ MOX (1 = active, 0 = inactive)

	io.control_out[0] |= MOX_DISABLED;

	// set C1
	//
	// 0 0 0 0 0 0 0 0
	// | | | | | | | |
	// | | | | | | + +------------ Speed (00 = 48kHz, 01 = 96kHz, 10 = 192kHz)
	// | | | | + +---------------- 10MHz Ref. (00 = Atlas/Excalibur, 01 = Penelope, 10 = Mercury)*
	// | | | +-------------------- 122.88MHz source (0 = Penelope, 1 = Mercury)*
	// | + +---------------------- Config (00 = nil, 01 = Penelope, 10 = Mercury, 11 = both)*
	// +-------------------------- Mic source (0 = Janus, 1 = Penelope)*

	// Bits 1,0
	setSampleRate(this, settings->getSampleRate());

	// Bits 7,..,2
	setHPSDRConfig();
	
	io.control_out[1] &= 0x03; // 0 0 0 0 0 0 1 1
	io.control_out[1] |= io.ccTx.clockByte;

	// set C2
	//
	// 0 0 0 0 0 0 0 0
	// |           | |
	// |           | +------------ Mode (1 = Class E, 0 = All other modes)
	// +---------- +-------------- Open Collector Outputs on Penelope or Hermes (bit 6…..bit 0)

	io.control_out[2] = io.control_out[2] & 0xFE; // 1 1 1 1 1 1 1 0
	io.control_out[2] = io.control_out[2] | io.rxClass;

	// set C3
	//
	// 0 0 0 0 0 0 0 0
	// | | | | | | | |
	// | | | | | | + +------------ Alex Attenuator (00 = 0dB, 01 = 10dB, 10 = 20dB, 11 = 30dB)
	// | | | | | +---------------- Preamp On/Off (0 = Off, 1 = On)
	// | | | | +------------------ LT2208 Dither (0 = Off, 1 = On)
	// | | | + ------------------- LT2208 Random (0= Off, 1 = On)
	// | + + --------------------- Alex Rx Antenna (00 = none, 01 = Rx1, 10 = Rx2, 11 = XV)
	// + ------------------------- Alex Rx out (0 = off, 1 = on). Set if Alex Rx Antenna > 00.

	io.control_out[3] = io.control_out[3] & 0xFB; // 1 1 1 1 1 0 1 1
	io.control_out[3] = io.control_out[3] | (io.ccTx.mercuryAttenuator << 2);

	io.control_out[3] = io.control_out[3] & 0xF7; // 1 1 1 1 0 1 1 1
	io.control_out[3] = io.control_out[3] | (io.ccTx.dither << 3);

	io.control_out[3] = io.control_out[3] & 0xEF; // 1 1 1 0 1 1 1 1
	io.control_out[3] = io.control_out[3] | (io.ccTx.random << 4);

	// set C4
	//
	// 0 0 0 0 0 0 0 0
	// | | | | | | | |
	// | | | | | | + + ----------- Alex Tx relay (00 = Tx1, 01= Tx2, 10 = Tx3)
	// | | | | | + --------------- Duplex (0 = off, 1 = on)
	// | | + + +------------------ Number of Receivers (000 = 1, 111 = 8)
	// | +------------------------ Time stamp – 1PPS on LSB of Mic data (0 = off, 1 = on)
	// +-------------------------- Common Mercury Frequency (0 = independent frequencies to Mercury
	//			                   Boards, 1 = same frequency to all Mercury boards)

	//io.control_out[4] &= 0xC7; // 1 1 0 0 0 1 1 1
	io.control_out[4] = (io.ccTx.duplex << 2) | ((io.receivers - 1) << 3);
}

void DataEngine::setHPSDRConfig() {

	io.ccTx.clockByte = 0x0;

	// C1
	// 0 0 0 0 0 0 0 0
	// | | | | | | | |
	// | | | | | | + +------------ Speed (00 = 48kHz, 01 = 96kHz, 10 = 192kHz)
	// | | | | + +---------------- 10MHz Ref. (00 = Atlas/Excalibur, 01 = Penelope, 10 = Mercury)*
	// | | | +-------------------- 122.88MHz source (0 = Penelope, 1 = Mercury)*
	// | + +---------------------- Config (00 = nil, 01 = Penelope, 10 = Mercury, 11 = both)*
	// +-------------------------- Mic source (0 = Janus, 1 = Penelope)*
	//
	// * Ignored by Hermes

	if (
		(settings->getPenelopePresence()   || settings->getPennyLanePresence()) &&
		((settings->get10MHzSource() == 0) || settings->getExcaliburPresence())
		)
	{

		io.ccTx.clockByte = MIC_SOURCE_PENELOPE | MERCURY_PRESENT | PENELOPE_PRESENT | MERCURY_122_88MHZ_SOURCE | ATLAS_10MHZ_SOURCE;
	}
	else if ((settings->getPenelopePresence() || settings->getPennyLanePresence()) && (settings->get10MHzSource() == 1)) {
		
		io.ccTx.clockByte = MIC_SOURCE_PENELOPE | MERCURY_PRESENT | PENELOPE_PRESENT | MERCURY_122_88MHZ_SOURCE | PENELOPE_10MHZ_SOURCE;
	}
	else if ((settings->getPenelopePresence() || settings->getPennyLanePresence()) && (settings->get10MHzSource() == 2)) {
		
		io.ccTx.clockByte = MIC_SOURCE_PENELOPE | MERCURY_PRESENT | PENELOPE_PRESENT | MERCURY_122_88MHZ_SOURCE | MERCURY_10MHZ_SOURCE;
	}
	else if ((settings->get10MHzSource() == 0) || settings->getExcaliburPresence()) {
		
		io.ccTx.clockByte = MERCURY_PRESENT | MERCURY_122_88MHZ_SOURCE | ATLAS_10MHZ_SOURCE;
	}
	else {
		
		io.ccTx.clockByte = MERCURY_PRESENT | MERCURY_122_88MHZ_SOURCE | MERCURY_10MHZ_SOURCE;
	}
}

void DataEngine::connectDTTSP() {

	/*CHECKED_CONNECT(
		dttspProcessing,
		SIGNAL(resultReadyAt(int)),
		this,
		SLOT(dttspReadyAt(int)));*/

	CHECKED_CONNECT(
		settings,
		SIGNAL(frequencyChanged(QObject *, bool, int, long)),
		this,
		SLOT(setFrequency(QObject *, bool, int, long)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(dspModeChanged(QObject *, int, DSPMode)), 
		this, 
		SLOT(setDttspDspMode(QObject *, int, DSPMode)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(agcModeChanged(QObject *, int, AGCMode)), 
		this, 
		SLOT(setDttspAgcMode(QObject *, int, AGCMode)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(agcGainChanged(QObject *, int, int)), 
		this, 
		SLOT(setDttspAgcGain(QObject *, int, int)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(filterFrequenciesChanged(QObject *, int, qreal, qreal)),
		this,
		SLOT(setDttspRXFilter(QObject *, int, qreal, qreal)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(mainVolumeChanged(QObject *, int, float)), 
		this, 
		SLOT(setDttSPMainVolume(QObject *, int, float)));
}

void DataEngine::connectQTDSP() {

	CHECKED_CONNECT(
		settings, 
		SIGNAL(filterFrequenciesChanged(QObject *, int, qreal, qreal)),
		this,
		SLOT(setRXFilter(QObject *, int, qreal, qreal)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(frequencyChanged(QObject *, bool, int, long)),
		this,
		SLOT(setFrequency(QObject *, bool, int, long)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(mainVolumeChanged(QObject *, int, float)), 
		this, 
		SLOT(setQtDSPMainVolume(QObject *, int, float)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(dspModeChanged(QObject *, int, DSPMode)),
		this,
		SLOT(setDspMode(QObject *, int, DSPMode)));

	CHECKED_CONNECT(
		settings,
		SIGNAL(agcModeChanged(QObject *, int, AGCMode)), 
		this, 
		SLOT(setAgcMode(QObject *, int, AGCMode)));

	CHECKED_CONNECT(
		settings, 
		SIGNAL(agcGainChanged(QObject *, int, int)), 
		this, 
		SLOT(setAgcGain(QObject *, int, int)));
}

void DataEngine::disconnectDTTSP() {

	disconnect(
		settings,
		SIGNAL(frequencyChanged(QObject *, bool, int, long)),
		this,
		SLOT(setFrequency(QObject *, bool, int, long)));

	disconnect(
		settings, 
		SIGNAL(dspModeChanged(QObject *, int, DSPMode)), 
		this, 
		SLOT(setDttspDspMode(QObject *, int, DSPMode)));

	disconnect(
		settings, 
		SIGNAL(agcModeChanged(QObject *, int, AGCMode)), 
		this, 
		SLOT(setDttspAgcMode(QObject *, int, AGCMode)));

	disconnect(
		settings, 
		SIGNAL(agcGainChanged(QObject *, int, int)), 
		this, 
		SLOT(setDttspAgcGain(QObject *, int, int)));

	disconnect(
		settings, 
		SIGNAL(filterFrequenciesChanged(QObject *, int, qreal, qreal)),
		this,
		SLOT(setDttspRXFilter(QObject *, int, qreal, qreal)));

	disconnect(
		settings, 
		SIGNAL(mainVolumeChanged(QObject *, int, float)), 
		this, 
		SLOT(setDttSPMainVolume(QObject *, int, float)));
}

void DataEngine::disconnectQTDSP() {

	disconnect(
		settings,
		SIGNAL(frequencyChanged(QObject *, bool, int, long)),
		this,
		SLOT(setFrequency(QObject *, bool, int, long)));

	disconnect(
		settings, 
		SIGNAL(mainVolumeChanged(QObject *, int, float)), 
		this, 
		SLOT(setQtDSPMainVolume(QObject *, int, float)));

	disconnect(
		settings, 
		SIGNAL(dspModeChanged(QObject *, int, DSPMode)),
		this,
		SLOT(setDspMode(QObject *, int, DSPMode)));

	disconnect(
		settings,
		SIGNAL(agcModeChanged(QObject *, int, AGCMode)), 
		this, 
		SLOT(setAgcMode(QObject *, int, AGCMode)));

	disconnect(
		settings, 
		SIGNAL(agcGainChanged(QObject *, int, int)), 
		this, 
		SLOT(setAgcGain(QObject *, int, int)));

	disconnect(
		settings, 
		SIGNAL(filterFrequenciesChanged(QObject *, int, qreal, qreal)),
		this,
		SLOT(setRXFilter(QObject *, int, qreal, qreal)));
}

void DataEngine::sendInitFramesToNetworkDevice(int rx) {

	QByteArray initDatagram;
	initDatagram.resize(1032);
	
	initDatagram[0] = (char)0xEF;
	initDatagram[1] = (char)0xFE;
	initDatagram[2] = (char)0x01;
	initDatagram[3] = (char)0x02;
	initDatagram[4] = (char)0x00;
	initDatagram[5] = (char)0x00;
	initDatagram[6] = (char)0x00;
	initDatagram[7] = (char)0x00;

	initDatagram[8] = SYNC;
    initDatagram[9] = SYNC;
    initDatagram[10] = SYNC;

	for (int i = 0; i < 5; i++) {
		
		initDatagram[i + 11]  = io.control_out[i];
	}

	for (int i = 16; i < 520; i++) {
		
		initDatagram[i]  = 0x00;
	}

	initDatagram[520] = SYNC;
    initDatagram[521] = SYNC;
    initDatagram[522] = SYNC;
	
	initDatagram[523] = io.control_out[0] | ((rx + 2) << 1);
	initDatagram[524] = RX[rx]->getFrequency() >> 24;
	initDatagram[525] = RX[rx]->getFrequency() >> 16;
	initDatagram[526] = RX[rx]->getFrequency() >> 8;
	initDatagram[527] = RX[rx]->getFrequency();

	/*io.output_buffer[3] = io.control_out[0] | ((rx + 2) << 1);
	io.output_buffer[4] = rxList[rx]->getFrequency() >> 24;
	io.output_buffer[5] = rxList[rx]->getFrequency() >> 16;
	io.output_buffer[6] = rxList[rx]->getFrequency() >> 8;
	io.output_buffer[7] = rxList[rx]->getFrequency();*/
	
	/*initDatagram[523] = io.control_out[0] | (2 << 1);
	initDatagram[524] = io.initialFrequency[0] >> 24;
	initDatagram[525] = io.initialFrequency[0] >> 16;
	initDatagram[526] = io.initialFrequency[0] >> 8;
	initDatagram[527] = io.initialFrequency[0];*/

	for (int i = 528; i < 1032; i++) {
		
		initDatagram[i]  = 0x00;
	}

	QUdpSocket socket;
	socket.bind(QHostAddress(settings->getHPSDRDeviceLocalAddr()), 
				settings->metisPort(),
				QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress);
				
	for (int i = 0; i < 1; i++) {
		
		if (socket.writeDatagram(initDatagram.data(), initDatagram.size(), io.hpsdrDeviceIPAddress, METIS_PORT) < 0)
			DATA_ENGINE_DEBUG << "error sending init data to device: " << qPrintable(socket.errorString());
		else {

			if (i == 0) DATA_ENGINE_DEBUG << "init frames sent to network device.";
		}
	}
	socket.close();
}
 
//********************************************************
// create, start/stop HPSDR device network IO

void DataEngine::createHpsdrIO() {

	m_hpsdrIO = new QHpsdrIO(&io);

	m_netIOThread = new QThreadEx();
	m_hpsdrIO->moveToThread(m_netIOThread);

	m_hpsdrIO->connect(
					m_netIOThread, 
					SIGNAL(started()), 
					SLOT(initHPSDRDevice()));
}

bool DataEngine::startHpsdrIO(QThread::Priority prio) {

	m_netIOThread->start(prio);

	if (m_netIOThread->isRunning()) {
					
		m_netIOThreadRunning = true;
		DATA_ENGINE_DEBUG << "HPSDR network IO thread started.";

		return true;
	}
	else {

		m_netIOThreadRunning = false;
		return false;
	}
}

void DataEngine::stopHpsdrIO() {

	if (m_netIOThread->isRunning()) {
		
		m_netIOThread->quit();
		m_netIOThread->wait(1000);
		delete m_netIOThread;
		delete m_hpsdrIO;
		m_hpsdrIO = 0;

		m_netIOThreadRunning = false;

		DATA_ENGINE_DEBUG << "HPSDR discovery thread stopped and deleted.";
	}
	else
		DATA_ENGINE_DEBUG << "network IO thread wasn't started.";
}

void DataEngine::networkDeviceStartStop(char value) {

	TNetworkDevicecard metis = settings->getCurrentMetisCard();
	QUdpSocket socket;
	socket.bind(QHostAddress(settings->getHPSDRDeviceLocalAddr()), 
				settings->metisPort(),
				QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress);
				
	m_commandDatagram.resize(64);
	m_commandDatagram[0] = (char)0xEF;
	m_commandDatagram[1] = (char)0xFE;
	m_commandDatagram[2] = (char)0x04;
	m_commandDatagram[3] = (char)value;

	for (int i = 4; i < 64; i++) m_commandDatagram[i] = 0x00;

	//if (socket.writeDatagram(m_commandDatagram, m_metisCards[0].ip_address, METIS_PORT) == 64) {
	if (socket.writeDatagram(m_commandDatagram, metis.ip_address, METIS_PORT) == 64) {
	
		//if (value == 1) {
		if (value != 0) {

			DATA_ENGINE_DEBUG << "sent start command to device at: "<< qPrintable(metis.ip_address.toString());
			m_networkDeviceRunning = true;
		}
		else {

			//DATA_ENGINE_DEBUG << "sent stop command to Metis at"<< m_metisCards[0].ip_address.toString();
			DATA_ENGINE_DEBUG << "sent stop command to device at: "<< qPrintable(metis.ip_address.toString());
			m_networkDeviceRunning = false;
		}
	}
	else
		DATA_ENGINE_DEBUG << "sending command to device failed.";
			
	socket.close();
}

//********************************************************
// create, start/stop data receiver

void DataEngine::createDataReceiver() {

	m_dataReceiver = new DataReceiver(&io);

	CHECKED_CONNECT(
		m_dataReceiver, 
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	switch (m_serverMode) {
		
		case QSDR::ExternalDSP:
			break;

		//case QSDR::InternalDSP:
		case QSDR::DttSP:
		case QSDR::QtDSP:
			m_message = "configured for %1 receiver(s) at %2 kHz sample rate.";	
			sendMessage(
				m_message.arg( 
					QString::number(settings->getNumberOfReceivers()), 
					QString::number(settings->getSampleRate()/1000)));
			break;

		case QSDR::ChirpWSPR:
		case QSDR::ChirpWSPRFile:
			break;
			
		case QSDR::NoServerMode:
		case QSDR::DemoMode:
			break;
	}

	m_dataRcvrThread = new QThreadEx();
	m_dataReceiver->moveToThread(m_dataRcvrThread);

	switch (m_hwInterface) {

		case QSDR::NoInterfaceMode:

			m_dataReceiver->connect(
						m_dataRcvrThread, 
						SIGNAL(started()), 
						SLOT(readData()));
			break;
			
		case QSDR::Metis:
		case QSDR::Hermes:

			m_dataReceiver->connect(
						m_dataRcvrThread, 
						SIGNAL(started()), 
						SLOT(initDataReceiverSocket()));
			break;
	}
}

bool DataEngine::startDataReceiver(QThread::Priority prio) {

	m_dataRcvrThread->start(prio);

	if (m_dataRcvrThread->isRunning()) {
					
		m_dataRcvrThreadRunning = true;
		DATA_ENGINE_DEBUG << "data receiver thread started.";

		return true;
	}
	else {

		m_dataRcvrThreadRunning = false;
		settings->setSystemState(
						this, 
						QSDR::DataProcessThreadError, 
						m_hwInterface, 
						m_serverMode,
						QSDR::DataEngineDown);
		return false;
	}
}

void DataEngine::stopDataReceiver() {

	if (m_dataRcvrThread->isRunning()) {
					
		m_dataReceiver->stop();

		/*if (m_serverMode == QSDR::InternalDSP || m_serverMode == QSDR::ChirpWSPR) {

			if (io.iq_queue.isEmpty())
				io.iq_queue.enqueue(m_datagram);
		}*/

		m_dataRcvrThread->quit();
		m_dataRcvrThread->wait(1000);
		delete m_dataRcvrThread;
		delete m_dataReceiver;
		m_dataReceiver = 0;

		if (m_serverMode == QSDR::ChirpWSPRFile) {

			while (!io.chirp_queue.isEmpty())
				io.chirp_queue.dequeue();
		}

		m_dataRcvrThreadRunning = false;

		DATA_ENGINE_DEBUG << "data receiver thread deleted.";
	}
	else
		DATA_ENGINE_DEBUG << "data receiver thread wasn't started.";
}
 
//********************************************************
// create, start/stop data processor

void DataEngine::createDataProcessor() {

	m_dataProcessor = new DataProcessor(this, m_serverMode);

	CHECKED_CONNECT(
		m_dataProcessor, 
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	switch (m_serverMode) {
		
		// The signal iqDataReady is generated by the function
		// processInputBuffer when a block of input data are
		// decoded.
		case QSDR::ExternalDSP:
			
			CHECKED_CONNECT_OPT(
				this,
				SIGNAL(iqDataReady(int)),
				m_dataProcessor,
				SLOT(externalDspProcessing(int)),
				Qt::DirectConnection);

			break;

		//case QSDR::InternalDSP:
		case QSDR::DttSP:
		case QSDR::QtDSP:
		case QSDR::ChirpWSPR:
		case QSDR::ChirpWSPRFile:

			/*connect(
				this,
				SIGNAL(iqDataReady(int)),
				SLOT(dttSPDspProcessing(int)),
				Qt::DirectConnection);*/
			
			break;
			
		case QSDR::NoServerMode:
		case QSDR::DemoMode:
			break;

		/*case QSDR::ChirpWSPR:
		case QSDR::ChirpWSPRFile:
			break;*/
	}

	m_dataProcThread = new QThreadEx();
	m_dataProcessor->moveToThread(m_dataProcThread);

	switch (m_hwInterface) {

		case QSDR::NoInterfaceMode:
			m_dataProcessor->connect(
						m_dataProcThread, 
						SIGNAL(started()), 
						SLOT(processData()));
			break;
			
		case QSDR::Metis:
		case QSDR::Hermes:
			m_dataProcessor->connect(
						m_dataProcThread, 
						SIGNAL(started()), 
						SLOT(processDeviceData()));

			break;
	}
	
	//m_dataProcessor->connect(m_dataProcThread, SIGNAL(started()), SLOT(initDataProcessorSocket()));

}

bool DataEngine::startDataProcessor(QThread::Priority prio) {

	m_dataProcThread->start(prio);
				
	if (m_dataProcThread->isRunning()) {
					
		m_dataProcThreadRunning = true;
		DATA_ENGINE_DEBUG << "data processor thread started.";

		return true;
	}
	else {

		m_dataProcThreadRunning = false;
		settings->setSystemState(
						this, 
						QSDR::DataProcessThreadError, 
						m_hwInterface, 
						m_serverMode,
						QSDR::DataEngineDown);
		return false;
	}
}

void DataEngine::stopDataProcessor() {

	if (m_dataProcThread->isRunning()) {
					
		m_dataProcessor->stop();
		
		//if (m_serverMode == QSDR::InternalDSP || m_serverMode == QSDR::ChirpWSPR) {
		if (m_serverMode == QSDR::DttSP || m_serverMode == QSDR::QtDSP || m_serverMode == QSDR::ChirpWSPR) {
			
			if (io.iq_queue.isEmpty())
				io.iq_queue.enqueue(QByteArray(BUFFER_SIZE, 0x0));
		}
		else if (m_serverMode == QSDR::ChirpWSPRFile) {

			if (io.data_queue.isEmpty()) {
				
				QList<qreal> buf;
				for (int i = 0; i < 128; i++) buf << 0.0f;
				io.data_queue.enqueue(buf);
			}
		}
				
		m_dataProcThread->quit();
		m_dataProcThread->wait();
		delete m_dataProcThread;
		delete m_dataProcessor;
		m_dataProcessor = 0;

		//if (m_serverMode == QSDR::InternalDSP || m_serverMode == QSDR::ChirpWSPR) {
		if (m_serverMode == QSDR::DttSP || m_serverMode == QSDR::QtDSP || m_serverMode == QSDR::ChirpWSPR) {

			while (!io.iq_queue.isEmpty())
				io.iq_queue.dequeue();

			DATA_ENGINE_DEBUG << "iq_queue empty.";
		}
		else if (m_serverMode == QSDR::ChirpWSPRFile) {
			
			while (!io.data_queue.isEmpty())
				io.data_queue.dequeue();

			DATA_ENGINE_DEBUG << "data_queue empty.";
			chirpData.clear();
		}

		m_dataProcThreadRunning = false;

		DATA_ENGINE_DEBUG << "data processor thread deleted.";
	}
	else
		DATA_ENGINE_DEBUG << "data processor thread wasn't started.";
}
 
//********************************************************
// create, start/stop winde band data processor

void DataEngine::createWideBandDataProcessor() {

	int size;

	if (m_mercuryFW > 32 || m_hermesFW > 16)
		size = BIGWIDEBANDSIZE;
	else
		size = SMALLWIDEBANDSIZE;
	
	m_wbDataProcessor = new WideBandDataProcessor(this);

	CHECKED_CONNECT(
		m_wbDataProcessor, 
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	switch (m_serverMode) {
		
		// The signal iqDataReady is generated by the function
		// processInputBuffer when a block of input data are
		// decoded.
		//case QSDR::InternalDSP:
		case QSDR::DttSP:
		case QSDR::QtDSP:

			m_wbFFT = new QFFT(this, size);
			
			cpxWBIn.resize(size);
			cpxWBOut.resize(size);

			io.wbWindow.resize(size);
			io.wbWindow.fill(0.0f);
			
			QFilter::MakeWindow(12, size, (float *)io.wbWindow.data()); // 12 = BLACKMANHARRIS_WINDOW

			m_wbAverager = new DualModeAverager(this, size/2);

			break;

		case QSDR::ExternalDSP:
		case QSDR::ChirpWSPR:
		case QSDR::ChirpWSPRFile:
			break;
			
		case QSDR::NoServerMode:
		case QSDR::DemoMode:
			break;
	}

	m_wbDataProcThread = new QThreadEx();
	m_wbDataProcessor->moveToThread(m_wbDataProcThread);
	m_wbDataProcessor->connect(
						m_wbDataProcThread, 
						SIGNAL(started()), 
						SLOT(processWideBandData()));
}

bool DataEngine::startWideBandDataProcessor(QThread::Priority prio) {

	m_wbDataProcThread->start(prio);//(QThread::TimeCriticalPriority);//(QThread::HighPriority);//(QThread::LowPriority);

	if (m_wbDataProcThread->isRunning()) {
					
		m_wbDataRcvrThreadRunning = true;
		DATA_ENGINE_DEBUG << "wide band data processor thread started.";

		return true;
	}
	else {

		m_wbDataRcvrThreadRunning = false;
		settings->setSystemState(
						this, 
						QSDR::WideBandDataProcessThreadError, 
						m_hwInterface, 
						m_serverMode,
						QSDR::DataEngineDown);
		return false;
	}
}

void DataEngine::stopWideBandDataProcessor() {

	if (m_wbDataProcThread->isRunning()) {
					
		m_wbDataProcessor->stop();
		if (io.wb_queue.isEmpty())
			io.wb_queue.enqueue(m_datagram);

		m_wbDataProcThread->quit();
		m_wbDataProcThread->wait();
		delete m_wbDataProcThread;
		delete m_wbDataProcessor;
		m_wbDataProcessor = 0;

		m_wbDataRcvrThreadRunning = false;

		delete m_wbFFT;
		cpxWBIn.clear();
		cpxWBOut.clear();

		if (m_wbAverager)
			delete m_wbAverager;
		
		DATA_ENGINE_DEBUG << "wide band data processor thread deleted.";
	}
	else
		DATA_ENGINE_DEBUG << "wide band data processor thread wasn't started.";
}
 
//********************************************************
// create, start/stop chirp processor
void DataEngine::createChirpDataProcessor() {

	m_chirpProcessor = new ChirpProcessor(&io);
	sendMessage("chirp decoder initialized.");
	
	CHECKED_CONNECT(
		m_chirpProcessor, 
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	CHECKED_CONNECT_OPT(
		m_audioEngine, 
		SIGNAL(chirpSignalChanged()),
		m_chirpProcessor,
		SLOT(generateLocalChirp()),
		Qt::DirectConnection);

	m_audioEngine->reset();
	if (m_audioEngine->generateSweptTone())
		sendMessage("audio chirp signal initialized.");
	else
		sendMessage("audio chirp signal initialization failed.");


	m_chirpDataProcThread = new QThreadEx();
	m_chirpProcessor->moveToThread(m_chirpDataProcThread);
	m_chirpProcessor->connect(
						m_chirpDataProcThread, 
						SIGNAL(started()),
						m_chirpProcessor,
						SLOT(processChirpData()));

	m_chirpInititalized = true;
}

bool DataEngine::startChirpDataProcessor(QThread::Priority prio) {

	m_chirpDataProcThread->start(prio);//(QThread::TimeCriticalPriority);//(QThread::HighPriority);//(QThread::LowPriority);
				
	if (m_chirpDataProcThread->isRunning()) {
					
		m_chirpDataProcThreadRunning = true;
		DATA_ENGINE_DEBUG << "chirp data processor thread started.";

		return true;
	}
	else {

		m_chirpDataProcThreadRunning = false;
		settings->setSystemState(
						this, 
						QSDR::DataProcessThreadError, 
						m_hwInterface, 
						m_serverMode,
						QSDR::DataEngineDown);
		return false;
	}
}

void DataEngine::stopChirpDataProcessor() {

	if (m_chirpInititalized) {

		m_chirpProcessor->stop();
		if (io.chirp_queue.isEmpty()) {
				
			QList<qreal> buf;
			for (int i = 0; i < 128; i++) buf << 0.0f;
				io.chirp_queue.enqueue(buf);
			}

			m_chirpDataProcThread->quit();
			m_chirpDataProcThread->wait();
			delete m_chirpDataProcThread;
			delete m_chirpProcessor;
			m_chirpProcessor = 0;

			if (m_hwInterface == QSDR::NoInterfaceMode) {

				//freeCPX(io.cpxIn);
				//freeCPX(io.cpxOut);
				delete m_chirpDspEngine;

				while (!io.chirp_queue.isEmpty())
					io.chirp_queue.dequeue();

				DATA_ENGINE_DEBUG << "io.cpxIn, io.cpxOut, fft deleted, io.chirp_queue empty.";
			}

			m_chirpInititalized = false;

			DATA_ENGINE_DEBUG << "chirp data processor thread deleted.";
	}
	else
		DATA_ENGINE_DEBUG << "chirp data processor thread wasn't started.";
}
 
//********************************************************
// create, start/stop audio receiver

void DataEngine::createAudioReceiver() {

	m_audioReceiver = new AudioReceiver(&io);

	CHECKED_CONNECT(
		m_audioReceiver, 
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	CHECKED_CONNECT(
		m_audioReceiver, 
		SIGNAL(rcveIQEvent(QObject *, int)), 
		this, 
		SLOT(setRcveIQSignal(QObject *, int)));

	CHECKED_CONNECT(
		m_audioReceiver, 
		SIGNAL(clientConnectedEvent(bool)), 
		this, 
		SLOT(setClientConnected(bool)));

	
	m_AudioRcvrThread = new QThreadEx();
	m_audioReceiver->moveToThread(m_AudioRcvrThread);

	m_audioReceiver->connect(
						m_AudioRcvrThread, 
						SIGNAL(started()), 
						SLOT(initClient()));
}

void DataEngine::createAudioProcessor() {

	m_audioProcessor = new AudioProcessor(this);
	
	CHECKED_CONNECT(
		m_audioProcessor, 
		SIGNAL(messageEvent(QString)), 
		this, 
		SLOT(newMessage(QString)));

	/*CHECKED_CONNECT(
		m_audioProcessor, 
		SIGNAL(AudioProcessorRunningEvent(bool)), 
		this, 
		SLOT(setAudioProcessorRunning(bool)));*/

	CHECKED_CONNECT_OPT(
		this, 
		SIGNAL(clientConnectedEvent(int)), 
		m_audioProcessor, 
		SLOT(clientConnected(int)), 
		Qt::DirectConnection);

	CHECKED_CONNECT_OPT(
		this, 
		SIGNAL(audioRxEvent(int)), 
		m_audioProcessor, 
		SLOT(audioReceiverChanged(int)), 
		Qt::DirectConnection);

	CHECKED_CONNECT_OPT(
		this,
		SIGNAL(audioDataReady()),
		m_audioProcessor,
		SLOT(deviceWriteBuffer()),
		Qt::DirectConnection);

	switch (m_serverMode) {

		case QSDR::ExternalDSP:
			
			m_audioProcThread = new QThreadEx();
			m_audioProcessor->moveToThread(m_audioProcThread);
			m_audioProcessor->connect(
								m_audioProcThread, 
								SIGNAL(started()), 
								SLOT(processAudioData()));
			break;

		//case QSDR::InternalDSP:
		case QSDR::DttSP:
		case QSDR::QtDSP:
			break;
			
		case QSDR::NoServerMode:
		case QSDR::DemoMode:
		case QSDR::ChirpWSPR:
		case QSDR::ChirpWSPRFile:
			break;
	}

	/*m_audioProcThread = new QThreadEx();
	m_audioProcessor->moveToThread(m_udioProcThread);
	m_audioProcessor->connect(m_audioProcThread, SIGNAL(started()), SLOT(processAudioData()));*/

	//setSampleRate(this, settings->getSampleRate());
}

bool DataEngine::startAudioProcessor(QThread::Priority prio, QSDR::_ServerMode mode) {

	if (!m_audioProcessorRunning) {

		switch (mode) {

			//case QSDR::InternalDSP:
			case QSDR::DttSP:
			case QSDR::QtDSP:
			case QSDR::ChirpWSPR:

				m_audioProcessor->initAudioProcessorSocket();
				m_audioProcessorRunning = true;

				return true;
				
			case QSDR::ExternalDSP:
			case QSDR::NoServerMode:
			case QSDR::DemoMode:
			case QSDR::ChirpWSPRFile:
				break;
		}

		return false;
	}
	else
		return false;
}

void DataEngine::stopAudioProcessor() {

	if (m_audioProcessorRunning) {
				
		m_audioProcessor->stop();
		io.au_queue.enqueue(m_datagram);

		if (m_audioProcThreadRunning) {
			m_audioProcThread->quit();
			m_audioProcThread->wait();
		}
		delete m_audioProcessor;
		m_audioProcessor = 0;

		m_audioProcessorRunning = false;

		DATA_ENGINE_DEBUG << "audio processor thread deleted.";
	}
	else
		DATA_ENGINE_DEBUG << "audio processor thread wasn't started.";
}
 
void DataEngine::displayDiscoverySocketError(QAbstractSocket::SocketError error) {

	DATA_ENGINE_DEBUG << "discovery socket error:" << error;
}

 
//*****************************************************************************
// HPSDR data processing functions

void DataEngine::processInputBuffer(const QByteArray &buffer) {

	int s = 0;

	if (buffer.at(s++) == SYNC && buffer.at(s++) == SYNC && buffer.at(s++) == SYNC) {

		// extract control bytes
        io.control_in[0] = buffer.at(s++);
        io.control_in[1] = buffer.at(s++);
        io.control_in[2] = buffer.at(s++);
        io.control_in[3] = buffer.at(s++);
        io.control_in[4] = buffer.at(s++);

        io.ccRx.ptt    = (bool)((io.control_in[0] & 0x01) == 0x01);
		io.ccRx.dash   = (bool)((io.control_in[0] & 0x02) == 0x02);
		io.ccRx.dot    = (bool)((io.control_in[0] & 0x04) == 0x04);
		io.ccRx.lt2208 = (bool)((io.control_in[1] & 0x01) == 0x01);


		io.ccRx.roundRobin = (uchar)(io.control_in[0] >> 3);
        switch (io.ccRx.roundRobin) { // cycle through C0

			case 0:

				if (io.ccRx.lt2208) { // check ADC signal
					
					if (m_ADCChangedTime.elapsed() > 50) {

						settings->setADCOverflow(2);
						m_ADCChangedTime.restart();
					}
				}

				//qDebug() << "CC: " << io.ccRx.roundRobin;
				if (m_hwInterface == QSDR::Hermes) {

					io.ccRx.hermesI01 = (bool)((io.control_in[1] & 0x02) == 0x02);
					io.ccRx.hermesI02 = (bool)((io.control_in[1] & 0x04) == 0x04);
					io.ccRx.hermesI03 = (bool)((io.control_in[1] & 0x08) == 0x08);
					io.ccRx.hermesI04 = (bool)((io.control_in[1] & 0x10) == 0x10);
					//qDebug() << "Hermes IO 1: " << io.ccRx.hermesI01 << "2: " << io.ccRx.hermesI02 << "3: " << io.ccRx.hermesI03 << "4: " << io.ccRx.hermesI04;
				}

				if (m_fwCount < 100) {

					if (m_hwInterface == QSDR::Metis) {
				
						if (io.ccRx.mercuryFirmwareVersion != io.control_in[2]) {
							io.ccRx.mercuryFirmwareVersion = io.control_in[2];
							settings->setMercuryVersion(io.ccRx.mercuryFirmwareVersion);
							m_message = "Mercury firmware version: %1.";
							sendMessage(m_message.arg(QString::number(io.control_in[2])));
						}
			
						if (io.ccRx.penelopeFirmwareVersion != io.control_in[3]) {
							io.ccRx.penelopeFirmwareVersion = io.control_in[3];
							settings->setPenelopeVersion(io.ccRx.penelopeFirmwareVersion);
							m_message = "Penelope firmware version: %1.";
							sendMessage(m_message.arg(QString::number(io.control_in[3])));
						}
			
						if (io.ccRx.networkDeviceFirmwareVersion != io.control_in[4]) {
							io.ccRx.networkDeviceFirmwareVersion = io.control_in[4];
							settings->setMetisVersion(io.ccRx.networkDeviceFirmwareVersion);
							m_message = "Metis firmware version: %1.";

							sendMessage(m_message.arg(QString::number(io.control_in[4])));
						}
					}
					else if (m_hwInterface == QSDR::Hermes) {

						if (io.ccRx.networkDeviceFirmwareVersion != io.control_in[4]) {
							io.ccRx.networkDeviceFirmwareVersion = io.control_in[4];
							settings->setHermesVersion(io.ccRx.networkDeviceFirmwareVersion);
							m_message = "Hermes firmware version: %1.";

							sendMessage(m_message.arg(QString::number(io.control_in[4])));
						}
					}
					m_fwCount++;
				}
				break;

			case 1:

				//qDebug() << "CC: " << io.ccRx.roundRobin;
				// forward power
				if (settings->getPenelopePresence() || (m_hwInterface == QSDR::Hermes)) { // || settings->getPennyLanePresence()
					
					io.ccRx.ain5 = (quint16)((quint16)(io.control_in[1] << 8) + (quint16)io.control_in[2]);
					
					io.penelopeForwardVolts = (qreal)(3.3 * (qreal)io.ccRx.ain5 / 4095.0);
					io.penelopeForwardPower = (qreal)(io.penelopeForwardVolts * io.penelopeForwardVolts / 0.09);
				}
				//qDebug() << "penelopeForwardVolts: " << io.penelopeForwardVolts << "penelopeForwardPower" << io.penelopeForwardPower;
				
				if (settings->getAlexPresence()) { //|| settings->getApolloPresence()) {
					
					io.ccRx.ain1 = (quint16)((quint16)(io.control_in[3] << 8) + (quint16)io.control_in[4]);
					
					io.alexForwardVolts = (qreal)(3.3 * (qreal)io.ccRx.ain1 / 4095.0);
					io.alexForwardPower = (qreal)(io.alexForwardVolts * io.alexForwardVolts / 0.09);
				}
				//qDebug() << "alexForwardVolts: " << io.alexForwardVolts << "alexForwardPower" << io.alexForwardPower;
                break;

			case 2:

				//qDebug() << "CC: " << io.ccRx.roundRobin;
				// reverse power
				if (settings->getAlexPresence()) { //|| settings->getApolloPresence()) {
					
					io.ccRx.ain2 = (quint16)((quint16)(io.control_in[1] << 8) + (quint16)io.control_in[2]);
					
					io.alexReverseVolts = (qreal)(3.3 * (qreal)io.ccRx.ain2 / 4095.0);
					io.alexReversePower = (qreal)(io.alexReverseVolts * io.alexReverseVolts / 0.09);
				}
				//qDebug() << "alexReverseVolts: " << io.alexReverseVolts << "alexReversePower" << io.alexReversePower;
				
				if (settings->getPenelopePresence() || (m_hwInterface == QSDR::Hermes)) { // || settings->getPennyLanePresence() {
					
					io.ccRx.ain3 = (quint16)((quint16)(io.control_in[3] << 8) + (quint16)io.control_in[4]);
					io.ain3Volts = (qreal)(3.3 * (double)io.ccRx.ain3 / 4095.0);
				}
				//qDebug() << "ain3Volts: " << io.ain3Volts;
				break;

			case 3:

				//qDebug() << "CC: " << io.ccRx.roundRobin;
				
				if (settings->getPenelopePresence() || (m_hwInterface == QSDR::Hermes)) { // || settings->getPennyLanePresence() {
					
					io.ccRx.ain4 = (quint16)((quint16)(io.control_in[1] << 8) + (quint16)io.control_in[2]);
					io.ccRx.ain6 = (quint16)((quint16)(io.control_in[3] << 8) + (quint16)io.control_in[4]);
					
					io.ain4Volts = (qreal)(3.3 * (qreal)io.ccRx.ain4 / 4095.0);
					
					if (m_hwInterface == QSDR::Hermes) // read supply volts applied to board
						io.supplyVolts = (qreal)((qreal)io.ccRx.ain6 / 186.0f);
				}
				//qDebug() << "ain4Volts: " << io.ain4Volts << "supplyVolts" << io.supplyVolts;
				break;

			//case 4:

				// more than 1 Mercury module (currently not usable)
				//qDebug() << "CC: " << io.ccRx.roundRobin;
				//switch (io.receivers) {

				//	case 1:
				//		io.ccRx.mercury1_LT2208 = (bool)((io.control_in[1] & 0x02) == 0x02);
				//		//qDebug() << "mercury1_LT2208: " << io.ccRx.mercury1_LT2208;
				//		break;

				//	case 2:
				//		io.ccRx.mercury1_LT2208 = (bool)((io.control_in[1] & 0x02) == 0x02);
				//		io.ccRx.mercury2_LT2208 = (bool)((io.control_in[2] & 0x02) == 0x02);
				//		//qDebug() << "mercury1_LT2208: " << io.ccRx.mercury1_LT2208 << "mercury2_LT2208" << io.ccRx.mercury2_LT2208;
				//		break;

				//	case 3:
				//		io.ccRx.mercury1_LT2208 = (bool)((io.control_in[1] & 0x02) == 0x02);
				//		io.ccRx.mercury2_LT2208 = (bool)((io.control_in[2] & 0x02) == 0x02);
				//		io.ccRx.mercury3_LT2208 = (bool)((io.control_in[3] & 0x02) == 0x02);
				//		//qDebug() << "mercury1_LT2208: " << io.ccRx.mercury1_LT2208 << "mercury2_LT2208" << io.ccRx.mercury2_LT2208;
				//		//qDebug() << "mercury3_LT2208: " << io.ccRx.mercury3_LT2208;
				//		break;

				//	case 4:
				//		io.ccRx.mercury1_LT2208 = (bool)((io.control_in[1] & 0x02) == 0x02);
				//		io.ccRx.mercury2_LT2208 = (bool)((io.control_in[2] & 0x02) == 0x02);
				//		io.ccRx.mercury3_LT2208 = (bool)((io.control_in[3] & 0x02) == 0x02);
				//		io.ccRx.mercury4_LT2208 = (bool)((io.control_in[4] & 0x02) == 0x02);
				//		//qDebug() << "mercury1_LT2208: " << io.ccRx.mercury1_LT2208 << "mercury2_LT2208" << io.ccRx.mercury2_LT2208;
				//		//qDebug() << "mercury3_LT2208: " << io.ccRx.mercury3_LT2208 << "mercury4_LT2208" << io.ccRx.mercury4_LT2208;
				//		break;
				//}
				//break;
		}

        switch (io.receivers) {

            case 1: m_maxSamples = 512-0;  break;
            case 2: m_maxSamples = 512-0;  break;
            case 3: m_maxSamples = 512-4;  break;
            case 4: m_maxSamples = 512-10; break;
            case 5: m_maxSamples = 512-24; break;
            case 6: m_maxSamples = 512-10; break;
            case 7: m_maxSamples = 512-20; break;
            case 8: m_maxSamples = 512-4;  break;
        }

        // extract the samples
        while (s < m_maxSamples) {

            // extract each of the receivers
            for (int r = 0; r < io.receivers; r++) {

                m_leftSample   = (int)((  signed char) buffer.at(s++)) << 16;
                m_leftSample  += (int)((unsigned char) buffer.at(s++)) << 8;
                m_leftSample  += (int)((unsigned char) buffer.at(s++));
                m_rightSample  = (int)((  signed char) buffer.at(s++)) << 16;
                m_rightSample += (int)((unsigned char) buffer.at(s++)) << 8;
                m_rightSample += (int)((unsigned char) buffer.at(s++));
				
				m_lsample = (float)(m_leftSample / 8388607.0);
				m_rsample = (float)(m_rightSample / 8388607.0);

				if (m_serverMode == QSDR::ChirpWSPR && 
					m_chirpInititalized && 
					m_chirpSamples < io.samplerate
				) {
					chirpData << m_lsample;
					chirpData << m_rsample;
				}

				if (m_serverMode == QSDR::DttSP) {
					
					RX[r]->in[m_rxSamples]				 = m_lsample; // 24 bit sample
					RX[r]->in[m_rxSamples + BUFFER_SIZE] = m_rsample; // 24 bit sample
				}
				else if (m_serverMode == QSDR::QtDSP) {
					
					RX[r]->inBuf[m_rxSamples].re = m_lsample; // 24 bit sample
					RX[r]->inBuf[m_rxSamples].im = m_rsample; // 24 bit sample
				}
            }

            m_micSample = (int)((signed char) buffer.at(s++)) << 8;

			// extract chirp signal time stamp
			m_chirpBit = (buffer.at(s) & 0x01);// == 0x01;
			
			m_micSample += (int)((unsigned char) buffer.at(s++));
    		m_micSample_float = (float) m_micSample / 32767.0 * io.mic_gain; // 16 bit sample

            // add to buffer
            io.mic_left_buffer[m_rxSamples]  = m_micSample_float;
            io.mic_right_buffer[m_rxSamples] = 0.0f;

			//m_chirpSamples++;

			if (m_serverMode == QSDR::ChirpWSPR && m_chirpInititalized) {
				if (m_chirpBit) {
					if (m_chirpGateBit) {
					
						// we've found the rising edge of the GPS 1PPS signal, so we set the samples 
						// counter back to zero in order to have a simple and precise synchronisation 
						// with the local chirp.
						DATA_ENGINE_DEBUG << "GPS 1 PPS";

						// remove the last sample (real and imag) and enqueue the buffer
						chirpData.removeLast();
						chirpData.removeLast();
						io.chirp_queue.enqueue(chirpData);

						// empty the buffer and add the last sample, which is the starting point of the chirp
						m_chirpSamples = 0;
						chirpData.clear();

						chirpData << m_lsample;
						chirpData << m_rsample;

						m_chirpStart = true;
						m_chirpStartSample = m_rxSamples;
						m_chirpGateBit = false;
					}
				}
				else
					m_chirpGateBit = true;
			}
			m_rxSamples++;
			m_chirpSamples++;

			// when we have enough rx samples we start the DSP processing.
            if (m_rxSamples == BUFFER_SIZE) {
				
				// version 1
				/*for (int r = 0; r < io.receivers; r++) {

					QFuture<void> dspResult = run(spin, r);
					dttspReadyAt(r);
				}*/

				// version 2
				//dttspProcessing->setFuture(QtConcurrent::mapped(m_rx, spin));
				

				// classic version 1
				/*for (int r = 0; r < io.receivers; r++)
					dspProcessing(r);*/

				// classic version 2
				if (m_serverMode == QSDR::DttSP)
					dttspProcessing();
				else if (m_serverMode == QSDR::QtDSP)
					qtdspProcessing();

				m_rxSamples = 0;
            }
        }
    } 
	else {

		if (m_SyncChangedTime.elapsed() > 50) {

			settings->setProtocolSync(2);
			m_SyncChangedTime.restart();
		}
	}
}

void DataEngine::processWideBandInputBuffer(const QByteArray &buffer) {

	int size;

	if (m_mercuryFW > 32 || m_hermesFW > 16)
		size = 2 * BIGWIDEBANDSIZE;
	else
		size = 2 * SMALLWIDEBANDSIZE;
	
	qint64 length = buffer.length();
	if (buffer.length() != size) {

		DATA_PROCESSOR_DEBUG << "wrong wide band buffer length: " << length;
		return;
	}

	int s;
	float sample;
	float norm = 1.0f / (4 * size);
	
	for (int i = 0; i < length; i += 2) {

		s =  (int)((qint8 ) buffer.at(i+1)) << 8;
		s += (int)((quint8) buffer.at(i));
		sample = (float)(s * norm);

		cpxWBIn[i/2].re = sample * io.wbWindow.at(i/2);
		cpxWBIn[i/2].im = sample * io.wbWindow.at(i/2);
	}

	m_wbFFT->DoFFTWForward(cpxWBIn, cpxWBOut, size/2);
	
	// averaging
	QVector<float> specBuf(size/4);

	m_wbMutex.lock();
	if (m_wbSpectrumAveraging) {
		
		for (int i = 0; i < size/4; i++)
			specBuf[i] = (float)(10.0 * log10(MagCPX(cpxWBOut.at(i)) + 1.5E-45));

		m_wbAverager->ProcessDBAverager(specBuf, specBuf);
		m_wbMutex.unlock();
	}
	else {

		for (int i = 0; i < size/4; i++)
			specBuf[i] = (float)(10.0 * log10(MagCPX(cpxWBOut.at(i)) + 1.5E-45));

		m_wbMutex.unlock();
	}

	settings->setWidebandSpectrumBuffer(specBuf);
}

void DataEngine::processFileBuffer(const QList<qreal> buffer) {

	
	int topsize = 2*BUFFER_SIZE - 1;
	//float specMax = -100.0f;
	//float specMin = 0.0f;

	Q_ASSERT(buffer.length() == 128);

	for (int i = 0; i < 64; i++) {
		
		cpxIn[i + m_rxSamples].re = buffer.at(2*i);
		cpxIn[i + m_rxSamples].im = buffer.at(2*i+1);

		chirpData << buffer.at(2*i);
		chirpData << buffer.at(2*i+1);

		m_chirpSamples++;
		if (m_chirpSamples == io.samplerate) {

			io.chirp_queue.enqueue(chirpData);
			chirpData.clear();
			m_chirpSamples = 0;
		}
	}
	m_rxSamples += 64;
		
	if (m_rxSamples == 2*BUFFER_SIZE) {
			
		m_chirpDspEngine->fft->DoFFTWForward(cpxIn, cpxOut, 2*BUFFER_SIZE);
			
		// reorder the spectrum buffer
		for (int i = 0; i < BUFFER_SIZE; i++) {
			
			m_spectrumBuffer[topsize - i] =
				(float)(10.0 * log10(MagCPX(cpxOut[i+BUFFER_SIZE]) + 1.5E-45));
			m_spectrumBuffer[BUFFER_SIZE - i] =
				(float)(10.0 * log10(MagCPX(cpxOut[i]) + 1.5E-45));
		}

		/*float specMean = 0.0f;
		for (int i = BUFFER_SIZE+20; i < BUFFER_SIZE+105; i++) {

			specMean += m_spectrumBuffer[i];
			if (m_spectrumBuffer[i] > specMax) specMax = m_spectrumBuffer[i];
			if (m_spectrumBuffer[i] < specMin) specMin = m_spectrumBuffer[i];
		}*/
		//specMean *= 1.0f/BUFFER_SIZE;
		//DATA_PROCESSOR_DEBUG << "pan min" << specMin << "max" << specMax << "mean" << specMean;

		SleeperThread::usleep(42667);
	
		//emit spectrumBufferChanged(m_spectrumBuffer);
		//settings->setSpectrumBuffer(m_spectrumBuffer);
		settings->setSpectrumBuffer(0, m_spectrumBuffer);
		
		m_rxSamples = 0;
	}
}

void DataEngine::processOutputBuffer(float *left, float *right) {

	qint16 leftRXSample;
    qint16 rightRXSample;
    qint16 leftTXSample;
    qint16 rightTXSample;

	// process the output
	for (int j = 0; j < BUFFER_SIZE; j += io.outputMultiplier) {

		leftRXSample  = (qint16)(left[j] * 32767.0f);
        rightRXSample = (qint16)(right[j] * 32767.0f);

		leftTXSample = 0;
        rightTXSample = 0;

		io.output_buffer[m_idx++] = leftRXSample  >> 8;
        io.output_buffer[m_idx++] = leftRXSample;
        io.output_buffer[m_idx++] = rightRXSample >> 8;
        io.output_buffer[m_idx++] = rightRXSample;
        io.output_buffer[m_idx++] = leftTXSample  >> 8;
        io.output_buffer[m_idx++] = leftTXSample;
        io.output_buffer[m_idx++] = rightTXSample >> 8;
        io.output_buffer[m_idx++] = rightTXSample;
		
		if (m_idx == IO_BUFFER_SIZE) {

			writeControlBytes();
			m_idx = IO_HEADER_SIZE;
		}
    }
}

void DataEngine::processOutputBuffer(const CPX &buffer) {

	qint16 leftRXSample;
    qint16 rightRXSample;
    qint16 leftTXSample;
    qint16 rightTXSample;

	// process the output
	for (int j = 0; j < BUFFER_SIZE; j += io.outputMultiplier) {

		leftRXSample  = (qint16)(buffer.at(j).re * 32767.0f);
		rightRXSample = (qint16)(buffer.at(j).im * 32767.0f);

		leftTXSample = 0;
        rightTXSample = 0;

		io.output_buffer[m_idx++] = leftRXSample  >> 8;
        io.output_buffer[m_idx++] = leftRXSample;
        io.output_buffer[m_idx++] = rightRXSample >> 8;
        io.output_buffer[m_idx++] = rightRXSample;
        io.output_buffer[m_idx++] = leftTXSample  >> 8;
        io.output_buffer[m_idx++] = leftTXSample;
        io.output_buffer[m_idx++] = rightTXSample >> 8;
        io.output_buffer[m_idx++] = rightTXSample;
		
		if (m_idx == IO_BUFFER_SIZE) {

			writeControlBytes();
			m_idx = IO_HEADER_SIZE;
		}
	}
}

void DataEngine::writeControlBytes() {

	io.output_buffer[0] = SYNC;
    io.output_buffer[1] = SYNC;
    io.output_buffer[2] = SYNC;
	
    io.mutex.lock();
    switch (m_sendState) {

    	case 0:

    		uchar rxAnt;
    		uchar rxOut;
    		uchar ant;

    		io.control_out[0] = 0x0; // C0
    		io.control_out[1] = 0x0; // C1
    		io.control_out[2] = 0x0; // C2
    		io.control_out[3] = 0x0; // C3
    		io.control_out[4] = 0x0; // C4

    		// C0
    		// 0 0 0 0 0 0 0 0
    		//               |
    		//               +------------ MOX (1 = active, 0 = inactive)

    		// set C1
    		//
    		// 0 0 0 0 0 0 0 0
    		// | | | | | | | |
    		// | | | | | | + +------------ Speed (00 = 48kHz, 01 = 96kHz, 10 = 192kHz)
    		// | | | | + +---------------- 10MHz Ref. (00 = Atlas/Excalibur, 01 = Penelope, 10 = Mercury)*
    		// | | | +-------------------- 122.88MHz source (0 = Penelope, 1 = Mercury)*
    		// | + +---------------------- Config (00 = nil, 01 = Penelope, 10 = Mercury, 11 = both)*
    		// +-------------------------- Mic source (0 = Janus, 1 = Penelope)*
    		//
   			// * Ignored by Hermes

    		io.control_out[1] |= io.speed; // sample rate

    		io.control_out[1] &= 0x03; // 0 0 0 0 0 0 1 1
    		io.control_out[1] |= io.ccTx.clockByte;

    		// set C2
    		//
    		// 0 0 0 0 0 0 0 0
    		// |           | |
    		// |           | +------------ Mode (1 = Class E, 0 = All other modes)
    		// +---------- +-------------- Open Collector Outputs on Penelope or Hermes (bit 6...bit 0)

    		io.control_out[2] = io.rxClass;

    		if (io.ccTx.pennyOCenabled) {

    			io.control_out[2] &= 0x1; // 0 0 0 0 0 0 0 1

    			if (io.ccTx.currentBand != (HamBand) gen) {

    				if (io.ccTx.mox || io.ccTx.ptt)
    					io.control_out[2] |= (io.ccTx.txJ6pinList.at(io.ccTx.currentBand) >> 1) << 1;
    				else
    					io.control_out[2] |= (io.ccTx.rxJ6pinList.at(io.ccTx.currentBand) >> 1) << 1;
    			}
    		}


    		// set C3
    		//
    		// 0 0 0 0 0 0 0 0
    		// | | | | | | | |
    		// | | | | | | + +------------ Alex Attenuator (00 = 0dB, 01 = 10dB, 10 = 20dB, 11 = 30dB)
    		// | | | | | +---------------- Preamp On/Off (0 = Off, 1 = On)
    		// | | | | +------------------ LT2208 Dither (0 = Off, 1 = On)
    		// | | | + ------------------- LT2208 Random (0= Off, 1 = On)
    		// | + + --------------------- Alex Rx Antenna (00 = none, 01 = Rx1, 10 = Rx2, 11 = XV)
    		// + ------------------------- Alex Rx out (0 = off, 1 = on). Set if Alex Rx Antenna > 00.


    		rxAnt = 0x07 & (io.ccTx.alexStates.at(io.ccTx.currentBand) >> 2);
    		rxOut = (rxAnt > 0) ? 1 : 0;

    		io.control_out[3] = (io.ccTx.alexStates.at(io.ccTx.currentBand) >> 7);

    		io.control_out[3] &= 0xFB; // 1 1 1 1 1 0 1 1
    		io.control_out[3] |= (io.ccTx.mercuryAttenuator << 2);

    		io.control_out[3] &= 0xF7; // 1 1 1 1 0 1 1 1
    		io.control_out[3] |= (io.ccTx.dither << 3);

    		io.control_out[3] &= 0xEF; // 1 1 1 0 1 1 1 1
    		io.control_out[3] |= (io.ccTx.random << 4);

    		io.control_out[3] &= 0x9F; // 1 0 0 1 1 1 1 1
    		io.control_out[3] |= rxAnt << 5;

    		io.control_out[3] &= 0x7F; // 0 1 1 1 1 1 1 1
    		io.control_out[3] |= rxOut << 7;

    		// set C4
    		//
    		// 0 0 0 0 0 0 0 0
    		// | | | | | | | |
    		// | | | | | | + + ----------- Alex Tx relay (00 = Tx1, 01= Tx2, 10 = Tx3)
    		// | | | | | + --------------- Duplex (0 = off, 1 = on)
    		// | | + + +------------------ Number of Receivers (000 = 1, 111 = 8)
    		// | +------------------------ Time stamp – 1PPS on LSB of Mic data (0 = off, 1 = on)
    		// +-------------------------- Common Mercury Frequency (0 = independent frequencies to Mercury
    		//			                   Boards, 1 = same frequency to all Mercury boards)

    		if (io.ccTx.mox || io.ccTx.ptt)
    			ant = (io.ccTx.alexStates.at(io.ccTx.currentBand) >> 5);
    		else
    			ant = io.ccTx.alexStates.at(io.ccTx.currentBand);

    		io.control_out[4] |= (ant != 0) ? ant-1 : ant;

    		io.control_out[4] &= 0xFB; // 1 1 1 1 1 0 1 1
    		io.control_out[4] |= io.ccTx.duplex << 2;

    		io.control_out[4] &= 0xC7; // 1 1 0 0 0 1 1 1
    		io.control_out[4] |= (io.receivers - 1) << 3;

    		io.control_out[4] &= 0xBF; // 1 0 1 1 1 1 1 1
    		io.control_out[4] |= io.ccTx.timeStamp << 6;

    		io.control_out[4] &= 0x7F; // 0 1 1 1 1 1 1 1
    		io.control_out[4] |= io.ccTx.commonMercuryFrequencies << 7;

    		// fill the out buffer with the C&C bytes
    		for (int i = 0; i < 5; i++)
    			io.output_buffer[i+3] = io.control_out[i];

    		m_sendState = 1;
    		break;

    	case 1:

    		// C0
    		// 0 0 0 0 0 0 1 x     C1, C2, C3, C4 NCO Frequency in Hz for Transmitter, Apollo ATU
    		//                     (32 bit binary representation - MSB in C1)

    		io.output_buffer[3] = 0x2; // C0

    		if (io.tx_freq_change >= 0) {

    			io.output_buffer[4] = RX.at(io.tx_freq_change)->getFrequency() >> 24;
    		    io.output_buffer[5] = RX.at(io.tx_freq_change)->getFrequency() >> 16;
    		    io.output_buffer[6] = RX.at(io.tx_freq_change)->getFrequency() >> 8;
    		    io.output_buffer[7] = RX.at(io.tx_freq_change)->getFrequency();

    		    io.tx_freq_change = -1;
    		}

    		m_sendState = io.ccTx.duplex ? 2 : 3;
    		break;

    	case 2:

    		// C0 = 0 0 0 0 0 1 0 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver_1
    		// C0 = 0 0 0 0 0 1 1 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver _2
    		// C0 = 0 0 0 0 1 0 0 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver _3
    		// C0 = 0 0 0 0 1 0 1 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver _4
    		// C0 = 0 0 0 0 1 1 0 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver _5
    		// C0 = 0 0 0 0 1 1 1 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver _6
    		// C0 = 0 0 0 1 0 0 0 x     C1, C2, C3, C4   NCO Frequency in Hz for Receiver _7

    		if (io.rx_freq_change >= 0) {

    			io.output_buffer[3] = (io.rx_freq_change + 2) << 1;
    			io.output_buffer[4] = RX.at(io.rx_freq_change)->getFrequency() >> 24;
    			io.output_buffer[5] = RX.at(io.rx_freq_change)->getFrequency() >> 16;
    			io.output_buffer[6] = RX.at(io.rx_freq_change)->getFrequency() >> 8;
    			io.output_buffer[7] = RX.at(io.rx_freq_change)->getFrequency();

    			io.rx_freq_change = -1;
    		}

    		m_sendState = 3;
    		break;

    	case 3:

    		io.control_out[0] = 0x12; // 0 0 0 1 0 0 1 0
    		io.control_out[1] = 0x0; // C1
    		io.control_out[2] = 0x0; // C2
    		io.control_out[3] = 0x0; // C3
    		io.control_out[4] = 0x0; // C4

    		// C1
    		// 0 0 0 0 0 0 0 0
    		// |             |
    		// +-------------+------------ Hermes/PennyLane Drive Level (0-255) (ignored by Penelope)


    		// C2
    		// 0 0 0 0 0 0 0 0
    		// | | | | | | | |
    		// | | | | | | | +------------ Hermes/Metis Penelope Mic boost (0 = 0dB, 1 = 20dB)
    		// | | | | | | +-------------- Metis/Penelope or PennyLane Mic/Line-in (0 = mic, 1 = Line-in)
    		// | | | | | +---------------- Hermes – Enable/disable Apollo filter (0 = disable, 1 = enable)
    		// | | | | +------------------ Hermes – Enable/disable Apollo tuner (0 = disable, 1 = enable)
    		// | | | +-------------------- Hermes – Apollo auto tune (0 = end, 1 = start)
    		// | | +---------------------- Hermes – select filter board (0 = Alex, 1 = Apollo)
    		// | +------------------------ Alex   - manual HPF/LPF filter select (0 = disable, 1 = enable)
    		// +-------------------------- VNA mode (0 = off, 1 = on)

    		// Alex configuration:
    		//
    		// manual 		  0

    		io.control_out[2] &= 0xBF; // 1 0 1 1 1 1 1 1
    		io.control_out[2] |= (io.ccTx.alexConfig & 0x01) << 6;

    		// C3
    		// 0 0 0 0 0 0 0 0
    		//   | | | | | | |
    		//   | | | | | | +------------ Alex   -	select 13MHz  HPF (0 = disable, 1 = enable)*
    		//   | | | | | +-------------- Alex   -	select 20MHz  HPF (0 = disable, 1 = enable)*
    		//   | | | | +---------------- Alex   -	select 9.5MHz HPF (0 = disable, 1 = enable)*
    		//   | | | +------------------ Alex   -	select 6.5MHz HPF (0 = disable, 1 = enable)*
    		//   | | +-------------------- Alex   -	select 1.5MHz HPF (0 = disable, 1 = enable)*
    		//   | +---------------------- Alex   -	Bypass all HPFs   (0 = disable, 1 = enable)*
    		//   +------------------------ Alex   -	6M low noise amplifier (0 = disable, 1 = enable)*
    		//
    		// *Only valid when Alex - manual HPF/LPF filter select is enabled

    		io.control_out[3] &= 0xFE; // 1 1 1 1 1 1 1 0
    		// HPF 13 MHz: 1 0 0 0 0 0 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x40) >> 6;

    		io.control_out[3] &= 0xFD; // 1 1 1 1 1 1 0 1
    		// HPF 20 MHz: 1 0 0 0 0 0 0 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x80) >> 6;

    		io.control_out[3] &= 0xFB; // 1 1 1 1 1 0 1 1
    		// HPF 9.5 MHz: 1 0 0 0 0 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x20) >> 3;

    		io.control_out[3] &= 0xF7; // 1 1 1 1 0 1 1 1
    		// HPF 6.5 MHz: 1 0 0 0 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x10) >> 1;

    		io.control_out[3] &= 0xEF; // 1 1 1 0 1 1 1 1
    		// HPF 1.5 MHz: 1 0 0 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x08) << 1;

    		io.control_out[3] &= 0xDF; // 1 1 0 1 1 1 1 1
    		// bypass all: 1 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x02) << 4;

    		io.control_out[3] &= 0xBF; // 1 0 1 1 1 1 1 1
    		// 6m BPF/LNA: 1 0 0
    		io.control_out[3] |= (io.ccTx.alexConfig & 0x04) << 4;

    		io.control_out[3] &= 0x7F; // 0 1 1 1 1 1 1 1
    		io.control_out[3] |= ((int)io.ccTx.vnaMode) << 7;

    		// C4
    		// 0 0 0 0 0 0 0 0
    		//   | | | | | | |
    		//   | | | | | | +------------ Alex   - 	select 30/20m LPF (0 = disable, 1 = enable)*
    		//   | | | | | +-------------- Alex   - 	select 60/40m LPF (0 = disable, 1 = enable)*
    		//   | | | | +---------------- Alex   - 	select 80m    LPF (0 = disable, 1 = enable)*
    		//   | | | +------------------ Alex   - 	select 160m   LPF (0 = disable, 1 = enable)*
    		//   | | +-------------------- Alex   - 	select 6m     LPF (0 = disable, 1 = enable)*
    		//   | +---------------------- Alex   - 	select 12/10m LPF (0 = disable, 1 = enable)*
    		//   +------------------------ Alex   - 	select 17/15m LPF (0 = disable, 1 = enable)*
    		//
    		// *Only valid when Alex - manual HPF/LPF filter select is enabled

    		io.control_out[4] &= 0xFE; // 1 1 1 1 1 1 1 0
    		// LPF 30/20m: 1 0 0 0 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x800) >> 11;

    		io.control_out[4] &= 0xFD; // 1 1 1 1 1 1 0 1
    		// LPF 60/40m: 1 0 0 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x400) >> 9;

    		io.control_out[4] &= 0xFB; // 1 1 1 1 1 0 1 1
    		// LPF 80m: 1 0 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x200) >> 7;

    		io.control_out[4] &= 0xF7; // 1 1 1 1 0 1 1 1
    		// LPF 160m: 1 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x100) >> 5;

    		io.control_out[4] &= 0xEF; // 1 1 1 0 1 1 1 1
    		// LPF 6m: 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x4000) >> 10;

    		io.control_out[4] &= 0xDF; // 1 1 0 1 1 1 1 1
    		// LPF 12/10m : 1 0 0 0 0 0 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x2000) >> 8;

    		io.control_out[4] &= 0xBF; // 1 0 1 1 1 1 1 1
    		// LPF 17/15m: 1 0 0 0 0 0 0 0 0 0 0 0 0
    		io.control_out[4] |= (io.ccTx.alexConfig & 0x1000) >> 6;

    		// fill the out buffer with the C&C bytes
    		for (int i = 0; i < 5; i++)
    			io.output_buffer[i+3] = io.control_out[i];

    		// round finished
    		m_sendState = 0;
    		break;
    }
    io.mutex.unlock();


	switch (m_hwInterface) {

		case QSDR::Metis:
		case QSDR::Hermes:

			io.audioDatagram.resize(IO_BUFFER_SIZE);
			//io.audioDatagram = QByteArray(reinterpret_cast<const char*>(&io.output_buffer), sizeof(io.output_buffer));
			io.audioDatagram = QByteArray::fromRawData((const char *)&io.output_buffer, IO_BUFFER_SIZE);
			
			//m_audioProcessor->deviceWriteBuffer();
			emit audioDataReady();
			break;
			
		case QSDR::NoInterfaceMode:
			break;
	}
}

void DataEngine::dttspProcessing() {

	for (int r = 0; r < io.receivers; r++) {
		
		if (RX.at(r)->getConnectedStatus()) {

			io.mutex.lock();
			RX.at(r)->dttsp->dttspAudioCallback(
					RX.at(r)->in, &RX.at(r)->in[BUFFER_SIZE],
					RX.at(r)->out, &RX.at(r)->out[BUFFER_SIZE],
					BUFFER_SIZE, 0);

			RX.at(r)->dttsp->dttspProcessPanadapter(0, RX.at(r)->spectrum);
			io.mutex.unlock();

			if (RX.at(r)->highResTimer->getElapsedTimeInMicroSec() >= RX.at(r)->getDisplayDelay()) {

				settings->setSpectrumBuffer(r, RX.at(r)->spectrum);
				RX.at(r)->highResTimer->start();
			}

			//io.mutex.unlock();

			if (r == io.currentReceiver) {

				if (m_smeterTime.elapsed() > 20) {

					m_sMeterValue = RX.at(r)->dttsp->dttspCalculateRXMeter(0, 0, m_meterType);
					settings->setSMeterValue(r, m_sMeterValue);
					m_smeterTime.restart();
				}

				processOutputBuffer(RX.at(r)->out, &RX.at(r)->out[BUFFER_SIZE]);
			}
		}
	}
}

void DataEngine::qtdspProcessing() {

	for (int r = 0; r < io.receivers; r++) {

		if (RX.at(r)->getConnectedStatus()) {

			// do the DSP processing
			io.mutex.lock();
			RX.at(r)->qtdsp->processDSP(RX.at(r)->inBuf, RX.at(r)->outBuf, BUFFER_SIZE);
			io.mutex.unlock();

			// spectrum
			RX.at(r)->qtdsp->getSpectrum(RX.at(r)->spectrum);

			if (RX.at(r)->highResTimer->getElapsedTimeInMicroSec() >= RX.at(r)->getDisplayDelay()) {

				settings->setSpectrumBuffer(r, RX.at(r)->spectrum);
				RX.at(r)->highResTimer->start();
			}

			if (r == io.currentReceiver) {

				if (m_smeterTime.elapsed() > 20) {

					m_sMeterValue = RX.at(r)->qtdsp->getSMeterInstValue();
					settings->setSMeterValue(r, m_sMeterValue);
					m_smeterTime.restart();
				}

				processOutputBuffer(RX.at(r)->outBuf);
			}
		}
	}
}

 
//*****************************************************************************
//

void DataEngine::setSystemState(
	QObject *sender, 
	QSDR::_Error err, 
	QSDR::_HWInterfaceMode hwmode, 
	QSDR::_ServerMode mode, 
	QSDR::_DataEngineState state)
{
	Q_UNUSED (sender)
	Q_UNUSED (err)

	io.mutex.lock();
	if (m_hwInterface != hwmode)
		m_hwInterface = hwmode;
		
	if (m_serverMode != mode)
		m_serverMode = mode;
		
	if (m_dataEngineState != state)
		m_dataEngineState = state;

	io.mutex.unlock();
}

float DataEngine::getFilterSizeCalibrationOffset() {

    //int size=1024; // dspBufferSize
    float i = log10((qreal) BUFFER_SIZE);
    return 3.0f*(11.0f - i);
}

void DataEngine::sendMessage(QString msg) {

	msg.prepend("[data engine]: ");

	emit messageEvent(msg);
}

void DataEngine::sendReceiverMessage(QString msg) {

	emit messageEvent(msg);
}

void DataEngine::newMessage(QString msg) {

	emit messageEvent(msg);
}

void DataEngine::searchHpsdrNetworkDevices() {

	if (!m_hpsdrIO) createHpsdrIO();

	// HPSDR network IO thread
	if (!startHpsdrIO(QThread::NormalPriority)) {

		DATA_ENGINE_DEBUG << "HPSDR network discovery thread could not be started.";
		sendMessage("HPSDR network discover thread could not be started.");
		return;
	}

	io.networkIOMutex.lock();
	io.devicefound.wait(&io.networkIOMutex);

	//m_hpsdrIO->findHPSDRDevices();

	// stop the discovery thread
	io.networkIOMutex.unlock();
	stopHpsdrIO();
}

void DataEngine::setHPSDRDeviceNumber(int value) {

	m_hpsdrDevices = value;
}

void DataEngine::rxListChanged(QList<Receiver *> list) {

	io.mutex.lock();
	RX = list;
	io.mutex.unlock();
}

void DataEngine::setCurrentReceiver(QObject *sender, int rx) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.currentReceiver = rx;
	io.mutex.unlock();
}

void DataEngine::setFramesPerSecond(QObject *sender, int rx, int value) {

	Q_UNUSED(sender)

	/*io.mutex.lock();
	if (m_fpsList.length() > 0)
		m_fpsList[rx] = (int)(1000000.0/value);
	io.mutex.unlock();*/
}

void DataEngine::setSampleRate(QObject *sender, int value) {

	Q_UNUSED(sender)

	io.mutex.lock();
	switch (value) {
	
		case 48000:
			io.samplerate = value;
			io.speed = 0;
			io.outputMultiplier = 1;
			break;
			
		case 96000:
			io.samplerate = value;
			io.speed = 1;
			io.outputMultiplier = 2;
			break;
			
		case 192000:
			io.samplerate = value;
			io.speed = 2;
			io.outputMultiplier = 4;
			break;
			
		default:
			sendMessage("invalid sample rate (48000, 96000, 192000)!\n");
			exit(1);
			break;
	}

	if ((m_serverMode == QSDR::DttSP || 
		 m_serverMode == QSDR::ChirpWSPR) && 
		 m_dataEngineState == QSDR::DataEngineUp)
	{	
		int rx = io.currentReceiver;
		if (RX.at(rx)->dttsp->getDttSPStatus()) {

			RX.at(rx)->dttsp->dttspSetSampleRate((double)io.samplerate);
			RX.at(rx)->dttsp->dttspSetRXOsc(0, 0, 0.0);
			RX.at(rx)->dttsp->dttspSetRXOutputGain(0, 0, settings->getMainVolume(io.currentReceiver));

			HamBand band = RX.at(rx)->getHamBand();
			DSPMode mode = RX.at(rx)->getDSPModeList().at(band);
			setDttspDspMode(this, rx, mode);
			setDttspAgcMode(this, rx, RX.at(rx)->getAGCMode());

			setFrequency(this, true, rx, RX.at(rx)->getFrequency());
		}
		/*
		if (dttSPList[io.currentReceiver]->getDttSPStatus()) {
			
			dttSPList[io.currentReceiver]->dttspSetSampleRate((double)io.samplerate);
			dttSPList[io.currentReceiver]->dttspSetRXOsc(0, 0, 0.0);
			dttSPList[io.currentReceiver]->dttspSetRXOutputGain(0, 0, settings->getMainVolume(io.currentReceiver));
		
			HamBand band = RX.at(io.currentReceiver)->getHamBand();
			DSPMode mode = RX.at(io.currentReceiver)->getDSPModeList().at(band);
			setDttspDspMode(this, io.currentReceiver, mode);
			setDttspAgcMode(this, io.currentReceiver, RX.at(io.currentReceiver)->getAGCMode());
			//setDttspFrequency(this, true, io.currentReceiver, RX.at(io.currentReceiver)->getFrequency());
			setFrequency(this, true, io.currentReceiver, RX.at(io.currentReceiver)->getFrequency());
		}
		*/
	}
	io.mutex.unlock();

	emit outMultiplierEvent(io.outputMultiplier);
}

void DataEngine::setMercuryAttenuator(QObject *sender, HamBand band, int value) {

	Q_UNUSED(sender)
	Q_UNUSED(band)

	io.mutex.lock();
	io.ccTx.mercuryAttenuator = value;
	io.mutex.unlock();
}

void DataEngine::setMercuryAttenuators(QObject *sender, QList<int> attn) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.ccTx.mercuryAttenuators = attn;
	io.mutex.unlock();
}

void DataEngine::setDither(QObject *sender, int value) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.ccTx.dither = value;
	io.mutex.unlock();
}

void DataEngine::setRandom(QObject *sender, int value) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.ccTx.random = value;
	io.mutex.unlock();
}

void DataEngine::set10MhzSource(QObject *sender, int source) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.control_out[1] = io.control_out[1] & 0xF3;
	io.control_out[1] = io.control_out[1] | (source << 2);
	io.mutex.unlock();
}

void DataEngine::set122_88MhzSource(QObject *sender, int source) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.control_out[1] = io.control_out[1] & 0xEF;
	io.control_out[1] = io.control_out[1] | (source << 4);
	io.mutex.unlock();
}

void DataEngine::setMicSource(QObject *sender, int source) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.control_out[1] = io.control_out[1] & 0x7F;
	io.control_out[1] = io.control_out[1] | (source << 7);
	io.mutex.unlock();
}

void DataEngine::setMercuryClass(QObject *sender, int value) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.rxClass = value;
	io.mutex.unlock();
}

void DataEngine::setMercuryTiming(QObject *sender, int value) {

	Q_UNUSED(sender)

	io.mutex.lock();
	io.timing = value;
	io.mutex.unlock();
}

void DataEngine::setAlexConfiguration(quint16 conf) {

	io.mutex.lock();
	io.ccTx.alexConfig = conf;
	DATA_ENGINE_DEBUG << "Alex Configuration = " << io.ccTx.alexConfig;
	io.mutex.unlock();
}

void DataEngine::setAlexStates(HamBand band, const QList<int> &states) {

	Q_UNUSED (band)

	io.mutex.lock();
	io.ccTx.alexStates = states;
	DATA_ENGINE_DEBUG << "Alex States = " << io.ccTx.alexStates;
	io.mutex.unlock();
}

void DataEngine::setPennyOCEnabled(bool value) {

	io.mutex.lock();
	io.ccTx.pennyOCenabled = value;
	io.mutex.unlock();
}

void DataEngine::setRxJ6Pins(const QList<int> &list) {

	io.mutex.lock();
	io.ccTx.rxJ6pinList = list;
	io.mutex.unlock();

}

void DataEngine::setTxJ6Pins(const QList<int> &list) {

	io.mutex.lock();
	io.ccTx.txJ6pinList = list;
	io.mutex.unlock();
}

void DataEngine::setRcveIQSignal(QObject *sender, int value) {

	emit rcveIQEvent(sender, value);
}

void DataEngine::setPenelopeVersion(QObject *sender, int version) {

	emit penelopeVersionInfoEvent(sender, version);
}

void DataEngine::setHwIOVersion(QObject *sender, int version) {

	emit hwIOVersionInfoEvent(sender, version);
}

void DataEngine::setNumberOfRx(QObject *sender, int value) {

	Q_UNUSED(sender)

	if (io.receivers == value) return;
	
	io.mutex.lock();
	io.receivers = value;
	io.mutex.unlock();
	//io.control_out[4] &= 0xc7;
	//io.control_out[4] |= (value - 1) << 3;

	m_message = "number of receivers set to %1.";
	sendMessage(m_message.arg(QString::number(value)));
}

void DataEngine::setTimeStamp(QObject *sender, bool value) {

	Q_UNUSED(sender)

	if (io.timeStamp == value) return;

	io.mutex.lock();
	io.timeStamp = value;
	io.mutex.unlock();
	//io.control_out[4] &= 0xc7;
	io.control_out[4] |= value << 6;

	if (value)
		m_message = "set time stamp on.";
	else
		m_message = "set time stamp off.";
	sendMessage(m_message);
}

void DataEngine::setRxSocketState(int rx, const char* prop, QString str) {

	RX[rx]->setProperty(prop, str);
	settings->setRxList(RX);
}

void DataEngine::setRxPeerAddress(int rx, QHostAddress address) {

	RX[rx]->setPeerAddress(address);
	settings->setRxList(RX);
}

void DataEngine::setRx(int rx) {

	io.mutex.lock();
	RX[rx]->setReceiver(rx);
	settings->setRxList(RX);
	io.mutex.unlock();
}

void DataEngine::setRxClient(int rx, int client) {

	io.mutex.lock();
	RX[rx]->setClient(client);
	settings->setRxList(RX);
	io.mutex.unlock();
}

void DataEngine::setClientConnected(QObject* sender, int rx) {

	Q_UNUSED(sender)

	if (!io.clientList.contains(rx)) {

		io.clientList.append(rx);
		io.audio_rx = rx;

		m_AudioRcvrThread->quit();
		m_AudioRcvrThread->wait();
		m_AudioRcvrThread->start();
	}
	else {

		io.sendIQ_toggle = true;
		io.rcveIQ_toggle = false;
		m_AudioRcvrThread->start();
	}
}

void DataEngine::setClientConnected(bool value) {

	clientConnected = value;
}

void DataEngine::setClientDisconnected(int client) {

	Q_UNUSED(client)
	/*if (clientConnected) {

		m_AudioRcvrThread->quit();
		m_AudioRcvrThread->wait();
		if (!m_AudioRcvrThread->isRunning())
			DATA_ENGINE_DEBUG << "audio receiver thread stopped.";

		clientConnected = false;		
	}
	sync_toggle = true;
	adc_toggle = false;*/
}

void DataEngine::setAudioProcessorRunning(bool value) {

	m_audioProcessorRunning = value;
}

void DataEngine::setAudioReceiver(QObject *sender, int rx) {

	Q_UNUSED(sender)

	io.mutex.lock();
	emit audioRxEvent(rx);
	io.mutex.unlock();
}

void DataEngine::setIQPort(int rx, int port) {

	io.mutex.lock();
	RX[rx]->setIQPort(port);
	settings->setRxList(RX);
	io.mutex.unlock();
}

void DataEngine::setRxConnectedStatus(QObject* sender, int rx, bool value) {

	Q_UNUSED(sender)

	io.mutex.lock();
	RX[rx]->setConnectedStatus(value);
	settings->setRxList(RX);
	io.mutex.unlock();
}

void DataEngine::setHamBand(QObject *sender, int rx, bool byBtn, HamBand band) {

	Q_UNUSED(sender)
	Q_UNUSED(rx)
	Q_UNUSED(byBtn)

	io.mutex.lock();
	io.ccTx.currentBand = band;
	io.mutex.unlock();
}

void DataEngine::setFrequency(QObject* sender, bool value, int rx, long frequency) {

	Q_UNUSED(sender)
	Q_UNUSED(value)

	//RX[rx]->frequency_changed = value;
	RX[rx]->setFrequency(frequency);
	io.rx_freq_change = rx;
	io.tx_freq_change = rx;
}

void DataEngine::setRXFilter(QObject *sender, int rx, qreal low, qreal high) {

	Q_UNUSED(sender)

	io.mutex.lock();
	RX.at(rx)->qtdsp->filter->setFilter((float)low, (float)high);
	io.mutex.unlock();
}

void DataEngine::setDspMode(QObject *sender, int rx, DSPMode mode) {

	Q_UNUSED(sender)

	RX.at(rx)->qtdsp->setDSPMode(mode);
}

void DataEngine::setAgcMode(QObject *sender, int rx, AGCMode mode) {

	Q_UNUSED(sender)

	RX.at(rx)->qtdsp->setAGCMode(mode);

}

void DataEngine::setAgcGain(QObject *sender, int rx, int value) {

	Q_UNUSED(sender)

	RX[rx]->setAGCGain(this, rx, value);
	//qtdspList[rx]->
}

void DataEngine::setWbSpectrumAveraging(bool value) {

	m_wbMutex.lock();
	m_wbSpectrumAveraging = value;
	m_wbMutex.unlock();
}

 
//**************************************************
// DttSP control
void DataEngine::setDttSPMainVolume(QObject *sender, int rx, float value) {

	Q_UNUSED(sender)

	switch (m_serverMode) {

		case QSDR::DttSP:
		case QSDR::ChirpWSPR:

			if (RX.at(rx)->dttsp->getDttSPStatus())
				RX.at(rx)->dttsp->dttspSetRXOutputGain(0, 0, value);

			break;
			
		case QSDR::NoServerMode:
		case QSDR::ExternalDSP:
		case QSDR::QtDSP:
		case QSDR::DemoMode:
		case QSDR::ChirpWSPRFile:
			break;
	}
}

void DataEngine::setDttspRXFilter(QObject *sender, int rx, qreal low, qreal high) {

	Q_UNUSED(sender)

	if (RX.at(rx)->dttsp)
		RX.at(rx)->dttsp->dttspSetRXFilter(0, 0, (double) low, (double) high);
}

void DataEngine::setDttspDspMode(QObject *sender, int rx, DSPMode mode) {

	Q_UNUSED(sender)

	if (RX.at(rx)->dttsp) {

		RX.at(rx)->dttsp->dttspSetMode(0, 0, mode);
		setDttspRXFilter(this, rx,
				getFilterFromDSPMode(settings->getDefaultFilterList(), mode).filterLo,
				getFilterFromDSPMode(settings->getDefaultFilterList(), mode).filterHi);
	}
}

void DataEngine::setDttspAgcMode(QObject *sender, int rx, AGCMode mode) {

	Q_UNUSED(sender)

	if (RX.at(rx)->dttsp)
		RX.at(rx)->dttsp->dttspSetRXAGC(0, 0, mode);
}

void DataEngine::setDttspAgcGain(QObject *sender, int rx, int value) {

	Q_UNUSED(sender)

	if (RX.at(rx)->dttsp)
		RX.at(rx)->dttsp->dttspSetRXAGCTop(0, 0, (double) (value * 1.0));
}

//**************************************************
// QtDSP control
void DataEngine::setQtDSPMainVolume(QObject *sender, int rx, float value) {

	if (RX.at(rx)->qtdsp)
		RX.at(rx)->qtdsp->setVolume(value);
}

//***************************************************

void DataEngine::loadWavFile(const QString &fileName) {

	if (m_audioEngine->loadFile(fileName))
		m_soundFileLoaded = true;
	else
		m_soundFileLoaded = false;
}

void DataEngine::suspend() {

	m_audioEngine->suspend();
}

void DataEngine::startPlayback() {

	m_audioEngine->startPlayback();
}

void DataEngine::showSettingsDialog() {

	m_audioEngine->showSettingsDialog();
}

void DataEngine::setAudioFileFormat(QObject *sender, const QAudioFormat &format) {

	Q_UNUSED (sender)
	Q_UNUSED (format)
}

void DataEngine::setAudioFilePosition(QObject *sender, qint64 position) {

	Q_UNUSED (sender)
	Q_UNUSED (position)
}

void DataEngine::setAudioFileBuffer(QObject *sender, qint64 position, qint64 length, const QByteArray &buffer) {

	Q_UNUSED (sender)

    m_audioFileBufferPosition = position;
    m_audioFileBufferLength = length;
	m_audioFileBuffer = buffer;

	//DATA_ENGINE_DEBUG << "audio file length" << m_audioFileBufferLength;
}

void DataEngine::setAudioFileBuffer(const QList<qreal> &buffer) {

	io.inputBuffer = buffer;
	
	/*for (int i = 0; i < buffer.length(); i++) {

		DATA_ENGINE_DEBUG << "i" << i << "audioBuffer" << io.inputBuffer.at(i);
	}*/
}
 
// *********************************************************************
// Data processor

DataProcessor::DataProcessor(DataEngine *de, QSDR::_ServerMode serverMode)
	: QObject()
	, m_dataEngine(de)
	, m_dataProcessorSocket(0)
	, m_serverMode(serverMode)
	, m_socketConnected(false)
	, m_bytes(0)
	, m_offset(0)
	, m_length(0)
	, m_stopped(false)
{
	m_IQSequence = 0L;
	m_sequenceHi = 0L;
	
	m_IQDatagram.resize(0);
}


DataProcessor::~DataProcessor() {
}

void DataProcessor::stop() {

	m_stopped = true;
}

void DataProcessor::initDataProcessorSocket() {

	m_dataProcessorSocket = new QUdpSocket();

	/*m_dataProcessorSocket->bind(QHostAddress(set->getHPSDRDeviceLocalAddr()),
								  23000, 
								  QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress);

	int newBufferSize = 64 * 1024;

	if (::setsockopt(m_dataProcessorSocket->socketDescriptor(), SOL_SOCKET,
                     SO_RCVBUF, (char *)&newBufferSize, sizeof(newBufferSize)) == -1) {
						 
						 DATA_ENGINE_DEBUG << "initDataProcessorSocket error setting m_dataProcessorSocket buffer size.";
	}*/

	//m_dataProcessorSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
	//m_dataProcessorSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

	CHECKED_CONNECT(
		m_dataProcessorSocket, 
		SIGNAL(error(QAbstractSocket::SocketError)), 
		this, 
		SLOT(displayDataProcessorSocketError(QAbstractSocket::SocketError)));
}


void DataProcessor::displayDataProcessorSocketError(QAbstractSocket::SocketError error) {

	DATA_PROCESSOR_DEBUG << "displayDataProcessorSocketError data processor socket error:" << error;
}


void DataProcessor::processDeviceData() {

	if (m_serverMode == QSDR::ExternalDSP) 
		initDataProcessorSocket();

	forever {

		m_dataEngine->processInputBuffer(m_dataEngine->io.iq_queue.dequeue());
		//DATA_ENGINE_DEBUG << "IQ queue length:" << m_dataEngine->io.iq_queue.count();
		//DATA_ENGINE_DEBUG << "iq_queue length:" << m_dataEngine->io.iq_queue.dequeue().length();
		
		m_mutex.lock();
		if (m_stopped) {
			m_stopped = false;
			m_mutex.unlock();
			break;
		}
		m_mutex.unlock();
	}

	if (m_serverMode == QSDR::ExternalDSP) {

		disconnect(this);
		m_dataProcessorSocket->close();
		delete m_dataProcessorSocket;
		m_dataProcessorSocket = NULL;

		m_socketConnected = false;
	}
}


void DataProcessor::processData() {

	forever {

		m_dataEngine->processFileBuffer(m_dataEngine->io.data_queue.dequeue());

		m_mutex.lock();
		if (m_stopped) {
			m_stopped = false;
			m_mutex.unlock();
			break;
		}
		m_mutex.unlock();
	}
}

void DataProcessor::externalDspProcessing(int rx) {

	// keep UDP packets < 512 bytes 
	// 8 bytes sequency number, 2 bytes offset, 2 bytes length, 500 bytes data

	if (!m_socketConnected) {

		m_dataProcessorSocket->connectToHost(m_dataEngine->RX[rx]->getPeerAddress(), m_dataEngine->RX[rx]->getIQPort());
		//int newBufferSize = 64 * 1024;
		int newBufferSize = 16 * 1024;

		if (::setsockopt(m_dataProcessorSocket->socketDescriptor(), SOL_SOCKET,
                     SO_RCVBUF, (char *)&newBufferSize, sizeof(newBufferSize)) == -1) {
						 
						 DATA_PROCESSOR_DEBUG << "externalDspProcessing error setting m_dataProcessorSocket buffer size.";
		}
		m_socketConnected = true;
	}
	
#ifndef __linux__
	m_sequenceHi = 0L;
#endif
	
	/*QUdpSocket socket;
	CHECKED_CONNECT(&socket, 
			SIGNAL(error(QAbstractSocket::SocketError)), 
			this, 
			SLOT(displayDataProcessorSocketError(QAbstractSocket::SocketError)));*/

	m_offset = 0;
	//m_IQDatagram.append(reinterpret_cast<const char*>(&m_dataEngine->rxList[rx]->input_buffer), sizeof(m_dataEngine->rxList[rx]->input_buffer));
	m_IQDatagram.append(
		reinterpret_cast<const char*>(&m_dataEngine->RX[rx]->inBuf),
		sizeof(m_dataEngine->RX[rx]->inBuf));

	m_IQDatagram.append(
		reinterpret_cast<const char*>(&m_dataEngine->RX[rx]->inBuf),
		sizeof(m_dataEngine->RX[rx]->inBuf));
		
	while (m_offset < m_IQDatagram.size()) {
	
		m_length = m_IQDatagram.size() - m_offset;
		
		if (m_length > 500)  
			m_length = 500;

		QByteArray datagram;
		datagram += QByteArray(reinterpret_cast<const char*>(&m_IQSequence), sizeof(m_IQSequence));
		datagram += QByteArray(reinterpret_cast<const char*>(&m_sequenceHi), sizeof(m_sequenceHi));
		datagram += QByteArray(reinterpret_cast<const char*>(&m_offset), sizeof(m_offset));
		datagram += QByteArray(reinterpret_cast<const char*>(&m_length), sizeof(m_length));
		datagram += m_IQDatagram.mid(m_offset, m_length);
		
		if (m_dataProcessorSocket->write(datagram) < 0)
		/*if (m_dataProcessorSocket->writeDatagram(datagram,
											m_dataEngine->rxList[rx]->getPeerAddress(),
											m_dataEngine->rxList[rx]->getIQPort()) < 0)*/
		//if (socket.writeDatagram(datagram,
		//						 m_dataEngine->rxList[rx]->getPeerAddress(),
		//						 m_dataEngine->rxList[rx]->getIQPort()) < 0)
		{
			if (!m_dataEngine->io.sendIQ_toggle) {  // toggles the sendIQ signal

				m_dataEngine->settings->setSendIQ(2);
				m_dataEngine->io.sendIQ_toggle = true;
			}

			DATA_ENGINE_DEBUG	<< "externalDspProcessing error sending data to client:" 
								<< m_dataProcessorSocket->errorString();
		}
		else {
		
			//socket.flush();
			if (m_dataEngine->io.sendIQ_toggle) { // toggles the sendIQ signal
				
				m_dataEngine->settings->setSendIQ(1);
				m_dataEngine->io.sendIQ_toggle = false;
			}
		}
		m_offset += m_length;
	}
	m_IQDatagram.resize(0);
	m_IQSequence++;
}


void DataProcessor::externalDspProcessingBig(int rx) {
	
	m_IQDatagram.append(
		reinterpret_cast<const char*>(&m_dataEngine->RX[rx]->in), sizeof(m_dataEngine->RX[rx]->in));
		
	if (m_dataProcessorSocket->writeDatagram(m_IQDatagram.data(), 
										m_IQDatagram.size(), 
										m_dataEngine->RX[rx]->getPeerAddress(),
										m_dataEngine->RX[rx]->getIQPort()) < 0)
										
	{		
		if (!m_dataEngine->io.sendIQ_toggle) {  // toggles the sendIQ signal

			m_dataEngine->settings->setSendIQ(2);
			m_dataEngine->io.sendIQ_toggle = true;
		}

		DATA_PROCESSOR_DEBUG << "error sending data to client:" << m_dataProcessorSocket->errorString();
	}
	else {
	
		m_dataProcessorSocket->flush();
		if (m_dataEngine->io.sendIQ_toggle) { // toggles the sendIQ signal
				
			m_dataEngine->settings->setSendIQ(1);
			m_dataEngine->io.sendIQ_toggle = false;
		}
	}
	m_IQDatagram.resize(0);
}


// *********************************************************************
// Wide band data processor
 
WideBandDataProcessor::WideBandDataProcessor(DataEngine *de, QSDR::_ServerMode serverMode)
	: QObject()
	, m_dataEngine(de)
	, m_serverMode(serverMode)
	, m_bytes(0)
	, m_stopped(false)
{
	m_WBDatagram.resize(0);
}


WideBandDataProcessor::~WideBandDataProcessor() {

}


void WideBandDataProcessor::stop() {

	//mutex.lock();
	m_stopped = true;
	//mutex.unlock();
}


void WideBandDataProcessor::processWideBandData() {

	forever {

		m_dataEngine->processWideBandInputBuffer(m_dataEngine->io.wb_queue.dequeue());
		
		m_mutex.lock();
		if (m_stopped) {
			m_stopped = false;
			m_mutex.unlock();
			break;
		}
		m_mutex.unlock();
	}
}

 
// *********************************************************************
// Audio processor

AudioProcessor::AudioProcessor(DataEngine *de)
	: QObject()
	, m_dataEngine(de)
	, m_audioProcessorSocket(0)
	, m_setNetworkDeviceHeader(true)
	, m_stopped(false)
	, m_client(0)
	, m_audioRx(0)
{
	//m_stopped = false;
	//m_setNetworkDeviceHeader = true;
	
	m_sendSequence = 0L;
	m_oldSendSequence = 0L;
	
	m_deviceSendDataSignature.resize(4);
	m_deviceSendDataSignature[0] = (char)0xEF;
	m_deviceSendDataSignature[1] = (char)0xFE;
	m_deviceSendDataSignature[2] = (char)0x01;
	m_deviceSendDataSignature[3] = (char)0x02;
}


AudioProcessor::~AudioProcessor() {
}


void AudioProcessor::stop() {

	m_stopped = true;
}


void AudioProcessor::initAudioProcessorSocket() {

	m_audioProcessorSocket = new QUdpSocket();
	m_audioProcessorSocket->connectToHost(m_dataEngine->io.hpsdrDeviceIPAddress, METIS_PORT);

	//m_audioProcessorSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
	//m_audioProcessorSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

	CHECKED_CONNECT(
		m_audioProcessorSocket, 
		SIGNAL(error(QAbstractSocket::SocketError)), 
		this, 
		SLOT(displayAudioProcessorSocketError(QAbstractSocket::SocketError)));

	m_dataEngine->setAudioProcessorRunning(true);
}


void AudioProcessor::audioReceiverChanged(int rx) {

	m_audioRx = rx;
}


void AudioProcessor::clientConnected(int rx) {

	m_mutex.lock();
	m_client = rx;
	m_mutex.unlock();
}


void AudioProcessor::displayAudioProcessorSocketError(QAbstractSocket::SocketError error) {

	AUDIO_PROCESSOR << "audio processor socket error:" << error;
}


void AudioProcessor::processAudioData() {

	initAudioProcessorSocket();

	forever {
		
		//DATA_ENGINE_DEBUG << "audioQueue length = " << m_dataEngine->io.au_queue.length();
		m_outBuffer = m_dataEngine->io.au_queue.dequeue();
		
		m_mutex.lock();
		if (m_stopped) {
			m_stopped = false;
			m_mutex.unlock();
			break;
		}
		m_mutex.unlock();

		memcpy(
			(float *) &m_left[0], 
			(float *) m_outBuffer.left(IO_AUDIOBUFFER_SIZE/2).data(), 
			BUFFER_SIZE * sizeof(float));

		memcpy(
			(float *) &m_right[0], 
			(float *) m_outBuffer.right(IO_AUDIOBUFFER_SIZE/2).data(), 
			BUFFER_SIZE * sizeof(float));
		
		m_dataEngine->processOutputBuffer(m_left, m_right);
	}


	disconnect(this);
	m_audioProcessorSocket->close();
	delete m_audioProcessorSocket;
	m_audioProcessorSocket = NULL;
}


void AudioProcessor::deviceWriteBuffer() {

	if (m_setNetworkDeviceHeader) {

		m_outDatagram.resize(0);
        m_outDatagram += m_deviceSendDataSignature;

		QByteArray seq(reinterpret_cast<const char*>(&m_sendSequence), sizeof(m_sendSequence));

		m_outDatagram += seq;
		m_outDatagram += m_dataEngine->io.audioDatagram;

		m_sendSequence++;
        m_setNetworkDeviceHeader = false;
    } 
	else {

		m_outDatagram += m_dataEngine->io.audioDatagram;

		//if (m_audioProcessorSocket->writeDatagram(outDatagram.data(), outDatagram.size(), m_dataEngine->metisIPAddress, m_dataEngine->metisDataPort) < 0)
		if (m_audioProcessorSocket->write(m_outDatagram) < 0)
			AUDIO_PROCESSOR << "error sending data to Metis:" << m_audioProcessorSocket->errorString();

		if (m_sendSequence != m_oldSendSequence + 1)
			AUDIO_PROCESSOR << "output sequence error: old =" << m_oldSendSequence << "; new =" << m_sendSequence; 

		m_oldSendSequence = m_sendSequence;
		m_setNetworkDeviceHeader = true;
    }
}
 
