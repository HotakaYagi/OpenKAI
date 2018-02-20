#ifndef OpenKAI_src_Autopilot_Controller_HM_HM_base_H_
#define OpenKAI_src_Autopilot_Controller_HM_HM_base_H_

#include "../../../Base/common.h"
#include "../../../IO/_TCP.h"
#include "../../../Protocol/_Canbus.h"
#include "../../ActionBase.h"

enum DIRECTION
{
	dir_left,dir_forward,dir_right
};

namespace kai
{

class HM_base: public ActionBase
{
public:
	HM_base();
	~HM_base();

	bool init(void* pKiss);
	bool link(void);
	void update(void);
	void updateCAN(void);
	bool draw(void);

	void cmd(void);
	void updateGPS(void);

public:
	_Canbus* m_pCAN;
	_IOBase* m_pCMD;
	string m_strCMD;

	int m_maxSpeed;
	int m_maxYawRate;
	int m_defaultSpeed;
	int m_speed;
	int m_yawRate;
	bool m_bSpeaker;
	bool m_bMute;
	unsigned long m_canAddrStation;

	uint8_t m_ctrlB0;
	uint8_t m_ctrlB1;

	uint8_t m_pinLEDl;
	uint8_t m_pinLEDm;
	uint8_t m_pinLEDr;

	DIRECTION	m_dir;

};

}

#endif

