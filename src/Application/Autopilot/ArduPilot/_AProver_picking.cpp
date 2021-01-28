#include "_AProver_picking.h"

#ifdef USE_OPENCV

namespace kai
{

_AProver_picking::_AProver_picking()
{
	m_pAP = NULL;
	m_pDrive = NULL;
	m_pArm = NULL;
	m_rcMode.init();
	m_pArmMC = NULL;
}

_AProver_picking::~_AProver_picking()
{
}

bool _AProver_picking::init(void* pKiss)
{
	IF_F(!this->_MissionBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	pK->v("iRCmodeChan", &m_rcMode.m_iChan);
	pK->a("vRCmodeDiv", &m_rcMode.m_vDiv);
	m_rcMode.setup();

	Kiss *pRC = pK->child("RC");
	int i = 0;
	while (1)
	{
		Kiss *pC = pRC->child(i++);
		if (pC->empty())
			break;

		RC_CHANNEL rc;
		rc.init();
		pC->v("iChan", &rc.m_iChan);
		pC->v("pwmL", &rc.m_pwmL);
		pC->v("pwmM", &rc.m_pwmM);
		pC->v("pwmH", &rc.m_pwmH);
		pC->a("vDiv", &rc.m_vDiv);
		rc.setup();
		m_vRC.push_back(rc);
	}

	IF_F(!m_pMC);
	m_iMission.STANDBY = m_pMC->getMissionIdxByName ("STANDBY");
	m_iMission.MANUAL = m_pMC->getMissionIdxByName ("MANUAL");
	m_iMission.AUTOPICK = m_pMC->getMissionIdxByName ("AUTOPICK");
	m_iMission.AUTO = m_pMC->getMissionIdxByName ("AUTO");
	IF_F(!m_iMission.bValid());

	string iName;
	iName = "";
	pK->v("_AP_base", &iName);
	m_pAP = (_AP_base*)pK->getInst(iName);
	IF_Fl(!m_pAP, iName + ": not found");

	iName = "";
	pK->v("_AProver_drive", &iName);
	m_pDrive = (_AProver_drive*)pK->getInst(iName);
//	IF_Fl(!m_pDrive, iName + ": not found");

	iName = "";
	pK->v("_PickingArm", &iName);
	m_pArm = (_PickingArm*)pK->getInst(iName);
	NULL_Fl(m_pArm, iName + ": not found");

	iName = "";
	pK->v("_MissionControlArm", &iName);
	m_pArmMC = (_MissionControl*)pK->getInst(iName);
	NULL_Fl(m_pArmMC, iName + ": not found");

	return true;
}

bool _AProver_picking::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG(ERROR)<< "Return code: " << retCode;
		m_bThreadON = false;
		return false;
	}

	return true;
}

int _AProver_picking::check(void)
{
	NULL__(m_pAP, -1);
	NULL__(m_pAP->m_pMav, -1);
//	NULL__(m_pDrive, -1);
	NULL__(m_pArm, -1);
	NULL__(m_pArmMC, -1);

	return this->_MissionBase::check();
}

void _AProver_picking::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		this->_MissionBase::update();

		if(!updateDrive())
		{
			m_pMC->transit(m_iMission.STANDBY);
//			m_pDrive->setSpeed(0.0);
//			m_pDrive->setYaw(0.0);
		}

		updatePicking();

		this->autoFPSto();
	}
}

bool _AProver_picking::updateDrive(void)
{
	IF_F(check() < 0);

	bool bArmed = m_pAP->bApArmed();
	uint32_t apMode = m_pAP->getApMode();
	uint16_t pwmMode = m_pAP->m_pMav->m_rcChannels.getRC(m_rcMode.m_iChan);
	string mission = m_pMC->getMissionName();

//	IF_F(!bArmed);
//	IF_F(apMode == AP_ROVER_HOLD);
	IF_F(pwmMode == UINT16_MAX);

	m_rcMode.pwm(pwmMode);
	int iMode = m_rcMode.i();

	switch(iMode)
	{
	case 0:
		m_pMC->transit(m_iMission.MANUAL);
		break;
	case 1:
		m_pMC->transit(m_iMission.AUTOPICK);
		break;
	case 2:
		m_pMC->transit(m_iMission.AUTO);
		break;
	}

	//m_pDrive->setSpeed(nSpeed);

	return true;
}

bool _AProver_picking::updatePicking(void)
{
	IF_F(check() < 0);

	int i;

	for(i=0; i<m_vRC.size(); i++)
	{
		RC_CHANNEL* pRC = &m_vRC[i];
		uint16_t r = m_pAP->m_pMav->m_rcChannels.getRC(pRC->m_iChan);
		if(r == UINT16_MAX)r = pRC->m_pwmM;
		pRC->pwm(r);
	}

	int iM = m_pMC->getMissionIdx();
	string armMission = m_pArmMC->getMissionName();

	if(iM == m_iMission.STANDBY)
	{

	}
	else if(iM == m_iMission.MANUAL)
	{
		vFloat3 vM;
		vM.x = m_vRC[0].v();
		vM.y = m_vRC[1].v();
		vM.z = m_vRC[2].v();

		m_pArmMC->transit("EXTERNAL");
		m_pArm->move(vM);
		m_pArm->grip((m_vRC[4].i())?false:true);
		m_pArm->wakeUp();

//		vM.init(-1.0);
//		vM.x = m_vRC[3].v();
//		m_pArm->rotate(vM);
	}
	else	//AUTOPICK
	{
		if(armMission == "EXTERNAL")
			m_pArmMC->transit("RECOVER");

	}

	return true;
}

void _AProver_picking::draw(void)
{
	this->_MissionBase::draw();
	IF_(check()<0);

	addMsg("RC mode: " + i2str(m_rcMode.i()));

	for(RC_CHANNEL rc : m_vRC)
		addMsg("Chan" + i2str(rc.m_iChan) + ": " + i2str(rc.m_pwm) + " | " + f2str(rc.v()) + " | " + i2str(rc.i()));

}

}
#endif
