/*
 * _RealSense.h
 *
 *  Created on: Apr 6, 2018
 *      Author: yankai
 */

#ifndef OpenKAI_src_Vision_RealSense_H_
#define OpenKAI_src_Vision_RealSense_H_

#include "../Base/common.h"
#include "_DepthVisionBase.h"
#include "../Utility/util.h"

#ifdef USE_REALSENSE
#include <librealsense2/rs.hpp>

namespace kai
{

class _RealSense: public _DepthVisionBase
{
public:
	_RealSense();
	virtual ~_RealSense();

	bool init(void* pKiss);
	bool start(void);
	void reset(void);

private:
	bool open(void);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_RealSense *) This)->update();
		return NULL;
	}

public:
	rs2::pipeline m_rsPipe;

};

}

#endif
#endif
