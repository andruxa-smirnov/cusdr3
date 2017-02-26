
//#include "cusdr_audioThread.h"
#include "cusdr_audioEngine.h"

bool endOfMusic;

void __stdcall syncFunc(HSYNC handle, DWORD channel, DWORD data, void *user) {

	Q_UNUSED(user);
	Q_UNUSED(data);

    BASS_ChannelRemoveSync(channel, handle);
    BASS_ChannelStop(channel);
    qDebug() << "End of playback!";
    endOfMusic = true;
}

AudioEngine::AudioEngine(QObject *parent)
	: QObject(parent)
	//:QThread(parent)
{
    if (!BASS_Init(-1, 44100, 0, NULL, NULL))
        qDebug() << "Cannot initialize device";
    t = new QTimer(this);
    connect(t, SIGNAL(timeout()), this, SLOT(signalUpdate()));
    endOfMusic = true;
}

void AudioEngine::play(QString filename) {

    BASS_ChannelStop(chan);
    if (!(chan = BASS_StreamCreateFile(false, filename.toLatin1(), 0, 0, BASS_SAMPLE_LOOP))
        && !(chan = BASS_MusicLoad(false, filename.toLatin1(), 0, 0, BASS_MUSIC_RAMP | BASS_SAMPLE_LOOP, 1)))
            qDebug() << "Can't play file";
    else {

        endOfMusic = false;
        BASS_ChannelPlay(chan, true);
        t->start(100);
        BASS_ChannelSetSync(chan, BASS_SYNC_END, 0, &syncFunc, 0);
        playing = true;
    }
}

void AudioEngine::startPlayback() {
}

void AudioEngine::pause() {

    BASS_ChannelPause(chan);
    t->stop();
    playing = false;
}

void AudioEngine::resume() {

    if (!BASS_ChannelPlay(chan, false)) {

        qDebug() << "Error resuming";
	}
    else {

        t->start(98);
        playing = true;
    }
}

void AudioEngine::stop() {

    BASS_ChannelStop(chan);
    t->stop();
    playing = false;
}

void AudioEngine::reset() {

    /*stopRecording();
    stopPlayback();
    setAudioState(QAudio::AudioInput, QAudio::StoppedState);
    setFormat(QAudioFormat());
    m_generateTone = false;
    delete m_file;
    m_file = 0;
    delete m_analysisFile;
    m_analysisFile = 0;
    m_buffer.clear();
	m_bufferPosition = 0;
    m_bufferLength = 0;
	m_spectrumBuffer.clear();
	m_spectrumBufferLength = 0;
    m_dataLength = 0;
    emit dataLengthChanged(0);
    resetAudioDevices();*/
}

void AudioEngine::signalUpdate() {

    if (endOfMusic == false) {

        emit curPos(BASS_ChannelBytes2Seconds(chan, BASS_ChannelGetPosition(chan, BASS_POS_BYTE)),
                    BASS_ChannelBytes2Seconds(chan, BASS_ChannelGetLength(chan, BASS_POS_BYTE)));
    }
    else {

        emit endOfPlayback();
        playing = false;
    }
}

void AudioEngine::changePosition(int position) {

    BASS_ChannelSetPosition(chan, BASS_ChannelSeconds2Bytes(chan, position), BASS_POS_BYTE);
}

void AudioEngine::run() {

    while (1);
}

bool AudioEngine::loadAudioFile(const QString &fileName) {

	Q_UNUSED(fileName);

    reset();
    bool result = false;
	/*
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    Q_ASSERT(!fileName.isEmpty());
    m_file = new WavFile(this);

    if (m_file->open(fileName)) {

		QString formatStr = formatToString(m_file->fileFormat());
		AUDIOENGINE_DEBUG << "file audio format" << formatStr;


        if (isPCMS16LE(m_file->fileFormat())) {

            result = initializePCMS16LE();

			m_audioFileBuffer = new AudiofileBuffer();
			m_audioFileBuffer = AudiofileBuffer::loadWav(m_file->fileName());

			qint16 channels = m_audioFileBuffer->getNofChannels();

			AUDIO_ENGINE_DEBUG << "  data length:" << m_audioFileBuffer->getDataLength();
			AUDIO_ENGINE_DEBUG << "  bytes per sample:" << m_audioFileBuffer->getBytesPerSample();
			AUDIO_ENGINE_DEBUG << "  bits per sample:" << m_audioFileBuffer->getBitsPerSample();
			AUDIO_ENGINE_DEBUG << "  samples per second:" << m_audioFileBuffer->getSamplesPerSec();
			AUDIO_ENGINE_DEBUG << "  no of channels:" << channels;

			const qint16 *ptr = reinterpret_cast<const qint16*>(m_audioFileBuffer->getRawData());
			const int numSamples = m_audioFileBuffer->getDataLength() / (m_audioFileBuffer->getBytesPerSample() * channels);
			//const int numSamples = m_audioFileBuffer->getDataLength() / m_audioFileBuffer->getBytesPerSample();
			AUDIO_ENGINE_DEBUG << "  no of samples:" << numSamples;
			//float realSample;
			QList<qreal> audioBuffer;
			
			for (int i = 0; i < numSamples; i++) {

				for (int j = 0; j < m_audioFileBuffer->getNofChannels(); j++) {
					
					//realSample = pcmToReal(*ptr);
					//AUDIO_ENGINE_DEBUG << i << "\t" << *ptr << "\t" << realSample;
					audioBuffer << pcmToReal(*ptr);
					ptr++;// += m_audioFileBuffer->getNofChannels();
				}

				//realSample = pcmToReal(*ptr);
				////AUDIO_ENGINE_DEBUG << i << "\t" << *ptr << "\t" << realSample;
				//audioBuffer << pcmToReal(*ptr);
				//ptr++;// += m_audioFileBuffer->getNofChannels();
			}

			//int lng = audioBuffer.length();
			//for (int i = 0; i < lng/6; i++)
			//	qDebug()  << audioBuffer.at(i) << "\t" << audioBuffer.at(i+1) << "\t" << audioBuffer.at(i+2) << "\t"
			//			  << audioBuffer.at(i+3) << "\t" << audioBuffer.at(i+4) << "\t" << audioBuffer.at(i+5);

			emit audiofileBufferChanged(audioBuffer);
        }
		else if (isPCMS32LE(m_file->fileFormat())) {

			result = initializePCMS32LE();
		}
		else {

            m_message = "[audio engine]: audio format %1 not supported.";
			emit messageEvent(m_message.arg(formatToString(m_file->fileFormat())));
        }
    } 
	else {
        
		m_message = "[audio engine]: could not open %1.";
		emit messageEvent(m_message.arg(fileName));
    }

    if (result) {

        m_analysisFile = new WavFile(this);
        m_analysisFile->open(fileName);
    }
*/
    return result;
}

void AudioEngine::suspend() {

    //if (QAudio::ActiveState == m_state || QAudio::IdleState == m_state) {

    //    switch (m_mode) {

    //    case QAudio::AudioInput:

    //        m_audioInput->suspend();
    //        break;

    //    case QAudio::AudioOutput:

    //        m_audioOutput->suspend();
    //        break;
    //    }
    //}
}

void AudioEngine::showSettingsDialog() {

    //setDialog->exec();
    //if (setDialog->result() == QDialog::Accepted) {

    //    setAudioInputDevice(setDialog->inputDevice());
    //    setAudioOutputDevice(setDialog->outputDevice());
    //    //setWindowFunction(setDialog->windowFunction());
    //}
}

//bool AudioEngine::generateSweptTone() {
//
//    Q_ASSERT(!m_generateTone);
//    Q_ASSERT(!m_file);
//    m_generateTone = true;
//    m_tone.startFreq = m_lowerChirpFreq;
//    m_tone.endFreq = m_upperChirpFreq;
//    m_tone.amplitude = m_chirpAmplitude;
//	m_tone.duration = m_chirpBufferDurationUs;
//    AUDIOENGINE_DEBUG	<< "generateSweptTone"
//						<< "startFreq" << m_tone.startFreq
//						<< "endFreq" << m_tone.endFreq
//						<< "amp" << m_tone.amplitude
//						<< "duration" << m_tone.duration;
//
//	m_message = "[audio engine]: set chirp signal to: low = %1 Hz, high = %2 Hz, amplitude = %3, duration = %4 s";
//	emit messageEvent(m_message.arg(m_lowerChirpFreq).arg(m_upperChirpFreq).arg(m_chirpAmplitude).arg(m_chirpBufferDurationUs/1E6));
//
//    return initializePCMS16LE();
//}