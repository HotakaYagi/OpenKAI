#include "../ArduPilot/_AP_relay.h"

namespace kai
{

_AP_relay::_AP_relay()
{
	m_pAP = NULL;
}

_AP_relay::~_AP_relay()
{
}

bool _AP_relay::init(void* pKiss)
{
	IF_F(!this->_StateBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	int i = 0;
	while (1)
	{
		Kiss* pR = pK->child(i++);
		if(pR->empty())break;

		AP_relay r;
		r.init();
		pR->v("iChan",&r.m_iChan);
		pR->v("bRelay",&r.m_bRelay);
		m_vRelay.push_back(r);
	}

	string iName;
	iName = "";
	F_ERROR_F(pK->v("_AP_base", &iName));
	m_pAP = (_AP_base*) (pK->getInst(iName));

	return true;
}

bool _AP_relay::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG(ERROR) << "Return code: "<< retCode;
		m_bThreadON = false;
		return false;
	}

	return true;
}

int _AP_relay::check(void)
{
	NULL__(m_pAP, -1);
	NULL__(m_pAP->m_pMav, -1);

	return this->_StateBase::check();
}

void _AP_relay::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		this->_StateBase::update();
		updateRelay();

		this->autoFPSto();
	}
}

void _AP_relay::updateRelay(void)
{
	IF_(check()<0);
	IF_(!bActive());

	_Mavlink* pMav = m_pAP->m_pMav;

	for(AP_relay s : m_vRelay)
	{
		pMav->clDoSetRelay(s.m_iChan, s.m_bRelay);
	}
}

void _AP_relay::draw(void)
{
	IF_(check()<0);
	this->_StateBase::draw();
	drawActive();

	for(AP_relay s : m_vRelay)
	{
		addMsg("Chan:" + i2str((int)s.m_iChan) + ", bRelay=" + i2str((int)s.m_bRelay), 1);
	}

}

}
