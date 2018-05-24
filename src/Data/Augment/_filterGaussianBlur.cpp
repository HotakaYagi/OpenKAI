/*
 * _filterGaussianBlur.cpp
 *
 *  Created on: Mar 12, 2018
 *      Author: yankai
 */

#include "_filterGaussianBlur.h"

namespace kai
{

_filterGaussianBlur::_filterGaussianBlur()
{
}

_filterGaussianBlur::~_filterGaussianBlur()
{
}

bool _filterGaussianBlur::init(void* pKiss)
{
	IF_F(!this->_FilterBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;
	pK->m_pInst = this;

	return true;
}

bool _filterGaussianBlur::link(void)
{
	IF_F(!this->_DataBase::link());
	Kiss* pK = (Kiss*) m_pKiss;

	return true;
}

bool _filterGaussianBlur::start(void)
{
	m_bComplete = false;

	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _filterGaussianBlur::update(void)
{
	srand(time(NULL));
	cv::RNG gen(cv::getTickCount());

	int nTot = 0;
	m_progress = 0.0;

	for (int i = 0; i < m_vFileIn.size(); i++)
	{
		string fNameIn = m_vFileIn[i];
		string dirNameIn = getFileDir(fNameIn);
		Mat mIn = cv::imread(fNameIn.c_str());
		IF_CONT(mIn.empty());

		Mat mOut;
		for (int j = 0; j < m_nProduce; j++)
		{
			int rX = m_dRand*NormRand()+1;
			int rY = m_dRand*NormRand()+1;

			if(rX%2==0)rX++;
			if(rY%2==0)rY++;

			cv::GaussianBlur(mIn, mOut, Size(rX,rY), 0, 0);
			cv::imwrite(dirNameIn + uuid() + m_extOut, mOut, m_PNGcompress);
		}

		nTot++;
		double prog = (double) i / (double) m_vFileIn.size();
		if (prog - m_progress > 0.1)
		{
			m_progress = prog;
			LOG_I("  - Progress: " + i2str((int)(m_progress * 100)) + "%");
		}
	}

	LOG_I("  - Complete: Total produced: " + i2str(nTot));
	m_bComplete = true;
}

}

