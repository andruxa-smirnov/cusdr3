/**
* @file cusdr_hpsdrWidget.cpp
* @brief Hardware settings widget class for cuSDR
* @author Hermann von Hasseln, DL3HVH
* @version 0.1
* @date 2010-09-21
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
#define LOG_HPSDR_WIDGET // use: HPSDR_WIDGET_DEBUG

#include "cusdr_hpsdrWidget.h"

#define	btn_height		15
#define	btn_width		70 //74
#define	btn_width2		52
#define	btn_widths		42


HPSDRWidget::HPSDRWidget(QWidget *parent) 
	: QWidget(parent)
	, set(Settings::instance())
	, m_serverMode(set->getCurrentServerMode())
	, m_hwInterface(set->getHWInterface())
	, m_hwInterfaceTemp(set->getHWInterface())
	, m_model(set->getModel())
	, m_dataEngineState(QSDR::DataEngineDown)
	, m_minimumWidgetWidth(set->getMinimumWidgetWidth())
	, m_minimumGroupBoxWidth(0)
	, m_numberOfReceivers(1)
	, m_hpsdrHardware(set->getHPSDRHardware())
{
	setMinimumWidth(m_minimumWidgetWidth);
	setContentsMargins(4, 8, 4, 0);
	setMouseTracking(true);
	
	m_firmwareCheck = set->getFirmwareVersionCheck();

	createSource10MhzExclusiveGroup();
	createSource122_88MhzExclusiveGroup();

	QBoxLayout *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	mainLayout->setSpacing(5);
	mainLayout->setMargin(0);
	mainLayout->addSpacing(8);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->setSpacing(0);
	hbox1->setContentsMargins(4, 0, 4, 0);
	hbox1->addWidget(hpsdrHardwareBtnGroup());

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->setSpacing(0);
	hbox2->setContentsMargins(4, 0, 4, 0);
	hbox2->addWidget(source10MhzExclusiveGroup);

	QHBoxLayout *hbox3 = new QHBoxLayout();
	hbox3->setSpacing(0);
	hbox3->setContentsMargins(4, 0, 4, 0);
	hbox3->addWidget(source122_88MhzExclusiveGroup);

	QHBoxLayout *hbox4 = new QHBoxLayout();
	hbox4->setSpacing(0);
	//hbox4->setMargin(0);
	//hbox4->addStretch();
	hbox4->setContentsMargins(4, 0, 4, 0);
	hbox4->addWidget(sampleRateExclusiveGroup());

	/*QHBoxLayout *hbox5 = new QHBoxLayout();
	hbox5->setSpacing(0);
	hbox5->setContentsMargins(4, 0, 4, 0);
	hbox5->addWidget(numberOfReceiversGroup());*/

	mainLayout->addLayout(hbox1);
	mainLayout->addLayout(hbox2);
	mainLayout->addLayout(hbox3);
	mainLayout->addLayout(hbox4);
	//mainLayout->addLayout(hbox5);
	mainLayout->addStretch();
	setLayout(mainLayout);

	setupConnections();
	setHardware();
}

HPSDRWidget::~HPSDRWidget() {

	disconnect(set, 0, this, 0);
	disconnect(this, 0, 0, 0);
}

void HPSDRWidget::setupConnections() {

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
}

QGroupBox* HPSDRWidget::hpsdrHardwareBtnGroup() {

	atlasPresenceBtn = new AeroButton("Atlas", this);
	atlasPresenceBtn->setRoundness(0);
	atlasPresenceBtn->setFixedSize(btn_width, btn_height);
	atlasPresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(atlasPresenceBtn);
	
	CHECKED_CONNECT(
		atlasPresenceBtn,
		SIGNAL(released()), 
		this, 
		SLOT(hardwareChanged()));

	/*hermesPresenceBtn = new AeroButton("Hermes/Anan", this);
	hermesPresenceBtn->setRoundness(0);
	hermesPresenceBtn->setFixedSize(btn_width, btn_height);
	hermesPresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(hermesPresenceBtn);
	
	CHECKED_CONNECT(
		hermesPresenceBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(hardwareChanged()));*/

	penelopePresenceBtn = new AeroButton("Penelope", this);
	penelopePresenceBtn->setRoundness(0);
	penelopePresenceBtn->setFixedSize(btn_width, btn_height);
	penelopePresenceBtn->setBtnState(AeroButton::OFF);
	
	CHECKED_CONNECT(
		penelopePresenceBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(penelopePresenceChanged()));

	pennyPresenceBtn = new AeroButton("Pennylane", this);
	pennyPresenceBtn->setRoundness(0);
	pennyPresenceBtn->setFixedSize(btn_width, btn_height);
	pennyPresenceBtn->setBtnState(AeroButton::OFF);

	CHECKED_CONNECT(
		pennyPresenceBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(pennyPresenceChanged()));
	
	mercuryPresenceBtn = new AeroButton("Mercury", this);
	mercuryPresenceBtn->setRoundness(0);
	mercuryPresenceBtn->setFixedSize(btn_width, btn_height);
	mercuryPresenceBtn->setBtnState(AeroButton::OFF);
	
	CHECKED_CONNECT(
		mercuryPresenceBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(mercuryPresenceChanged()));

	alexPresenceBtn = new AeroButton("Alex", this);
	alexPresenceBtn->setRoundness(0);
	alexPresenceBtn->setFixedSize(btn_width, btn_height);
	alexPresenceBtn->setBtnState(AeroButton::OFF);
	
	CHECKED_CONNECT(
		alexPresenceBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(alexPresenceChanged()));
	
	excaliburPresenceBtn = new AeroButton("Excalibur", this);
	excaliburPresenceBtn->setRoundness(0);
	excaliburPresenceBtn->setFixedSize(btn_width, btn_height);
	excaliburPresenceBtn->setBtnState(AeroButton::OFF);
	
	CHECKED_CONNECT(
		excaliburPresenceBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(excaliburPresenceChanged()));
	

	anan10PresenceBtn = new AeroButton("Anan 10", this);
	anan10PresenceBtn->setRoundness(0);
	anan10PresenceBtn->setFixedHeight(btn_height);
	anan10PresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(anan10PresenceBtn);

	CHECKED_CONNECT(
		anan10PresenceBtn,
		SIGNAL(released()),
		this,
		SLOT(hardwareChanged()));

	anan10EPresenceBtn = new AeroButton("Anan 10E", this);
	anan10EPresenceBtn->setRoundness(0);
	anan10EPresenceBtn->setFixedHeight(btn_height);
	anan10EPresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(anan10EPresenceBtn);

	CHECKED_CONNECT(
		anan10EPresenceBtn,
		SIGNAL(released()),
		this,
		SLOT(hardwareChanged()));

	anan100PresenceBtn = new AeroButton("Anan 100", this);
	anan100PresenceBtn->setRoundness(0);
	anan100PresenceBtn->setFixedHeight(btn_height);
	anan100PresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(anan100PresenceBtn);

	CHECKED_CONNECT(
		anan100PresenceBtn,
		SIGNAL(released()),
		this,
		SLOT(hardwareChanged()));

	anan100DPresenceBtn = new AeroButton("Anan 100D", this);
	anan100DPresenceBtn->setRoundness(0);
	anan100DPresenceBtn->setFixedHeight(btn_height);
	anan100DPresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(anan100DPresenceBtn);

	CHECKED_CONNECT(
		anan100DPresenceBtn,
		SIGNAL(released()),
		this,
		SLOT(hardwareChanged()));

	anan200DPresenceBtn = new AeroButton("Anan 200D", this);
	anan200DPresenceBtn->setRoundness(0);
	anan200DPresenceBtn->setFixedHeight(btn_height);
	anan200DPresenceBtn->setBtnState(AeroButton::OFF);
	hardwareBtnList.append(anan200DPresenceBtn);

	CHECKED_CONNECT(
		anan200DPresenceBtn,
		SIGNAL(released()),
		this,
		SLOT(hardwareChanged()));

	//***************************************************************
	firmwareCheckBtn = new AeroButton("On", this);
	firmwareCheckBtn->setRoundness(0);
	firmwareCheckBtn->setFixedSize(btn_widths, btn_height);

	if (m_firmwareCheck) {

		firmwareCheckBtn->setBtnState(AeroButton::ON);
		firmwareCheckBtn->setText("On");
	}
	else {

		firmwareCheckBtn->setBtnState(AeroButton::OFF);
		firmwareCheckBtn->setText("Off");
	}

	CHECKED_CONNECT(
		firmwareCheckBtn,
		SIGNAL(released()),
		this,
		SLOT(firmwareCheckChanged()));

	m_fwCheckLabel = new QLabel("Firmware Check:", this);
	m_fwCheckLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_fwCheckLabel->setStyleSheet(set->getLabelStyle());
	
	QHBoxLayout *hbox0 = new QHBoxLayout();
	hbox0->setSpacing(4);
	hbox0->addStretch();
	hbox0->addWidget(m_fwCheckLabel);
	hbox0->addWidget(firmwareCheckBtn);

	QGridLayout *gridLayout = new QGridLayout();
	gridLayout->setVerticalSpacing(4);
	gridLayout->setHorizontalSpacing(2);
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->addWidget(atlasPresenceBtn, 0, 0);
	gridLayout->addWidget(mercuryPresenceBtn, 0, 1);
	gridLayout->addWidget(penelopePresenceBtn, 0, 2);
	gridLayout->addWidget(pennyPresenceBtn, 1, 1);
	gridLayout->addWidget(excaliburPresenceBtn, 1, 2);
	gridLayout->addWidget(alexPresenceBtn, 2, 1);
	gridLayout->addWidget(anan10PresenceBtn, 3, 0);
	gridLayout->addWidget(anan10EPresenceBtn, 3, 1);
	gridLayout->addWidget(anan100PresenceBtn, 4, 0);
	gridLayout->addWidget(anan100DPresenceBtn, 4, 1);
	gridLayout->addWidget(anan200DPresenceBtn, 4, 2);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->setSpacing(4);
	hbox1->addStretch();
	hbox1->addLayout(gridLayout);

	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSpacing(4);
	vbox->addSpacing(6);
	vbox->addLayout(hbox0);
	vbox->addSpacing(6);
	vbox->addLayout(hbox1);

	QGroupBox *groupBox = new QGroupBox(tr("Hardware selection"), this);
	groupBox->setMinimumWidth(m_minimumGroupBoxWidth);
	groupBox->setLayout(vbox);
	groupBox->setStyleSheet(set->getWidgetStyle());
	groupBox->setFont(QFont("Arial", 8));

	return groupBox;
}

void HPSDRWidget::createSource10MhzExclusiveGroup() {

	atlasBtn = new AeroButton("Atlas", this);
	atlasBtn->setRoundness(0);
	atlasBtn->setFixedSize(btn_width, btn_height);
	source10MhzBtnList.append(atlasBtn);

	CHECKED_CONNECT(
		atlasBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(source10MhzChanged()));
	
	penelopeBtn = new AeroButton("Penny[Lane]", this);
	penelopeBtn->setRoundness(0);
	penelopeBtn->setFixedSize(btn_width, btn_height);
	source10MhzBtnList.append(penelopeBtn);

	CHECKED_CONNECT(
		penelopeBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(source10MhzChanged()));
	
	mercuryBtn = new AeroButton("Mercury", this);
	mercuryBtn->setRoundness(0);
	mercuryBtn->setFixedSize(btn_width, btn_height);
	source10MhzBtnList.append(mercuryBtn);

	CHECKED_CONNECT(
		mercuryBtn, 
		SIGNAL(released()), 
		this, 
		SLOT(source10MhzChanged()));
	
	
	sources10Mhz << "Atlas" << "Penelope" << "Mercury";

	switch(set->get10MHzSource()) {

		case 0:
			atlasBtn->setBtnState(AeroButton::ON);
			penelopeBtn->setBtnState(AeroButton::OFF);
			mercuryBtn->setBtnState(AeroButton::OFF);
			break;
			
		case 1:
			penelopeBtn->setBtnState(AeroButton::ON);
			atlasBtn->setBtnState(AeroButton::OFF);
			mercuryBtn->setBtnState(AeroButton::OFF);
			break;

		case 2:
			mercuryBtn->setBtnState(AeroButton::ON);
			atlasBtn->setBtnState(AeroButton::OFF);
			penelopeBtn->setBtnState(AeroButton::OFF);
			break;

		case 3:
			mercuryBtn->setBtnState(AeroButton::OFF);
			atlasBtn->setBtnState(AeroButton::OFF);
			penelopeBtn->setBtnState(AeroButton::OFF);
			break;
	}
	
	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->setSpacing(4);
	hbox1->addStretch();
	hbox1->addWidget(penelopeBtn);
	hbox1->addWidget(mercuryBtn);
	
	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->setSpacing(4);
	hbox2->addStretch();
	hbox2->addWidget(atlasBtn);
	
	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSpacing(4);
	vbox->addSpacing(6);
	vbox->addLayout(hbox1);
	vbox->addLayout(hbox2);
	
	source10MhzExclusiveGroup = new QGroupBox(tr("10 MHz Clock"), this);
	source10MhzExclusiveGroup->setMinimumWidth(m_minimumGroupBoxWidth);
	source10MhzExclusiveGroup->setLayout(vbox);
	source10MhzExclusiveGroup->setStyleSheet(set->getWidgetStyle());
	source10MhzExclusiveGroup->setFont(QFont("Arial", 8));
}

void HPSDRWidget::createSource122_88MhzExclusiveGroup() {

	penelope2Btn = new AeroButton("Penny[Lane]", this);
	penelope2Btn->setRoundness(0);
	penelope2Btn->setFixedSize(btn_width, btn_height);

	CHECKED_CONNECT(
		penelope2Btn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(source122_88MhzChanged()));

	mercury2Btn = new AeroButton("Mercury", this);
	mercury2Btn->setRoundness(0);
	mercury2Btn->setFixedSize(btn_width, btn_height);

	CHECKED_CONNECT(
		mercury2Btn, 
		SIGNAL(clicked()), 
		this, 
		SLOT(source122_88MhzChanged()));

	switch(set->get122_8MHzSource()) {

		case 0:
			penelope2Btn->setBtnState(AeroButton::ON);
			mercury2Btn->setBtnState(AeroButton::OFF);
			break;

		case 1:
			mercury2Btn->setBtnState(AeroButton::ON);
			penelope2Btn->setBtnState(AeroButton::OFF);
			break;
	}
	
	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->setSpacing(4);
	hbox1->addStretch();
	hbox1->addWidget(penelope2Btn);
	hbox1->addWidget(mercury2Btn);
	
	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSpacing(4);
	vbox->addSpacing(6);
	vbox->addLayout(hbox1);
	
	source122_88MhzExclusiveGroup = new QGroupBox(tr("122.8 MHz Clock"), this);
	source122_88MhzExclusiveGroup->setMinimumWidth(m_minimumGroupBoxWidth);
	source122_88MhzExclusiveGroup->setLayout(vbox);
	source122_88MhzExclusiveGroup->setStyleSheet(set->getWidgetStyle());
	source122_88MhzExclusiveGroup->setFont(QFont("Arial", 8));
}

QGroupBox *HPSDRWidget::sampleRateExclusiveGroup() {

	samplerate48Btn = new AeroButton("48 kHz", this);
	samplerate48Btn->setRoundness(0);
	samplerate48Btn->setFixedSize (50, btn_height);
	samplerate48Btn->setStyleSheet(set->getMiniButtonStyle());
	samplerateBtnList.append(samplerate48Btn);
	CHECKED_CONNECT(samplerate48Btn, SIGNAL(released()), this, SLOT(sampleRateChanged()));

	samplerate96Btn = new AeroButton("96 kHz", this);
	samplerate96Btn->setRoundness(0);
	samplerate96Btn->setFixedSize (50, btn_height);
	samplerate96Btn->setStyleSheet(set->getMiniButtonStyle());
	samplerateBtnList.append(samplerate96Btn);
	CHECKED_CONNECT(samplerate96Btn, SIGNAL(released()), this, SLOT(sampleRateChanged()));

	samplerate192Btn = new AeroButton("192 kHz", this);
	samplerate192Btn->setRoundness(0);
	samplerate192Btn->setFixedSize (50, btn_height);
	samplerate192Btn->setStyleSheet(set->getMiniButtonStyle());
	samplerateBtnList.append(samplerate192Btn);
	CHECKED_CONNECT(samplerate192Btn, SIGNAL(released()), this, SLOT(sampleRateChanged()));

	samplerate384Btn = new AeroButton("384 kHz", this);
	samplerate384Btn->setRoundness(0);
	samplerate384Btn->setFixedSize (50, btn_height);
	samplerate384Btn->setStyleSheet(set->getMiniButtonStyle());
	samplerateBtnList.append(samplerate384Btn);
	CHECKED_CONNECT(samplerate384Btn, SIGNAL(released()), this, SLOT(sampleRateChanged()));

	switch(set->getSampleRate()) {

		case 48000:
			samplerate48Btn->setBtnState(AeroButton::ON);
			samplerate96Btn->setBtnState(AeroButton::OFF);
			samplerate192Btn->setBtnState(AeroButton::OFF);
			samplerate384Btn->setBtnState(AeroButton::OFF);
			break;

		case 96000:
			samplerate48Btn->setBtnState(AeroButton::OFF);
			samplerate96Btn->setBtnState(AeroButton::ON);
			samplerate192Btn->setBtnState(AeroButton::OFF);
			samplerate384Btn->setBtnState(AeroButton::OFF);
			break;

		case 192000:
			samplerate48Btn->setBtnState(AeroButton::OFF);
			samplerate96Btn->setBtnState(AeroButton::OFF);
			samplerate192Btn->setBtnState(AeroButton::ON);
			samplerate384Btn->setBtnState(AeroButton::OFF);
			break;

		case 384000:
			samplerate48Btn->setBtnState(AeroButton::OFF);
			samplerate96Btn->setBtnState(AeroButton::OFF);
			samplerate192Btn->setBtnState(AeroButton::OFF);
			samplerate384Btn->setBtnState(AeroButton::ON);
			break;
	}

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing(4);
	hbox->addStretch();
	hbox->addWidget(samplerate48Btn);
	hbox->addWidget(samplerate96Btn);
	hbox->addWidget(samplerate192Btn);
	hbox->addWidget(samplerate384Btn);

	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSpacing(4);
	vbox->addSpacing(6);
	vbox->addLayout(hbox);

	QGroupBox *groupBox = new QGroupBox(tr("Sample Rate"), this);
	groupBox->setMinimumWidth(m_minimumGroupBoxWidth);
	groupBox->setLayout(vbox);
	groupBox->setStyleSheet(set->getWidgetStyle());
	groupBox->setFont(QFont("Arial", 8));

	return groupBox;
}

QGroupBox *HPSDRWidget::numberOfReceiversGroup() {

	m_receiverComboBox = new QComboBox(this);
	m_receiverComboBox->setStyleSheet(set->getComboBoxStyle());
	m_receiverComboBox->setMinimumContentsLength(4);

	QString str = "%1";
	for (int i = 0; i < set->getMaxNumberOfReceivers(); i++)
		m_receiverComboBox->addItem(str.arg(i+1));

	/*CHECKED_CONNECT(
		m_receiverComboBox,
		SIGNAL(currentIndexChanged(int)),
		this,
		SLOT(setNumberOfReceivers(int)));*/

	m_receiversLabel = new QLabel("Receivers:", this);
    m_receiversLabel->setFrameStyle(QFrame::Box | QFrame::Raised);
	m_receiversLabel->setStyleSheet(set->getLabelStyle());
	

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->setSpacing(5);
	//hbox1->addStretch();
	hbox1->addWidget(m_receiversLabel);
	hbox1->addStretch();
	hbox1->addWidget(m_receiverComboBox);
	
	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSpacing(4);
	vbox->addSpacing(6);
	vbox->addLayout(hbox1);
	
	QGroupBox *groupBox = new QGroupBox(tr("Number of Receivers"), this);
	groupBox->setMinimumWidth(m_minimumGroupBoxWidth);
	groupBox->setLayout(vbox);
	groupBox->setStyleSheet(set->getWidgetStyle());
	groupBox->setFont(QFont("Arial", 8));

	return groupBox;
}

// ************************************************************************

void HPSDRWidget::setHardware() {

	switch (m_model) {

		case 0: // no hardware
			break;

		case 1: // Atlas modules

			atlasPresenceBtn->setBtnState(AeroButton::ON);
			anan10PresenceBtn->setBtnState(AeroButton::OFF);
			anan10EPresenceBtn->setBtnState(AeroButton::OFF);
			anan100PresenceBtn->setBtnState(AeroButton::OFF);
			anan100DPresenceBtn->setBtnState(AeroButton::OFF);
			anan200DPresenceBtn->setBtnState(AeroButton::OFF);

			enableAtlasModules();
			break;

		case 2: // Anan 10
			
			atlasPresenceBtn->setBtnState(AeroButton::OFF);
			anan10PresenceBtn->setBtnState(AeroButton::ON);
			anan10EPresenceBtn->setBtnState(AeroButton::OFF);
			anan100PresenceBtn->setBtnState(AeroButton::OFF);
			anan100DPresenceBtn->setBtnState(AeroButton::OFF);
			anan200DPresenceBtn->setBtnState(AeroButton::OFF);
		
			disableAtlasModules();
			break;

		case 3: // Anan 10E

			atlasPresenceBtn->setBtnState(AeroButton::OFF);
			anan10PresenceBtn->setBtnState(AeroButton::OFF);
			anan10EPresenceBtn->setBtnState(AeroButton::ON);
			anan100PresenceBtn->setBtnState(AeroButton::OFF);
			anan100DPresenceBtn->setBtnState(AeroButton::OFF);
			anan200DPresenceBtn->setBtnState(AeroButton::OFF);

			disableAtlasModules();
			break;

		case 4: // Anan 100

			atlasPresenceBtn->setBtnState(AeroButton::OFF);
			anan10PresenceBtn->setBtnState(AeroButton::OFF);
			anan10EPresenceBtn->setBtnState(AeroButton::OFF);
			anan100PresenceBtn->setBtnState(AeroButton::ON);
			anan100DPresenceBtn->setBtnState(AeroButton::OFF);
			anan200DPresenceBtn->setBtnState(AeroButton::OFF);

			disableAtlasModules();
			break;

		case 5: // Anan 100D

			atlasPresenceBtn->setBtnState(AeroButton::OFF);
			anan10PresenceBtn->setBtnState(AeroButton::OFF);
			anan10EPresenceBtn->setBtnState(AeroButton::OFF);
			anan100PresenceBtn->setBtnState(AeroButton::OFF);
			anan100DPresenceBtn->setBtnState(AeroButton::ON);
			anan200DPresenceBtn->setBtnState(AeroButton::OFF);

			disableAtlasModules();
			break;

		case 6: // Anan 200D

			atlasPresenceBtn->setBtnState(AeroButton::OFF);
			anan10PresenceBtn->setBtnState(AeroButton::OFF);
			anan10EPresenceBtn->setBtnState(AeroButton::OFF);
			anan100PresenceBtn->setBtnState(AeroButton::OFF);
			anan100DPresenceBtn->setBtnState(AeroButton::OFF);
			anan200DPresenceBtn->setBtnState(AeroButton::ON);

			disableAtlasModules();
			break;
	}

	if (set->getAlexPresence() || m_model == QSDR::anan100 || m_model == QSDR::anan100D || m_model == QSDR::anan200D)
		alexPresenceBtn->setBtnState(AeroButton::ON);
	else if (!set->getAlexPresence())
		alexPresenceBtn->setBtnState(AeroButton::OFF);
}

void HPSDRWidget::hardwareChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	int btn = hardwareBtnList.indexOf(button);

	foreach(AeroButton *btn, hardwareBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}

	button->setBtnState(AeroButton::ON);
	button->update();

	m_model = (QSDR::_SDRModel) (btn+1);
	set->setModel(m_model);

	setHardware();

	switch (m_model) {

	case 0: // no hardware
		break;

	case 1:	// Atlas modules
		source10MhzExclusiveGroup->show();
		source122_88MhzExclusiveGroup->show();

		m_hwInterface = QSDR::Metis;
		set->setModel(QSDR::atlas);
		set->setMaxNumberOfReceivers(4);

		emit messageEvent("[hw]: HPSDR Atlas chosen.");
		break;

	case 2: // Anan 10
		source10MhzExclusiveGroup->hide();
		source122_88MhzExclusiveGroup->hide();
		set->setModel(QSDR::anan10);
		set->setMaxNumberOfReceivers(7);

		m_hwInterface = QSDR::Hermes;
		emit messageEvent("[hw]: Anan10 chosen.");
		break;

	case 3: // Anan10E model
		source10MhzExclusiveGroup->hide();
		source122_88MhzExclusiveGroup->hide();
		set->setModel(QSDR::anan10E);
		set->setMaxNumberOfReceivers(2);

		m_hwInterface = QSDR::Hermes;
		emit messageEvent("[hw]: Anan10E chosen.");

		break;

	case 4: // Anan100 model
		source10MhzExclusiveGroup->hide();
		source122_88MhzExclusiveGroup->hide();
		set->setModel(QSDR::anan100);
		set->setMaxNumberOfReceivers(4);

		m_hwInterface = QSDR::Hermes;
		emit messageEvent("[hw]: Anan100 chosen.");
		break;

	case 5: // Anan100D model
		source10MhzExclusiveGroup->hide();
		source122_88MhzExclusiveGroup->hide();
		set->setModel(QSDR::anan100D);
		set->setMaxNumberOfReceivers(7);

		m_hwInterface = QSDR::Hermes;
		emit messageEvent("[hw]: Anan100D chosen.");
		break;

	case 6: // Anan200D model
		source10MhzExclusiveGroup->hide();
		source122_88MhzExclusiveGroup->hide();
		set->setModel(QSDR::anan200D);
		set->setMaxNumberOfReceivers(7);

		m_hwInterface = QSDR::Hermes;
		emit messageEvent("[hw]: Anan200D chosen.");
		break;
	}

	set->setSystemState(
		this,
		QSDR::NoError,
		m_hwInterface,
		m_serverMode,
		m_dataEngineState);
}

void HPSDRWidget::enableAtlasModules() {

	mercuryPresenceBtn->setEnabled(true);
	if (set->getMercuryPresence())
		mercuryPresenceBtn->setBtnState(AeroButton::ON);
	else
		mercuryPresenceBtn->setBtnState(AeroButton::OFF);

	penelopePresenceBtn->setEnabled(true);
	pennyPresenceBtn->setEnabled(true);

	if (set->getPenelopePresence()) {

		penelopePresenceBtn->setBtnState(AeroButton::ON);
		pennyPresenceBtn->setBtnState(AeroButton::OFF);
	}
	else if (set->getPennyLanePresence()) {

		penelopePresenceBtn->setBtnState(AeroButton::OFF);
		pennyPresenceBtn->setBtnState(AeroButton::ON);
	}
	else {

		penelopePresenceBtn->setBtnState(AeroButton::OFF);
		pennyPresenceBtn->setBtnState(AeroButton::OFF);
	}

	excaliburPresenceBtn->setEnabled(true);
	if (set->getExcaliburPresence()) {

		set->set10MhzSource(this, 3); // none
		mercuryBtn->setBtnState(AeroButton::OFF);
		atlasBtn->setBtnState(AeroButton::OFF);
		penelopeBtn->setBtnState(AeroButton::OFF);
		mercuryBtn->setEnabled(false);
		penelopeBtn->setEnabled(false);
		atlasBtn->setEnabled(false);

		excaliburPresenceBtn->setBtnState(AeroButton::ON);
	}
	else {

		switch (set->get10MHzSource()) {

		case 0:
			atlasBtn->setBtnState(AeroButton::ON);
			penelopeBtn->setBtnState(AeroButton::OFF);
			mercuryBtn->setBtnState(AeroButton::OFF);
			break;

		case 1:
			penelopeBtn->setBtnState(AeroButton::ON);
			atlasBtn->setBtnState(AeroButton::OFF);
			mercuryBtn->setBtnState(AeroButton::OFF);
			break;

		case 2:
			mercuryBtn->setBtnState(AeroButton::ON);
			atlasBtn->setBtnState(AeroButton::OFF);
			penelopeBtn->setBtnState(AeroButton::OFF);
			break;

		case 3:
			mercuryBtn->setBtnState(AeroButton::OFF);
			atlasBtn->setBtnState(AeroButton::OFF);
			penelopeBtn->setBtnState(AeroButton::OFF);
			break;
		}

		excaliburPresenceBtn->setBtnState(AeroButton::OFF);
	}
}

void HPSDRWidget::disableAtlasModules() {

	penelopePresenceBtn->setBtnState(AeroButton::OFF);
	penelopePresenceBtn->setEnabled(false);

	pennyPresenceBtn->setBtnState(AeroButton::OFF);
	pennyPresenceBtn->setEnabled(false);

	mercuryPresenceBtn->setBtnState(AeroButton::OFF);
	mercuryPresenceBtn->setEnabled(false);

	excaliburPresenceBtn->setBtnState(AeroButton::OFF);
	excaliburPresenceBtn->setEnabled(false);

	set->set10MhzSource(this, 2); // none
	source10MhzExclusiveGroup->hide();
	source122_88MhzExclusiveGroup->hide();
}

//void HPSDRWidget::ananHardwareChanged() {
//
//	AeroButton *button = qobject_cast<AeroButton *>(sender());
//	int btn = ananBtnList.indexOf(button);
//
//	foreach(AeroButton *btn, ananBtnList) {
//
//		btn->setBtnState(AeroButton::OFF);
//		btn->update();
//	}
//
//	button->setBtnState(AeroButton::ON);
//	button->update();
//
//	//set->setAnanModel((QSDR::AnanModel) btn);
//
//	if (btn > 0 && alexPresenceBtn->btnState() == AeroButton::OFF) {
//
//		set->setAlexPresence(true);
//		alexPresenceBtn->setBtnState(AeroButton::ON);
//		alexPresenceBtn->update();
//		emit messageEvent("[hw]: Alex added.");
//	}
//
//	QString str;
//	switch (btn) {
//
//	case 0:
//		str = "[hw]: Anan 10 selected.";
//		set->setBoard(QSDR::hermes);
//		break;
//
//	case 1:
//		str = "[hw]: Anan 10E selected.";
//		set->setBoard(QSDR::hermes14);
//		break;
//
//	case 2:
//		str = "[hw]: Anan 100 selected.";
//		set->setBoard(QSDR::hermes);
//		break;
//
//	case 3:
//		str = "[hw]: Anan 100D selected.";
//		set->setBoard(QSDR::angelia);
//		break;
//
//	case 4:
//		str = "[hw]: Anan 200D selected.";
//		set->setBoard(QSDR::orion);
//		break;
//
//	default:
//		str = "";
//		break;
//	}
//
//	//_set->setAnanModel((QSDR::AnanModel) btn);
//
//	emit messageEvent(str);
//}

void HPSDRWidget::systemStateChanged(
	QObject *sender, 
	QSDR::_Error err, 
	QSDR::_HWInterfaceMode hwmode, 
	QSDR::_ServerMode mode, 
	QSDR::_DataEngineState state)
{
	Q_UNUSED (sender)
	Q_UNUSED (err)

	if (m_hwInterface != hwmode)
		m_hwInterface = hwmode;

	//m_oldServerMode = m_serverMode;
	if (m_serverMode != mode) {

		if (mode == QSDR::ChirpWSPR)
			disableButtons();

		if (m_serverMode == QSDR::ChirpWSPR)
			enableButtons();
		
		m_serverMode = mode;
	}
		
	if (m_dataEngineState != state) {

		if (state == QSDR::DataEngineUp)
			disableButtons();
		else
			enableButtons();

		m_dataEngineState = state;
	}
	update();
}


void HPSDRWidget::penelopePresenceChanged() {

	if (penelopePresenceBtn->btnState() == AeroButton::OFF) {
		
		if (pennyPresenceBtn->btnState() == AeroButton::ON) {

			set->setPennyLanePresence(false);
			pennyPresenceBtn->setBtnState(AeroButton::OFF);
			pennyPresenceBtn->update();

			emit messageEvent("[hpsdr]: PennyLane removed");
		}
		set->setPenelopePresence(true);
		penelopePresenceBtn->setBtnState(AeroButton::ON);

		emit messageEvent("[hpsdr]: Penelope added");

	} else {

		set->setPenelopePresence(false);
		penelopePresenceBtn->setBtnState(AeroButton::OFF);
		emit messageEvent("[hpsdr]: Penelope removed.");
	}
}

void HPSDRWidget::pennyPresenceChanged() {

	if (pennyPresenceBtn->btnState() == AeroButton::OFF) {
		
		if (penelopePresenceBtn->btnState() == AeroButton::ON) {

			set->setPenelopePresence(false);
			penelopePresenceBtn->setBtnState(AeroButton::OFF);
			penelopePresenceBtn->update();

			emit messageEvent("[hpsdr]: Penelope removed");
		}
		set->setPennyLanePresence(true);
		pennyPresenceBtn->setBtnState(AeroButton::ON);

		emit messageEvent("[hpsdr]: PennyLane added");

	} else {

		set->setPennyLanePresence(false);
		pennyPresenceBtn->setBtnState(AeroButton::OFF);
		emit messageEvent("[hpsdr]: PennyLane removed.");
	}
}

void HPSDRWidget::mercuryPresenceChanged() {

	if (mercuryPresenceBtn->btnState() == AeroButton::OFF) {
		
		set->setMercuryPresence(true);
		mercuryPresenceBtn->setBtnState(AeroButton::ON);
		emit messageEvent("[hpsdr]: Mercury added.");

	} else {

		set->setMercuryPresence(false);
		mercuryPresenceBtn->setBtnState(AeroButton::OFF);
		emit messageEvent("[hpsdr]: Mercury removed.");
	}
}

void HPSDRWidget::alexPresenceChanged() {

	if (alexPresenceBtn->btnState() == AeroButton::OFF) {
		
		set->setAlexPresence(true);
		alexPresenceBtn->setBtnState(AeroButton::ON);
		emit messageEvent("[hpsdr]: Alex added.");

	} else {

		set->setAlexPresence(false);
		alexPresenceBtn->setBtnState(AeroButton::OFF);
		emit messageEvent("[hpsdr]: Alex removed.");
	}
}

void HPSDRWidget::excaliburPresenceChanged() {

	if (excaliburPresenceBtn->btnState() == AeroButton::OFF) {
		
		set->set10MhzSource(this, 3); // None
		
		mercuryBtn->setBtnState(AeroButton::OFF);
		atlasBtn->setBtnState(AeroButton::OFF);
		penelopeBtn->setBtnState(AeroButton::OFF);
		
		mercuryBtn->setEnabled(false);
		penelopeBtn->setEnabled(false);
		atlasBtn->setEnabled(false);

		set->setExcaliburPresence(true);
		excaliburPresenceBtn->setBtnState(AeroButton::ON);
		emit messageEvent("[hpsdr]: Excalibur added.");

	} else {

		set->set10MhzSource(this, 2); // Mercury
		
		mercuryBtn->setEnabled(true);
		penelopeBtn->setEnabled(true);
		atlasBtn->setEnabled(true);

		mercuryBtn->setBtnState(AeroButton::ON);
		mercuryBtn->update();
		atlasBtn->setBtnState(AeroButton::OFF);
		penelopeBtn->setBtnState(AeroButton::OFF);

		set->setExcaliburPresence(false);
		excaliburPresenceBtn->setBtnState(AeroButton::OFF);
		emit messageEvent("[hpsdr]: Excalibur removed.");
	}
}

void HPSDRWidget::firmwareCheckChanged() {

	if (firmwareCheckBtn->btnState() == AeroButton::OFF) {

		set->setCheckFirmwareVersion(this, true);
		firmwareCheckBtn->setBtnState(AeroButton::ON);
		firmwareCheckBtn->setText("On");

	} else {

		set->setCheckFirmwareVersion(this, false);
		firmwareCheckBtn->setBtnState(AeroButton::OFF);
		firmwareCheckBtn->setText("Off");
	}
}

void HPSDRWidget::source10MhzChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	int btn = source10MhzBtnList.indexOf(button);
	
	foreach(AeroButton *btn, source10MhzBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}

	set->set10MhzSource(this, btn);
	button->setBtnState(AeroButton::ON);
	button->update();

	QString msg = "[hpsdr]: 10 MHz source changed to %1";
	emit messageEvent(msg.arg(sources10Mhz.at(btn)));
}

void HPSDRWidget::source122_88MhzChanged() {
	
	switch (set->get122_8MHzSource()) {

		// penelope 0, mercury 1
		case 0:
			penelope2Btn->setBtnState(AeroButton::OFF);
			penelope2Btn->update();

			set->set122_88MhzSource(this, 1);
			emit messageEvent("[hpsdr]: 122.88 MHz source changed to Mercury.");
			mercury2Btn->setBtnState(AeroButton::ON);
			break;
			
		case 1:
			mercury2Btn->setBtnState(AeroButton::OFF);
			mercury2Btn->update();

			set->set122_88MhzSource(this, 0);
			emit messageEvent("[hpsdr]: 122.88 MHz source changed to Penelope.");
			penelope2Btn->setBtnState(AeroButton::ON);
			break;
	}
}

void HPSDRWidget::sampleRateChanged() {

	AeroButton *button = qobject_cast<AeroButton *>(sender());
	int btnHit = samplerateBtnList.indexOf(button);

	foreach (AeroButton *btn, samplerateBtnList) {

		btn->setBtnState(AeroButton::OFF);
		btn->update();
	}

	button->setBtnState(AeroButton::ON);
	button->update();

	switch (btnHit) {

		case 0:
			set->setSampleRate(this, 48000);
			HPSDR_WIDGET_DEBUG << "set sample rate to 48 kHz.";
			break;
			
		case 1:
			set->setSampleRate(this, 96000);
			HPSDR_WIDGET_DEBUG << "set sample rate to 96 kHz.";
			break;

		case 2:
			set->setSampleRate(this, 192000);
			HPSDR_WIDGET_DEBUG << "set sample rate to 192 kHz.";
			break;

		case 3:
			set->setSampleRate(this, 384000);
			HPSDR_WIDGET_DEBUG << "set sample rate to 384 kHz.";
			break;
	}
}

//void HPSDRWidget::setNumberOfReceivers(int receivers) {
//
//	m_numberOfReceivers = receivers+1;
//	set->setReceivers(this, m_numberOfReceivers);
//}

void HPSDRWidget::disableButtons() {

	disconnect(atlasPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	disconnect(penelopePresenceBtn,	SIGNAL(released()),	this, SLOT(penelopePresenceChanged()));
	disconnect(pennyPresenceBtn, SIGNAL(released()), this, SLOT(pennyPresenceChanged()));
	disconnect(mercuryPresenceBtn, SIGNAL(released()), this, SLOT(mercuryPresenceChanged()));
	disconnect(alexPresenceBtn,	SIGNAL(released()), this, SLOT(alexPresenceChanged()));
	disconnect(excaliburPresenceBtn, SIGNAL(released()), this, SLOT(excaliburPresenceChanged()));
	disconnect(anan10PresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	disconnect(anan10EPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	disconnect(anan100PresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	disconnect(anan100DPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	disconnect(anan200DPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	disconnect(firmwareCheckBtn, SIGNAL(released()), this, SLOT(firmwareCheckChanged()));
}

void HPSDRWidget::enableButtons() {

	connect(atlasPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	connect(penelopePresenceBtn, SIGNAL(released()), this, SLOT(penelopePresenceChanged()));
	connect(pennyPresenceBtn, SIGNAL(released()), this, SLOT(pennyPresenceChanged()));
	connect(mercuryPresenceBtn, SIGNAL(released()), this, SLOT(mercuryPresenceChanged()));
	connect(alexPresenceBtn, SIGNAL(released()), this, SLOT(alexPresenceChanged()));
	connect(excaliburPresenceBtn, SIGNAL(released()), this, SLOT(excaliburPresenceChanged()));
	connect(anan10PresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	connect(anan10EPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	connect(anan100PresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	connect(anan100DPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	connect(anan200DPresenceBtn, SIGNAL(released()), this, SLOT(hardwareChanged()));
	connect(firmwareCheckBtn, SIGNAL(released()), this, SLOT(firmwareCheckChanged()));
}
