/*
 * _PCscanView.h
 *
 *  Created on: May 28, 2020
 *      Author: yankai
 */

#ifndef OpenKAI_src_Application_PCscan__PCscanView_H_
#define OpenKAI_src_Application_PCscan__PCscanView_H_
#ifdef USE_OPEN3D
#include "../../PointCloud/_PCviewer.h"
#include "PCscanUI.h"

namespace kai
{

	class _PCscanView : public _PCviewer
	{
	public:
		_PCscanView();
		virtual ~_PCscanView();

		virtual bool init(void *pKiss);
		virtual bool start(void);
		virtual int check(void);

	protected:
		static void OnBtnScan( void* pPCV );
		static void OnBtnReset( void* pPCV );
		static void OnBtnSave( void* pPCV );

		virtual void update(void);
		virtual void updateGUI(void);

		static void *getUpdate(void *This)
		{
			((_PCscanView *)This)->update();
			return NULL;
		}

		static void *getUpdateGUI(void *This)
		{
			((_PCscanView *)This)->updateGUI();
			return NULL;
		}

	protected:
		shared_ptr<visualizer::PCscanUI> m_spWin;
	};

}
#endif
#endif
