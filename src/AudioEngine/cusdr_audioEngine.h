#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

//#include <QThread>
#include <QDebug>
#include <QTimer>
//#include "bass24/c/bass.h"
#include "Util/bass.h"
#include "cusdr_settings.h"

#ifdef LOG_AUDIOENGINE
#   define AUDIOENGINE_DEBUG qDebug().nospace() << "AudioEngine::\t"
#else
#   define AUDIOENGINE_DEBUG nullDebug()
#endif

struct Tone {
    Tone(qreal freq = 0.0, qreal amp = 0.0, qint64 dur = 0)
    :   frequency(freq), amplitude(amp), duration(dur)
    { }

    // Start and end frequencies for swept tone generation
    qreal   frequency;

    // Amplitude in range [0.0, 1.0]
    qreal   amplitude;

	// tone duration in micro seconds
	qint64	duration;
};

struct SweptTone {
    SweptTone(qreal start = 0.0, qreal end = 0.0, qreal amp = 0.0, qint64 dur = 0)
    :   startFreq(start), endFreq(end), amplitude(amp), duration(dur)
    { Q_ASSERT(end >= start); }

    SweptTone(const Tone &tone)
    :   startFreq(tone.frequency), endFreq(tone.frequency), amplitude(tone.amplitude), duration(tone.duration)
    { }

    // Start and end frequencies for swept tone generation
    qreal   startFreq;
    qreal   endFreq;

    // Amplitude in range [0.0, 1.0]
    qreal   amplitude;

	// tone duration in micro seconds
	qint64	duration;
};

void __stdcall syncFunc(HSYNC handle, DWORD channel, DWORD data, void *user);

//class AudioThread : public QThread
class AudioEngine : public QObject {

    Q_OBJECT

public:
    explicit AudioEngine(QObject *parent = 0);
    bool playing;
	//bool generateSweptTone();
	bool loadAudioFile(const QString &fileName);
    void run();

private:
    unsigned long chan;
    QTimer*		t;

	QString		m_message;
	SweptTone   m_tone;

	int			m_lowerChirpFreq;
	int			m_upperChirpFreq;

	qint64		m_chirpBufferDurationUs;

	bool		m_generateTone;

	qreal		m_chirpAmplitude;

signals:
	void messageEvent(QString msg);
    void endOfPlayback();
    void curPos(double Position, double Total);

public slots:
    void play(QString filepath);
	void startPlayback();
	void showSettingsDialog();
    void pause();
    void resume();
	void reset();
    void stop();
	void suspend();
    void signalUpdate();
    void changePosition(int position);
};

#endif // AUDIOENGINE_H