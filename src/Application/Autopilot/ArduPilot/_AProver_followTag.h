
#ifndef OpenKAI_src_Autopilot_AP__AProver_followTag_H_
#define OpenKAI_src_Autopilot_AP__AProver_followTag_H_

#include "_AP_base.h"
#include "../../../Universe/_Universe.h"
#include "../../../Control/PIDctrl.h"
#include "../../../Control/Drive.h"
#include "../../../Utility/util.h"

namespace kai
{

class _AProver_followTag: public _MissionBase
{
public:
	_AProver_followTag();
	~_AProver_followTag();

	bool init(void* pKiss);
	bool start(void);
	void update(void);
	int check(void);
	void draw(void);

private:
	void updateFollow (void);
    _Object* findTarget(void);
	static void* getUpdateThread(void* This)
	{
		(( _AProver_followTag *) This)->update();
		return NULL;
	}

private:
	_AP_base* m_pAP;
	Drive* m_pD;
    _Universe* m_pU;
    PIDctrl* m_pPIDtagX;
    PIDctrl* m_pPIDtagHdg;
    
    float m_errX;
    float m_errHdg;

	float   m_nSpd;
    float   m_nStr;
	float   m_tagTargetX;    
   	int     m_iTagStop;
    
};

}
#endif
