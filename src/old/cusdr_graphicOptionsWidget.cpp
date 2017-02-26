/**
* @file  cusdr_graphicOptionsWidget.cpp
* @brief OpenGL graphics options widget class for cuSDR
* @author Hermann von Hasseln, DL3HVH
* @version 0.1
* @date 2011-08-19
*/

/*
 *   
 *   Copyright 2010, 2011 Hermann von Hasseln, DL3HVH
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

//#define LOG_GRAPHIC_OPTIONS_WIDGET

//#include <QtGui>
//#include <QPen>
//#include <QDebug>
//#include <QDialog>
//#include <QColorDialog>

#include "cusdr_graphicOptionsWidget.h"

#define	btn_height		15
#define	btn_width		70
#define	btn_widths		38
#define	btn_width2		52
#define	btn_width3		60

GraphicOptionsWidget::GraphicOptionsWidget(QWidget *parent) 
	: QWidget(parent)
	, set(Settings::instance())
	, m_serverMode(set->getCurrentServerMode())
	, m_hwInterface(set->getHWInterface())
	, m_dataEngineState(set->getDataEngineState())
	, m_panadapterMode(set->getPanadapterMode())
	, m_waterColorScheme(set->getWaterfallColorScheme())
	, m_colorTriangle(new QtColorTriangle(this))
	, m_antialiased(true)
	, m_mouseOver(false)
	, m_minimumWidgetWidth(set->getMinimumWidgetWidth())
	, m_minimumGroupBoxWidth(set->getMinimumGroupBoxWidth())
	, m_btnSpacing(5)
	, m_currentReceiver(set->getCurrentReceiver())
	, m_btnChooserHit(0)
	, m_framesPerSecond(set->getFramesPerSecond(m_currentReceiver))
	, m_graphicResolution(set->getGraphicResolution())
	, m_sampleRate(set->getSampleRate())
	, m_sMeterHoldTime(set->getSMeterHoldTime())
{
	//setMinimumWidth(m_minimumWidgetWidth);
	setContentsMargins(4, 0, 4, 0);
	setMouseTracking(true);

	m_rxDataList = set->getReceiverDataList();
	
	//QFont titleFont;
	m_titleFont.setStyleStrategy(QFont::PreferQuality);
	m_titleFont.setFixedPitch(true);
	#ifdef Q_OS_MAC
		m_titleFont.setPixelSize(11);
		m_titleFont.setFamily("Arial");
		//m_smallFont.setBold(true);
	#endif
	#ifdef Q_OS_WIN
		m_titleFont.setPixelSize(11);
		m_titleFont.setFamily("Arial");
		//m_smallFont.setBold(true);
		//m_smallFont.setItalic(true);
	#endif

	m_smallFont.setStyleStrategy(QFont::PreferAntialias);
	m_smallFont.setFixedPitch(true);
	#ifdef Q_OS_MAC
		m_smallFont.setPixelSize(10);
		m_smallFont.setFamily("Arial");
		//m_smallFont.setBold(true);
	#endif
	#ifdef Q_OS_WIN
		m_smallFont.setPixelSize(10);
		m_smallFont.setFamily("Arial");
		//m_smallFont.setBold(true);
		//m_smallFont.setItalic(true);
	#endif

	createFPSGroupBox();
	createPanSpectrumOptions();
	createWaterfallSpectrumOptions();
	createSMeterOptions();
	createColorChooserWidget();
	createCallSignEditor();

	QBoxLayout *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	mainLayout->setSpacing(5);
	mainLayout->setMargin(0);
	mainLayout->addSpacing(8);

	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(0);
	hbox1->setMargin(0);
	hbox1->addStretch();
	hbox1->addWidget(m_fpsGroupBox);

	QHBoxLayout *hbox2 = new QHBoxLayout;
	hbox2->setSpacing(0);
	hbox2->setMargin(0);
	hbox2->addStretch();
	hbox2->addWidget(m_panSpectrumOptions);

	QHBoxLayout *hbox3 = new QHBoxLayout;
	hbox3->setSpacing(0);
	hbox3->setMargin(0);
	hbox3->addStretch();
	hbox3->addWidget(m_waterfallSpectrumOptions);

	QHBoxLayout *hbox4 = new QHBoxLayout;
	hbox4->setSpacing(0);
	hbox4->setMargin(0);
	hbox4->addStretch();
	hbox4->addWidget(m_sMeterOptions);

	QHBoxLayout *hbox5 = new QHBoxLayout;
	hbox5->setSpacing(0);
	hbox5->setMargin(0);
	hbox5->addStretch();
	hbox5->addWidget(m_colorChooserWidget);
	hbox5->addStretch();

	QHBoxLayout *hbox6 = new QHBoxLayout;
	hbox6->setSpacing(0);
	hbox6->setMargin(0);
	hbox6->addStretch();
	hbox6->addWidget(m_callSignEditor);
	hbox6->addStretch();

	mainLayout->addLayout(hbox1);
	mainLayout->addLayout(hbox2);
	mainLayout->addLayout(hbox3);
	mainLayout->addLayout(hbox4);
	mainLayout->addLayout(hbox5);
	mainLayout->addLayout(hbox6);
	mainLayout->addStretch();
	setLayout(mainLayout);

	m_panadapterColors = set->getPanadapterColors();
	m_oldPanadapterColors = m_panadapterColors;

	setupConnections();
}

GraphicOptionsWidget::~GraphicOptionsWidget() {

	// disconnect all signals
	disconnect(set, 0, this, 0);
	disconnect(this, 0, 0, 0);
}

QSize GraphicOptionsWidget::sizeHint() const {
	
	return QSize(m_minimumWidgetWidth, height());
}

QSize GraphicOptionsWidget::minimumSizeHint() const {

	return QSize(m_minimumWidgetWidth, height());
}

void GraphicOptionsWidget::setupConnections() {

	CHECKED_CONNECT(
		set,
		SIGNAL(systemStateChanged(
					QObject *,
					QSDR::_Error,
					QSDR::_HWInterfaceMode,
					QSDR::_ServerMode,
					QSDR::_DataEngineState)),
		this,
		SLOT(systemStateChanged(
					QObject *,
					QSDR::_Error,
					QSDR::_HWInterfaceMode,
					QSDR::_ServerMode,
					QSDR::_DataEngineState)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(graphicModeChanged(
					QObject *,
					QSDRGraphics::_Panadapter,
					QSDRGraphics::_WfScheme)),
		this, 
		SLOT(graphicModeChanged(
					QObject *,
					QSDRGraphics::_Panadapter,
					QSDRGraphics::_WfScheme)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(currentReceiverChanged(QObject *, int)),
		this, 
		SLOT(setCurrentReceiver(QObject *, int)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(sampleRateChanged(QObject *, int)), 
		this, 
		SLOT(sampleRateChanged(QObject *, int)));

	/*CHECKED_CONNECT(
		set, 
		SIGNAL(dBmScaleMinChanged(int, qreal)), 
		this, 
		SLOT(setdBmScaleMin(int, qreal)));

	CHECKED_CONNECT(
		set, 
		SIGNAL(dBmScaleMaxChanged(int, qreal)), 
		this, 
		SLOT(setdBmScaleMax(int, qreal)));*/

	CHECKED_CONNECT(
		m_colorTriangle, 
		SIGNAL(colorChanged(QColor)), 
		this, 
		SLOT(triangleColorChanged(QColor)));

	CHECKED_CONNECT(
		set,
		SIGNAL(framesPerSecondChanged(QObject*, int, int)),
		this,
		SLOT(setFramesPerSecond(QObject*, int, int)));
}

void GraphicOptionsWidget::systemStateChanged(
	QObject *sender, 
	QSDR::_Error err, 
	QSDR::_HWInterfaceMode hwmode, 
	QSDR::_ServerMode mode, 
	QSDR::_DataEngineState state)
{
	Q_UNUSED (sender)
	Q_UNUSED (err)

	bool change = false;

	if (m_hwInterface != hwmode) {
		
		m_hwInterface = hwmode;
		//hwInterfaceChanged();
		change = true;
	}

	if (m_serverMode != mode) {
		
		m_serverMode = mode;
		change = true;
	}
		
	if (m_dataEngineState != state) {

		m_dataEngineState = state;

		if (m_dataEngineState == QSDR::DataEngineUp) {
		
			
		}
		else {

			
		}
		change = true;
	}
	if (!change) return;

	update();
}

void GraphicOptionsWidget::graphicModeChanged(
	QObject *sender,
	QSDRGraphics::_Panadapter panMode,
	QSDRGraphics::_WfScheme waterColorScheme)
{
	Q_UNUSED (sender)

	bool change = false;

	if (m_panadapterMode != panMode) {
		
		m_panadapterMode = panMode;
		change = true;
	}

	if (m_waterColorScheme != waterColorScheme) {
		
		m_waterColorScheme = waterColorScheme;
		change = true;
	}

	if (!change) return;

	update();
}

void GraphicOptionsWidget::createFPSGroupBox() {

	QFontMetrics fm(m_smallFont);
	int fontMaxWidth = fm.boundingRect(" 200 ").width();
	//int vol = (int)(set->getMainVolume(0) * 100);

	m_fpsSlider = new QSlider(Qt::Horizontal, this);
	m_fpsSlider->setTickPosition(QSlider::NoTicks);
	m_fpsSlider->setFixedSize(190, 12);
	m_fpsSlider->setSingleStep(1);
	m_fpsSlider->setRange(1, 200);
	m_fpsSlider->setValue(m_framesPerSecond);
	m_fpsSlider->setStyleSheet(set->getVolSliderStyle());

	CHECKED_CONNECT(m_fpsSlider, SIGNAL(valueChanged(int)), this, SLOT(fpsValueChanged(int)));

	QString str = "%1 ";
	m_fpsLevelLabel = new QLabel(str.arg(m_framesPerSecond, 2, 10, QLatin1Char(' ')), this);
	m_fpsLevelLabel->setFont(m_smallFont);
	m_fpsLevelLabel->setFixedSize(fontMaxWidth, 12);
	m_fpsLevelLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_fpsLevelLabel->setStyleSheet(set->getSliderLabelStyle());

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->setSpacing(0);
	hbox->setMargin(0);
	hbox->addStretch();
	hbox->addWidget(m_fpsSlider);
	hbox->addWidget(m_fpsLevelLabel);

	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->setSpacing(6);
	vbox->addSpacing(6);
	vbox->addLayout(hbox);

	m_fpsGroupBox = new QGroupBox(tr("Frames per Second"), this);
	m_fpsGroupBox->setMinimumWidth(m_minimumGroupBoxWidth);
	m_fpsGroupBox->setLayout(vbox);
	m_fpsGroupBox->setStyleSheet(set->getWidgetStyle());
	m_fpsGroupBox->setFont(QFont("Arial", 8));
}

void GraphicOptionsWidget::createPanSpectrumOptions() {

	m_PanLineBtn = new AeroButton("Line", this);
	m_PanLineBtn->setRoundness(0);
	m_PanLineBtn->setFixedSize(btn_width, btn_height);
	m_panadapterBtnList.append(m_PanLineBtn);

	CHECKED_CONNECT(
		m_PanLineBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(panModeChanged()));

	m_PanFilledLineBtn = new AeroButton("Filled Line", this);
	m_PanFilledLineBtn->setRoundness(0);
	m_PanFilledLineBtn->setFixedSize(btn_width, btn_height);
	m_panadapterBtnList.append(m_PanFilledLineBtn);

	CHECKED_CONNECT(
		m_PanFilledLineBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(panModeChanged()));

	m_PanSolidBtn = new AeroButton("Solid", this);
	m_PanSolidBtn->setRoundness(0);
	m_PanSolidBtn->setFixedSize (btn_width, btn_height);
	m_panadapterBtnList.append(m_PanSolidBtn);

	CHECKED_CONNECT(
		m_PanSolidBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(panModeChanged()));
	
	switch (m_panadapterMode) {

		case QSDRGraphics::Line:
			
			m_PanLineBtn->setBtnState(AeroButton::ON);
			m_PanFilledLineBtn->setBtnState(AeroButton::OFF);
			m_PanSolidBtn->setBtnState(AeroButton::OFF);
			break;

		case QSDRGraphics::FilledLine:
			
			m_PanFilledLineBtn->setBtnState(AeroButton::ON);
			m_PanLineBtn->setBtnState(AeroButton::OFF);
			m_PanSolidBtn->setBtnState(AeroButton::OFF);
			break;

		case QSDRGraphics::Solid:
			
			m_PanSolidBtn->setBtnState(AeroButton::ON);
			m_PanLineBtn->setBtnState(AeroButton::OFF);
			m_PanFilledLineBtn->setBtnState(AeroButton::OFF);
			break;
	}

	QFontMetrics fm(m_smallFont);
	int fontMaxWidth = fm.boundingRect(" 200 ").width();

	m_avgValue = set->getSpectrumAveragingCnt();

	m_avgSlider = new QSlider(Qt::Horizontal, this);
	m_avgSlider->setTickPosition(QSlider::NoTicks);
	m_avgSlider->setFixedSize(130, 12);
	m_avgSlider->setSingleStep(1);
	m_avgSlider->setRange(1, 100);
	m_avgSlider->setValue(m_avgValue);
	m_avgSlider->setStyleSheet(set->getVolSliderStyle());

	CHECKED_CONNECT(m_avgSlider, SIGNAL(valueChanged(int)), this, SLOT(averagingFilterCntChanged(int)));

	QString str = "%1 ";
	m_avgLevelLabel = new QLabel(str.arg(m_avgValue, 2, 10, QLatin1Char(' ')), this);
	m_avgLevelLabel->setFont(m_smallFont);
	m_avgLevelLabel->setFixedSize(fontMaxWidth, 12);
	m_avgLevelLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_avgLevelLabel->setStyleSheet(set->getSliderLabelStyle());

	m_avgLabel = new QLabel("Avg Filter:", this);
	m_avgLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_avgLabel->setStyleSheet(set->getLabelStyle());

	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(4);
	hbox1->addStretch();
	hbox1->addWidget(m_PanLineBtn);
	hbox1->addWidget(m_PanFilledLineBtn);
	hbox1->addWidget(m_PanSolidBtn);
	
	QHBoxLayout *hbox2 = new QHBoxLayout;
	hbox2->setSpacing(0);
	hbox2->setMargin(0);
	hbox2->addWidget(m_avgLabel);
	hbox2->addStretch();
	hbox2->addWidget(m_avgSlider);
	hbox2->addWidget(m_avgLevelLabel);

	/*
	QHBoxLayout *hbox4 = new QHBoxLayout;
	hbox4->setSpacing(4);
	hbox4->addWidget(m_framePerSecondsLabel);
	hbox4->addStretch();
	hbox4->addWidget(m_framesPerSecondSpinBox);
	*/
	
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->setSpacing(6);
	vbox->addSpacing(6);
	vbox->addLayout(hbox1);
	vbox->addLayout(hbox2);
	//vbox->addLayout(hbox3);
	//vbox->addLayout(hbox4);

	m_panSpectrumOptions = new QGroupBox(tr("Panadapter Spectrum"), this);
	m_panSpectrumOptions->setMinimumWidth(m_minimumGroupBoxWidth);
	m_panSpectrumOptions->setLayout(vbox);
	m_panSpectrumOptions->setStyleSheet(set->getWidgetStyle());
	m_panSpectrumOptions->setFont(QFont("Arial", 8));
}

void GraphicOptionsWidget::createWaterfallSpectrumOptions() {

	m_waterfallSimpleBtn = new AeroButton("Simple", this);
	m_waterfallSimpleBtn->setRoundness(0);
	m_waterfallSimpleBtn->setFixedSize(btn_width, btn_height);
	m_waterfallSchemeBtnList.append(m_waterfallSimpleBtn);

	CHECKED_CONNECT(
		m_waterfallSimpleBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(waterfallSchemeChanged()));

	m_waterfallEnhancedBtn = new AeroButton("Enhanced", this);
	m_waterfallEnhancedBtn->setRoundness(0);
	m_waterfallEnhancedBtn->setFixedSize(btn_width, btn_height);
	m_waterfallSchemeBtnList.append(m_waterfallEnhancedBtn);

	CHECKED_CONNECT(
		m_waterfallEnhancedBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(waterfallSchemeChanged()));

	m_waterfallSpectranBtn = new AeroButton("Spectran", this);
	m_waterfallSpectranBtn->setRoundness(0);
	m_waterfallSpectranBtn->setFixedSize(btn_width, btn_height);
	m_waterfallSchemeBtnList.append(m_waterfallSpectranBtn);

	CHECKED_CONNECT(
		m_waterfallSpectranBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(waterfallSchemeChanged()));

	switch (m_waterColorScheme) {

		case QSDRGraphics::simple:
			
			m_waterfallSimpleBtn->setBtnState(AeroButton::ON);
			m_waterfallEnhancedBtn->setBtnState(AeroButton::OFF);
			m_waterfallSpectranBtn->setBtnState(AeroButton::OFF);
			break;

		case QSDRGraphics::enhanced:
			
			m_waterfallEnhancedBtn->setBtnState(AeroButton::ON);
			m_waterfallSimpleBtn->setBtnState(AeroButton::OFF);
			m_waterfallSpectranBtn->setBtnState(AeroButton::OFF);
			break;

		case QSDRGraphics::spectran:
			
			m_waterfallSpectranBtn->setBtnState(AeroButton::ON);
			m_waterfallSimpleBtn->setBtnState(AeroButton::OFF);
			m_waterfallEnhancedBtn->setBtnState(AeroButton::OFF);
			break;
	}

	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(4);
	hbox1->addStretch();
	//hbox1->addSpacing(77);
	hbox1->addWidget(m_waterfallSimpleBtn);
	hbox1->addWidget(m_waterfallEnhancedBtn);
	hbox1->addWidget(m_waterfallSpectranBtn);

	/*m_waterfallTimeSpinBox = new QSpinBox(this);
	m_waterfallTimeSpinBox->setMinimum(5);
	m_waterfallTimeSpinBox->setMaximum(250);
	m_waterfallTimeSpinBox->setSingleStep(5);
	m_waterfallTimeSpinBox->setStyleSheet(set->getSpinBoxStyle());
	m_waterfallTimeSpinBox->setValue(m_rxDataList.at(m_currentReceiver).waterfallTime);

	CHECKED_CONNECT(
		m_waterfallTimeSpinBox,
		SIGNAL(valueChanged(int)),
		this,
		SLOT(waterfallTimeChanged(int)));*/

	m_waterfallLoOffsetSpinBox = new QSpinBox(this);
	m_waterfallLoOffsetSpinBox->setMinimum(-200);
	m_waterfallLoOffsetSpinBox->setMaximum(200);
	m_waterfallLoOffsetSpinBox->setSingleStep(1);
	m_waterfallLoOffsetSpinBox->setStyleSheet(set->getSpinBoxStyle());
	m_waterfallLoOffsetSpinBox->setValue(m_rxDataList.at(m_currentReceiver).waterfallOffsetLo);

	CHECKED_CONNECT(
		m_waterfallLoOffsetSpinBox,
		SIGNAL(valueChanged(int)),
		this,
		SLOT(waterfallLoOffsetChanged(int)));

	m_waterfallHiOffsetSpinBox = new QSpinBox(this);
	m_waterfallHiOffsetSpinBox->setMinimum(-200);
	m_waterfallHiOffsetSpinBox->setMaximum(200);
	m_waterfallHiOffsetSpinBox->setSingleStep(1);
	m_waterfallHiOffsetSpinBox->setStyleSheet(set->getSpinBoxStyle());
	m_waterfallHiOffsetSpinBox->setValue(m_rxDataList.at(m_currentReceiver).waterfallOffsetHi);

	CHECKED_CONNECT(
		m_waterfallHiOffsetSpinBox,
		SIGNAL(valueChanged(int)),
		this,
		SLOT(waterfallHiOffsetChanged(int)));

	//m_waterfallTimeLabel = new QLabel("Timing (ms):", this);
    //m_waterfallTimeLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	//m_waterfallTimeLabel->setStyleSheet(set->getLabelStyle());

	m_waterfallLoOffsetLabel = new QLabel("Offset Low (dBm):", this);
    m_waterfallLoOffsetLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_waterfallLoOffsetLabel->setStyleSheet(set->getLabelStyle());

	m_waterfallHiOffsetLabel = new QLabel("Offset High (dBm):", this);
    m_waterfallHiOffsetLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_waterfallHiOffsetLabel->setStyleSheet(set->getLabelStyle());

	/*QHBoxLayout *hbox2 = new QHBoxLayout;
	hbox2->setSpacing(4);
	hbox2->addWidget(m_waterfallTimeLabel);
	hbox2->addStretch();
	hbox2->addWidget(m_waterfallTimeSpinBox);*/

	QHBoxLayout *hbox3 = new QHBoxLayout;
	hbox3->setSpacing(4);
	hbox3->addWidget(m_waterfallLoOffsetLabel);
	hbox3->addStretch();
	hbox3->addWidget(m_waterfallLoOffsetSpinBox);

	QHBoxLayout *hbox4 = new QHBoxLayout;
	hbox4->setSpacing(4);
	hbox4->addWidget(m_waterfallHiOffsetLabel);
	hbox4->addStretch();
	hbox4->addWidget(m_waterfallHiOffsetSpinBox);

	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->setSpacing(6);
	vbox->addSpacing(6);
	vbox->addLayout(hbox1);
	//vbox->addLayout(hbox2);
	vbox->addLayout(hbox3);
	vbox->addLayout(hbox4);

	m_waterfallSpectrumOptions = new QGroupBox(tr("Waterfall Spectrum"), this);
	m_waterfallSpectrumOptions->setMinimumWidth(m_minimumGroupBoxWidth);
	m_waterfallSpectrumOptions->setLayout(vbox);
	m_waterfallSpectrumOptions->setStyleSheet(set->getWidgetStyle());
	m_waterfallSpectrumOptions->setFont(QFont("Arial", 8));
}

void GraphicOptionsWidget::createSMeterOptions() {

	/*m_sMeterA = new AeroButton("S-Meter A", this);
	m_sMeterA->setRoundness(0);
	m_sMeterA->setFixedSize (btn_width, btn_height);

	CHECKED_CONNECT(
		m_sMeterA, 
		SIGNAL(clicked()), 
		this, 
		SLOT(sMeterChanged()));

	m_sMeterB = new AeroButton("S-Meter B", this);
	m_sMeterB->setRoundness(0);
	m_sMeterB->setFixedSize (btn_width, btn_height);

	CHECKED_CONNECT(
		m_sMeterA, 
		SIGNAL(clicked()), 
		this, 
		SLOT(sMeterChanged()));*/
	
	m_sMeterHoldTimeSpinBox = new QSpinBox(this);
	m_sMeterHoldTimeSpinBox->setMinimum(500);
	m_sMeterHoldTimeSpinBox->setMaximum(10000);
	m_sMeterHoldTimeSpinBox->setSingleStep(500);
	m_sMeterHoldTimeSpinBox->setStyleSheet(set->getSpinBoxStyle());
	m_sMeterHoldTimeSpinBox->setValue(m_sMeterHoldTime);

	CHECKED_CONNECT(
		m_sMeterHoldTimeSpinBox,
		SIGNAL(valueChanged(int)),
		this,
		SLOT(sMeterHoldTimeChanged(int)));

	m_sMeterHoldTimeLabel = new QLabel("Hold Time (ms):", this);
    m_sMeterHoldTimeLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_sMeterHoldTimeLabel->setStyleSheet(set->getLabelStyle());

	/*QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(4);
	hbox1->addStretch();
	hbox1->addWidget(m_sMeterA);
	hbox1->addWidget(m_sMeterB);*/

	QHBoxLayout *hbox2 = new QHBoxLayout;
	hbox2->setSpacing(4);
	//hbox2->addSpacing(77);
	hbox2->addWidget(m_sMeterHoldTimeLabel);
	hbox2->addStretch();
	hbox2->addWidget(m_sMeterHoldTimeSpinBox);

	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->setSpacing(6);
	vbox->addSpacing(6);
	//vbox->addLayout(hbox1);
	vbox->addLayout(hbox2);
	//vbox->addLayout(hbox3);
	//vbox->addLayout(hbox4);

	m_sMeterOptions = new QGroupBox(tr("S-Meter"), this);
	m_sMeterOptions->setMinimumWidth(m_minimumGroupBoxWidth);
	m_sMeterOptions->setLayout(vbox);
	m_sMeterOptions->setStyleSheet(set->getWidgetStyle());
	m_sMeterOptions->setFont(QFont("Arial", 8));
}

void GraphicOptionsWidget::createColorChooserWidget() {

	QColor col = set->getPanadapterColors().panCenterLineColor;

	m_setPanBackground = new AeroButton("Background", this);
	m_setPanBackground->setRoundness(0);
	m_setPanBackground->setFixedSize (btn_width3, btn_height);
	m_setPanBackground->setStyleSheet(set->getMiniButtonStyle());
	m_setPanBackground->setColorOn(col);
	m_changeColorBtnList.append(m_setPanBackground);

	CHECKED_CONNECT(
		m_setPanBackground, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().waterfallColor;

	m_setWaterfall = new AeroButton("Waterfall", this);
	m_setWaterfall->setRoundness(0);
	m_setWaterfall->setFixedSize (btn_width3, btn_height);
	m_setWaterfall->setStyleSheet(set->getMiniButtonStyle());
	m_setWaterfall->setColorOn(col);
	m_changeColorBtnList.append(m_setWaterfall);

	CHECKED_CONNECT(
		m_setWaterfall, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().panLineColor;

	m_setPanLine = new AeroButton("Pan Line", this);
	m_setPanLine->setRoundness(0);
	m_setPanLine->setFixedSize (btn_width3, btn_height);
	m_setPanLine->setStyleSheet(set->getMiniButtonStyle());
	m_setPanLine->setColorOn(col);
	m_changeColorBtnList.append(m_setPanLine);

	CHECKED_CONNECT(
		m_setPanLine, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().panLineFilledColor;

	m_setPanLineFilling = new AeroButton("Pan Filling", this);
	m_setPanLineFilling->setRoundness(0);
	m_setPanLineFilling->setFixedSize (btn_width3, btn_height);
	m_setPanLineFilling->setStyleSheet(set->getMiniButtonStyle());
	m_setPanLineFilling->setColorOn(col);
	m_changeColorBtnList.append(m_setPanLineFilling);

	CHECKED_CONNECT(
		m_setPanLineFilling, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().panSolidTopColor;

	m_setPanSolidTop = new AeroButton("Solid T", this);
	m_setPanSolidTop->setRoundness(0);
	m_setPanSolidTop->setFixedSize (btn_width3, btn_height);
	m_setPanSolidTop->setStyleSheet(set->getMiniButtonStyle());
	m_setPanSolidTop->setColorOn(col);
	m_changeColorBtnList.append(m_setPanSolidTop);

	CHECKED_CONNECT(
		m_setPanSolidTop, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().panSolidBottomColor;

	m_setPanSolidBottom = new AeroButton("Solid B", this);
	m_setPanSolidBottom->setRoundness(0);
	m_setPanSolidBottom->setFixedSize (btn_width3, btn_height);
	m_setPanSolidBottom->setStyleSheet(set->getMiniButtonStyle());
	m_setPanSolidBottom->setColorOn(col);
	m_changeColorBtnList.append(m_setPanSolidBottom);

	CHECKED_CONNECT(
		m_setPanSolidBottom, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().wideBandLineColor;

	m_setWideBandLine = new AeroButton("Wideband", this);
	m_setWideBandLine->setRoundness(0);
	m_setWideBandLine->setFixedSize (btn_width3, btn_height);
	m_setWideBandLine->setStyleSheet(set->getMiniButtonStyle());
	m_setWideBandLine->setColorOn(col);
	m_changeColorBtnList.append(m_setWideBandLine);

	CHECKED_CONNECT(
		m_setWideBandLine, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().wideBandFilledColor;

	m_setWideBandFilling = new AeroButton("WB Filling", this);
	m_setWideBandFilling->setRoundness(0);
	m_setWideBandFilling->setFixedSize (btn_width3, btn_height);
	m_setWideBandFilling->setStyleSheet(set->getMiniButtonStyle());
	m_setWideBandFilling->setColorOn(col);
	m_changeColorBtnList.append(m_setWideBandFilling);

	CHECKED_CONNECT(
		m_setWideBandFilling, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().wideBandSolidTopColor;

	m_setWideBandSolidTop = new AeroButton("WB Solid T", this);
	m_setWideBandSolidTop->setRoundness(0);
	m_setWideBandSolidTop->setFixedSize (btn_width3, btn_height);
	m_setWideBandSolidTop->setStyleSheet(set->getMiniButtonStyle());
	m_setWideBandSolidTop->setColorOn(col);
	m_changeColorBtnList.append(m_setWideBandSolidTop);

	CHECKED_CONNECT(
		m_setWideBandSolidTop, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().wideBandSolidBottomColor;

	m_setWideBandSolidBottom = new AeroButton("WB Solid B", this);
	m_setWideBandSolidBottom->setRoundness(0);
	m_setWideBandSolidBottom->setFixedSize (btn_width3, btn_height);
	m_setWideBandSolidBottom->setStyleSheet(set->getMiniButtonStyle());
	m_setWideBandSolidBottom->setColorOn(col);
	m_changeColorBtnList.append(m_setWideBandSolidBottom);

	CHECKED_CONNECT(
		m_setWideBandSolidBottom, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().wideBandSolidBottomColor;

	m_setDistanceLine = new AeroButton("Dist. Line", this);
	m_setDistanceLine->setRoundness(0);
	m_setDistanceLine->setFixedSize (btn_width3, btn_height);
	m_setDistanceLine->setStyleSheet(set->getMiniButtonStyle());
	m_setDistanceLine->setColorOn(col);
	m_changeColorBtnList.append(m_setDistanceLine);

	CHECKED_CONNECT(
		m_setDistanceLine, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().wideBandSolidBottomColor;

	m_setDistanceLineFilling = new AeroButton("Dist. Filling", this);
	m_setDistanceLineFilling->setRoundness(0);
	m_setDistanceLineFilling->setFixedSize (btn_width3, btn_height);
	m_setDistanceLineFilling->setStyleSheet(set->getMiniButtonStyle());
	m_setDistanceLineFilling->setColorOn(col);
	m_changeColorBtnList.append(m_setDistanceLineFilling);

	CHECKED_CONNECT(
		m_setDistanceLineFilling, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().panCenterLineColor;

	m_setPanCenterLine = new AeroButton("Center", this);
	m_setPanCenterLine->setRoundness(0);
	m_setPanCenterLine->setFixedSize (btn_width3, btn_height);
	m_setPanCenterLine->setStyleSheet(set->getMiniButtonStyle());
	m_setPanCenterLine->setColorOn(col);
	m_changeColorBtnList.append(m_setPanCenterLine);

	CHECKED_CONNECT(
		m_setPanCenterLine, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));

	col = set->getPanadapterColors().gridLineColor;

	m_setGridLine = new AeroButton("Grid", this);
	m_setGridLine->setRoundness(0);
	m_setGridLine->setFixedSize (btn_width3, btn_height);
	m_setGridLine->setStyleSheet(set->getMiniButtonStyle());
	m_setGridLine->setColorOn(col);
	m_changeColorBtnList.append(m_setGridLine);

	CHECKED_CONNECT(
		m_setGridLine, 
		SIGNAL(clicked()), 
		this, 
		SLOT(colorChooserChanged()));


	m_setPanBackground->setBtnState(AeroButton::OFF);
	m_setWaterfall->setBtnState(AeroButton::OFF);
	m_setPanLine->setBtnState(AeroButton::OFF);
	m_setPanLineFilling->setBtnState(AeroButton::OFF);
	m_setPanSolidTop->setBtnState(AeroButton::OFF);
	m_setPanSolidBottom->setBtnState(AeroButton::OFF);
	m_setWideBandLine->setBtnState(AeroButton::OFF);
	m_setWideBandFilling->setBtnState(AeroButton::OFF);
	m_setWideBandSolidTop->setBtnState(AeroButton::OFF);
	m_setWideBandSolidBottom->setBtnState(AeroButton::OFF);
	m_setDistanceLine->setBtnState(AeroButton::OFF);
	m_setDistanceLineFilling->setBtnState(AeroButton::OFF);
	m_setPanCenterLine->setBtnState(AeroButton::OFF);
	m_setGridLine->setBtnState(AeroButton::OFF);

	m_resetBtn = new AeroButton("Reset", this);
	m_resetBtn->setRoundness(0);
	//m_resetBtn->setFixedSize (btn_widths, btn_height);
	m_resetBtn->setFixedSize (btn_width3, btn_height);
	
	CHECKED_CONNECT(
		m_resetBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(resetColors()));

	m_okBtn = new AeroButton("OK", this);
	m_okBtn->setRoundness(0);
	//m_okBtn->setFixedSize (btn_widths, btn_height);
	m_okBtn->setFixedSize (btn_width3, btn_height);
	
	CHECKED_CONNECT(
		m_okBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(acceptColors()));

	
	QGridLayout *grid1 = new QGridLayout;
	grid1->setSpacing(1);
	grid1->setHorizontalSpacing(1);
	grid1->setContentsMargins(0, 0, 0, 0);
	grid1->addWidget(m_colorTriangle, 0, 3, 5, 3);
	grid1->addWidget(m_setPanBackground, 0, 0);
	grid1->addWidget(m_setWaterfall, 0, 1);
	grid1->addWidget(m_setPanLine, 1, 0);
	grid1->addWidget(m_setPanLineFilling, 1, 1);
	grid1->addWidget(m_setPanSolidTop, 2, 0);
	grid1->addWidget(m_setPanSolidBottom, 2, 1);
	grid1->addWidget(m_setWideBandLine, 3, 0);
	grid1->addWidget(m_setWideBandFilling, 3, 1);
	grid1->addWidget(m_setWideBandSolidTop, 4, 0);
	grid1->addWidget(m_setWideBandSolidBottom, 4, 1);
	grid1->addWidget(m_setDistanceLine, 5, 0);
	grid1->addWidget(m_setDistanceLineFilling, 5, 1);
	grid1->addWidget(m_setPanCenterLine, 6, 0);
	grid1->addWidget(m_setGridLine, 6, 1);
	grid1->addWidget(m_resetBtn, 8, 0);
	grid1->addWidget(m_okBtn, 8, 1);
	//grid1->addWidget(m_resetBtn, 7, 3);
	//grid1->addWidget(m_okBtn, 7, 4);

	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(4);
	hbox1->addStretch();
	hbox1->addWidget(m_resetBtn);
	hbox1->addWidget(m_okBtn);
	
	QVBoxLayout *vbox1 = new QVBoxLayout;
	vbox1->setSpacing(4);
	//vbox1->addStretch();
	vbox1->addWidget(m_colorTriangle);
	vbox1->addLayout(hbox1);
	
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->setSpacing(4);
	hbox->addSpacing(6);
	hbox->addLayout(grid1);
	//hbox->addLayout(vbox1);

	m_colorChooserWidget = new QGroupBox(tr("Color Chooser"), this);
	m_colorChooserWidget->setMinimumWidth(m_minimumGroupBoxWidth);
	m_colorChooserWidget->setLayout(hbox);
	m_colorChooserWidget->setStyleSheet(set->getWidgetStyle());
	m_colorChooserWidget->setFont(QFont("Arial", 8));
}

void GraphicOptionsWidget::createCallSignEditor() {

	callSignLineEdit = new QLineEdit(this);
	callSignLineEdit->setStyleSheet(set->getLineEditStyle());
	callSignLineEdit->setText(set->getCallsign());

	CHECKED_CONNECT(
		callSignLineEdit, 
		SIGNAL(textEdited(const QString &)), 
		this, 
		SLOT(callSignTextChanged(const QString &)));

	m_setCallSignBtn = new AeroButton("Set", this);
	m_setCallSignBtn->setRoundness(0);
	m_setCallSignBtn->setFixedSize(btn_width, btn_height);
	
	CHECKED_CONNECT(
		m_setCallSignBtn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(callSignChanged()));

	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->setSpacing(4);
	//hbox1->addStretch();
	hbox1->addWidget(callSignLineEdit);
	hbox1->addStretch();
	hbox1->addWidget(m_setCallSignBtn);
	//hbox1->addWidget(m_sMeterB);

	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->setSpacing(6);
	vbox->addLayout(hbox1);
	//vbox->addLayout(hbox2);

	m_callSignEditor = new QGroupBox(tr("Call Sign Editor"), this);
	m_callSignEditor->setMinimumWidth(m_minimumGroupBoxWidth);
	m_callSignEditor->setLayout(vbox);
	m_callSignEditor->setStyleSheet(set->getWidgetStyle());
	m_callSignEditor->setFont(QFont("Arial", 8));
}

void GraphicOptionsWidget::panModeChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	int btnHit = m_panadapterBtnList.indexOf(button);

	foreach(AeroButton *btn, m_panadapterBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}

	button->setBtnState(AeroButton::ON);
	button->update();

	switch (btnHit) {

		case 0:
			m_panadapterMode = QSDRGraphics::Line;
			emit messageEvent("[graphics]: set panadapter to line.");
			break;
			
		case 1:
			m_panadapterMode = QSDRGraphics::FilledLine;
			emit messageEvent("[graphics]: set panadapter to filled line.");
			break;

		case 2:
			m_panadapterMode = QSDRGraphics::Solid;
			emit messageEvent("[graphics]: set panadapter to solid.");
			break;
	}

	set->setGraphicsState(this, m_panadapterMode, m_waterColorScheme);
}

void GraphicOptionsWidget::sMeterChanged() {
}

void GraphicOptionsWidget::waterfallSchemeChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	int btnHit = m_waterfallSchemeBtnList.indexOf(button);

	foreach(AeroButton *btn, m_waterfallSchemeBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}

	button->setBtnState(AeroButton::ON);
	button->update();

	switch (btnHit) {

		case 0:
			m_waterColorScheme = QSDRGraphics::simple;
			emit messageEvent("[graphics]: set waterfall scheme to simple.");
			break;
			
		case 1:
			m_waterColorScheme = QSDRGraphics::enhanced;
			emit messageEvent("[graphics]: set waterfall scheme to enhanced.");
			break;

		case 2:
			m_waterColorScheme = QSDRGraphics::spectran;
			emit messageEvent("[graphics]: set waterfall scheme to spectran.");
			break;
	}

	set->setGraphicsState(this, m_panadapterMode, m_waterColorScheme);
}

void GraphicOptionsWidget::resolutionModeChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	int btnHit = m_changeResolutionBtnList.indexOf(button);

	foreach(AeroButton *btn, m_changeResolutionBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}

	button->setBtnState(AeroButton::ON);
	button->update();

	switch (btnHit) {

		case 0:
			m_graphicResolution = 1;
			break;
			
		case 1:
			m_graphicResolution = 2;
			break;

		case 2:
			m_graphicResolution = 4;
			break;
	}

	set->setGraphicResolution(m_graphicResolution);
}

void GraphicOptionsWidget::fpsValueChanged(int value) {

	m_framesPerSecond = value;

	QString str = "%1 ";
	m_fpsLevelLabel->setText(str.arg(m_framesPerSecond, 2, 10, QLatin1Char(' ')));
	set->setFramesPerSecond(this, m_currentReceiver, value);
}

void GraphicOptionsWidget::setFramesPerSecond(QObject *sender, int rx, int value) {

	Q_UNUSED(sender)

	disconnect(m_fpsSlider, SIGNAL(valueChanged(int)), this, SLOT(fpsValueChanged(int)));
	m_fpsSlider->setValue(value);
	m_fpsSlider->update();
	connect(m_fpsSlider, SIGNAL(valueChanged(int)), this, SLOT(fpsValueChanged(int)));

	QString str = "%1 ";
	m_fpsLevelLabel->setText(str.arg(value, 2, 10, QLatin1Char(' ')));
}

void GraphicOptionsWidget::averagingFilterCntChanged(int value) {

	m_avgValue = value;

	QString str = "%1 ";
	m_avgLevelLabel->setText(str.arg(m_avgValue, 2, 10, QLatin1Char(' ')));

	set->setSpectrumAveragingCnt(value);
}

void GraphicOptionsWidget::setAveragingMode() {
	
//	if (m_avgBtn->btnState() == AeroButton::OFF) {
//
//		set->setSpectrumAveraging(true);
//		m_avgBtn->setBtnState(AeroButton::ON);
//		m_avgBtn->setText("On");
//		emit messageEvent("[display]: averaging spectrum display activated.");
//
//	} else {
//
//		set->setSpectrumAveraging(false);
//		m_avgBtn->setBtnState(AeroButton::OFF);
//		m_avgBtn->setText("Off");
//		emit messageEvent("[display]: averaging spectrum display deactivated.");
//	}
}

void GraphicOptionsWidget::colorChooserChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	m_btnChooserHit = m_changeColorBtnList.indexOf(button);

	foreach(AeroButton *btn, m_changeColorBtnList) {

		btn->setBtnState(AeroButton::OFF);
		//btn->setColor(QColor(90, 90, 90));
		btn->update();
	}

	button->setBtnState(AeroButton::ON);
	button->update();

	switch (m_btnChooserHit) {

		case 0:
			m_colorTriangle->setColor(set->getPanadapterColors().panBackgroundColor);
			break;

		case 1:
			m_colorTriangle->setColor(set->getPanadapterColors().waterfallColor);
			break;
			
		case 2:
			m_colorTriangle->setColor(set->getPanadapterColors().panLineColor);
			break;

		case 3:
			m_colorTriangle->setColor(set->getPanadapterColors().panLineFilledColor);
			break;

		case 4:
			m_colorTriangle->setColor(set->getPanadapterColors().panSolidTopColor);
			break;

		case 5:
			m_colorTriangle->setColor(set->getPanadapterColors().panSolidBottomColor);
			break;

		case 6:
			m_colorTriangle->setColor(set->getPanadapterColors().wideBandLineColor);
			break;

		case 7:
			m_colorTriangle->setColor(set->getPanadapterColors().wideBandFilledColor);
			break;

		case 8:
			m_colorTriangle->setColor(set->getPanadapterColors().wideBandSolidTopColor);
			break;

		case 9:
			m_colorTriangle->setColor(set->getPanadapterColors().wideBandSolidBottomColor);
			break;

		case 10:
			m_colorTriangle->setColor(set->getPanadapterColors().distanceLineColor);
			break;

		case 11:
			m_colorTriangle->setColor(set->getPanadapterColors().distanceLineFilledColor);
			break;

		case 12:
			m_colorTriangle->setColor(set->getPanadapterColors().panCenterLineColor);
			break;

		case 13:
			m_colorTriangle->setColor(set->getPanadapterColors().gridLineColor);
			break;
	}
}

void GraphicOptionsWidget::resetColors() {

	m_panadapterColors = m_oldPanadapterColors;

	foreach(AeroButton *btn, m_changeColorBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}
	set->setPanadapterColors(m_panadapterColors);
}

void GraphicOptionsWidget::acceptColors() {

	m_oldPanadapterColors = m_panadapterColors;
	set->setPanadapterColors(m_panadapterColors);
}

void GraphicOptionsWidget::triangleColorChanged(QColor color) {

	m_currentColor = color;
	m_changeColorBtnList.at(m_btnChooserHit)->setColorOn(color);
	m_changeColorBtnList.at(m_btnChooserHit)->update();

	switch (m_btnChooserHit) {

		case 0:
			m_panadapterColors.panBackgroundColor = color;
			break;

		case 1:
			m_panadapterColors.waterfallColor = color;
			break;
			
		case 2:
			m_panadapterColors.panLineColor = color;
			break;

		case 3:
			m_panadapterColors.panLineFilledColor = color;
			break;

		case 4:
			m_panadapterColors.panSolidTopColor = color;
			break;

		case 5:
			m_panadapterColors.panSolidBottomColor = color;
			break;

		case 6:
			m_panadapterColors.wideBandLineColor = color;
			break;

		case 7:
			m_panadapterColors.wideBandFilledColor = color;
			break;

		case 8:
			m_panadapterColors.wideBandSolidTopColor = color;
			break;

		case 9:
			m_panadapterColors.wideBandSolidBottomColor = color;
			break;

		case 10:
			m_panadapterColors.distanceLineColor = color;
			break;

		case 11:
			m_panadapterColors.distanceLineFilledColor = color;
			break;

		case 12:
			m_panadapterColors.panCenterLineColor = color;
			break;

		case 13:
			m_panadapterColors.gridLineColor = color;
			break;
	}
	set->setPanadapterColors(m_panadapterColors);
}

void GraphicOptionsWidget::sampleRateChanged(QObject *sender, int value) {

	Q_UNUSED(sender)

	m_sampleRate = value;
	//int currentValue = m_framesPerSecondSpinBox->value();

	//m_framesPerSecondSpinBox->setMaximum(m_sampleRate/1000);

	//if (currentValue > m_sampleRate/1000) {

	//	m_framesPerSecondSpinBox->setValue(m_sampleRate/1000);
	//	set->setFramesPerSecond(value);
	//}
}

void GraphicOptionsWidget::waterfallTimeChanged(int value) {

	set->setWaterfallTime(m_currentReceiver, value);
}

void GraphicOptionsWidget::waterfallLoOffsetChanged(int value) {

	set->setWaterfallOffesetLo(m_currentReceiver, value);
}

void GraphicOptionsWidget::waterfallHiOffsetChanged(int value) {

	set->setWaterfallOffesetHi(m_currentReceiver, value);
}

void GraphicOptionsWidget::sMeterHoldTimeChanged(int value) {

	set->setSMeterHoldTime(value);
}

void GraphicOptionsWidget::setCurrentReceiver(QObject *sender, int rx) {

	Q_UNUSED(sender)

	if (m_currentReceiver == rx) return;
	m_currentReceiver = rx;
}

void GraphicOptionsWidget::callSignTextChanged(const QString& text) {

	m_callSingText = text;
}

void GraphicOptionsWidget::callSignChanged() {

	set->setCallsign(m_callSingText);
}
