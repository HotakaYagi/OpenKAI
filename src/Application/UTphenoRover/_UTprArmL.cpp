#include "_UTprArmL.h"

namespace kai
{

_UTprArmL::_UTprArmL()
{
	m_pAx = NULL;
	m_pAy = NULL;
	m_pAz = NULL;
	m_pU = NULL;

	m_vP.init(0.5,0.5,0.0);
	m_vPtarget.init(0.5,0.5,0.0);
	m_vPrecover.init(25000, 0, 0); //x,rot,y

	m_pXpid = NULL;
	m_pYpid = NULL;

    m_zSpeed = 1000;
	m_zrK = 1.0;
	m_vZgoal.init(15000, 20000);
}

_UTprArmL::~_UTprArmL()
{
}

bool _UTprArmL::init(void *pKiss)
{
	IF_F(!this->_MissionBase::init(pKiss));
	Kiss *pK = (Kiss*) pKiss;

	pK->v("vPtarget", &m_vPtarget);
	pK->v("vPrecover", &m_vPrecover);
	pK->v("zSpeed", &m_zSpeed);
	pK->v("zrK", &m_zrK);
	pK->v("vZgoal", &m_vZgoal);

	IF_F(!m_pMC);
	IF_F(!m_iMission.assign(m_pMC));

	string n;

	n = "";
	F_ERROR_F(pK->v("_ActuatorBaseX", &n ));
	m_pAx = (_ActuatorBase*) (pK->getInst( n ));
	NULL_Fl(m_pAx, n + " not found");

	n = "";
	F_ERROR_F(pK->v("_ActuatorBaseY", &n ));
	m_pAy = (_ActuatorBase*) (pK->getInst( n ));
	NULL_Fl(m_pAy, n + " not found");

    n = "";
	F_ERROR_F(pK->v("_ActuatorBaseZ", &n ));
	m_pAz = (_ActuatorBase*) (pK->getInst( n ));
	NULL_Fl(m_pAz, n + " not found");

    n = "";
	F_ERROR_F(pK->v("_Universe", &n ));
	m_pU = (_Universe*) (pK->getInst( n ));
	NULL_Fl(m_pU, n + " not found");

	n = "";
	F_ERROR_F(pK->v("PIDx", &n ));
	m_pXpid = (PIDctrl*) (pK->getInst( n ));
	NULL_Fl(m_pXpid, n + " not found");

	n = "";
	F_ERROR_F(pK->v("PIDy", &n ));
	m_pYpid = (PIDctrl*) (pK->getInst( n ));
	NULL_Fl(m_pYpid, n + " not found");

	return true;
}

bool _UTprArmL::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		m_bThreadON = false;
		return false;
	}

	return true;
}

int _UTprArmL::check(void)
{
	NULL__(m_pAx, -1);
	NULL__(m_pAy, -1);
	NULL__(m_pAz, -1);
	NULL__(m_pU, -1);
	NULL__(m_pXpid, -1);
	NULL__(m_pYpid, -1);

	return this->_MissionBase::check();
}

void _UTprArmL::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		this->_MissionBase::update();
		updateArm();

		this->autoFPSto();
	}
}

void _UTprArmL::updateArm(void)
{
	IF_(check() < 0);
	IF_(!bActive());

	int iM = m_pMC->getMissionIdx();
	bool bTransit = false;
    
	if(iM == m_iMission.FOLLOW)
	{
		bTransit = follow();
	}
	else if(iM == m_iMission.RECOVER)
	{
		bTransit = recover();
	}
	else
	{
		recover();
	}

	if(bTransit)
		m_pMC->transit();
}

bool _UTprArmL::follow(void)
{
    float z = m_pAz->getP(0);
    if(z >= m_vZgoal.y)
    {
        stop();
        return true;
    }
    else if(z >= m_vZgoal.x)
    {
        m_pAx->setStarget(0, 0);
        m_pAy->setStarget(0, 0);
        m_pAz->setStarget(0, m_zSpeed);
        return false;
    }
    
	_Object* tO = findTarget();
	if(!tO)
	{
		recover();
		return false;
	}

	m_vP = tO->getPos();
   	float x = m_vP.x - m_vPtarget.x;
	float y = m_vP.y - m_vPtarget.y;
	float r = sqrt(x*x + y*y);

	float sX = m_pXpid->update(m_vP.x, m_vPtarget.x, m_tStamp);
	float sY = m_pYpid->update(m_vP.y, m_vPtarget.y, m_tStamp);
	float sZ = m_zSpeed * constrain(1.0 - r*m_zrK, 0.0, 1.0);

	m_pAx->setStarget(0, sX);
	m_pAy->setStarget(0, sY);
	m_pAz->setStarget(0, sZ);

	return false;
}

_Object* _UTprArmL::findTarget(void)
{    
	_Object *pO;
	_Object *tO = NULL;
	float rSqr = FLT_MAX;
	int i = 0;
	while ((pO = m_pU->get(i++)) != NULL)
	{
		vFloat3 p = pO->getPos();
		float x = p.x - m_vPtarget.x;
		float y = p.y - m_vPtarget.y;
		float r = x*x + y*y;
		IF_CONT(r > rSqr);

		tO = pO;
		rSqr = r;
	}

    return tO;
}

bool _UTprArmL::recover(void)
{
    //recover vertical axis first
	if(!m_pAz->bComplete(0))
    {
        m_pAx->setStarget(0, 0);
        m_pAy->setStarget(0, 0);
        m_pAz->setPtarget(0, m_vPrecover.z);
        return false;
    }

    m_pAx->setPtarget(0, m_vPrecover.x);
	m_pAy->setPtarget(0, m_vPrecover.y);
	IF_F(!m_pAx->bComplete(0));
//	IF_F(!m_pAy->bComplete(0));

	return true;
}

void _UTprArmL::stop(void)
{
	m_pAx->setStarget(0, 0);
	m_pAy->setStarget(0, 0);
	m_pAz->setStarget(0, 0);
}

void _UTprArmL::draw(void)
{
	this->_MissionBase::draw();

	addMsg("vP = (" + f2str(m_vP.x) + ", " + f2str(m_vP.y) + ", " + f2str(m_vP.z) + ")");
	addMsg("vPtarget = (" + f2str(m_vPtarget.x) + ", " + f2str(m_vPtarget.y) + ", " + f2str(m_vPtarget.z) + ")");

	IF_(!checkWindow());
	Mat* pM = ((Window*) this->m_pWindow)->getFrame()->m();
	Point pC = Point(m_vP.x * pM->cols, m_vP.y * pM->rows);
	circle(*pM, pC, 5.0, Scalar(255, 255, 0), 2);
}

}
