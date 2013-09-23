/** @file Sequence.cpp
 *  @brief Implementation of JEMRIS Sequence
 */

/*
 *  JEMRIS Copyright (C) 
 *                        2006-2013  Tony Stoecker
 *                        2007-2013  Kaveh Vahedipour
 *                        2009-2013  Daniel Pflugfelder
 *                                  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Sequence.h"
#include "ConcatSequence.h"
#include "AtomicSequence.h"
#include <stdio.h>
#include "BinaryContext.h"

/***********************************************************/
bool    Sequence::Prepare(const PrepareMode mode){

	ATTRIBUTE("Aux1"     , m_aux1 );
	ATTRIBUTE("Aux2"     , m_aux2 );
	ATTRIBUTE("Aux3"     , m_aux3 );

	bool btag = Module::Prepare(mode);

        //hide XML attributes which were set by Module::Prepare()
	if (mode != PREP_UPDATE)
	    HideAttribute("Duration");

	vector<Module*> children = GetChildren();

	for (unsigned int i=0; i<children.size() ; i++) {

		DEBUG_PRINT("  Sequence::Prepare() of " << GetName() << " calls Prepare(" << mode <<
			    ") of " << children[i]->GetName() << endl;)
		btag = (children[i]->Prepare(mode) && btag);

	}

    if (GetParent()==m_parameters && !btag && mode == PREP_VERBOSE) //only the top node of the tree cries out
            cout << "\n!!! warning in Prepare(1) of sequence " << GetName() <<endl << endl;

    return btag;

}

/***********************************************************/
void Sequence::SeqDiag (const string& fname ) {

	//prepare H5 file structure
	BinaryContext bc ("seq.h5", IO::OUT);
	if (bc.Status() != IO::OK) return;

	NDData<double>      di (GetNumOfTPOIs() + 1);
	std::vector<double>  t (GetNumOfTPOIs() + 1);
	int numaxes = 7;

	NDData<double> seqdata(numaxes,GetNumOfTPOIs()+1);
	
	//HDF5 dataset names
	vector<string> seqaxis;
	seqaxis.push_back("T");		//Time
	seqaxis.push_back("RXP");	//receiver phase
	seqaxis.push_back("TXM");	//transmitter magnitude
	seqaxis.push_back("TXP");	//transmitter phase
	seqaxis.push_back("GX");	//X gradient
	seqaxis.push_back("GY");	//Y gradient
	seqaxis.push_back("GZ");	//Z gradient

	//recursive data collect
	double time   =  0.;
	long   offset =  0l;
	seqdata (1,0) = -1.;
	CollectSeqData (seqdata, time, offset);

	seqdata = transpose(seqdata);

	memcpy (&t[0], &seqdata[0], di.Size() * sizeof(double));

	//write to HDF5 file
	for (size_t i=0; i<numaxes; i++) {
		std::string URN (seqaxis[i]);
		memcpy (&di[0], &seqdata[i*di.Size()], di.Size() * sizeof(double));
		bc.Write(di, URN, "/seqdiag");
		if (i == 4)
			bc.Write (cumtrapz(di,t), "KX", "/seqdiag");
		if (i == 5)
			bc.Write (cumtrapz(di,t), "KY", "/seqdiag");
		if (i == 6)
			bc.Write (cumtrapz(di,t), "KZ", "/seqdiag");
	}

}

/***********************************************************/
void  Sequence::CollectSeqData  (NDData<double>& seqdata, double t, long offset) {

	if (GetType() == MOD_CONCAT) {

		vector<Module*> children = GetChildren();
		ConcatSequence* pSeq     = ((ConcatSequence*) this);

		for (RepIter r=pSeq->begin(); r<pSeq->end(); ++r) {

			for (unsigned int j=0; j<children.size() ; ++j) {

				((Sequence*) children[j])->CollectSeqData(seqdata, t, offset);
				if (children[j]->GetType() != MOD_CONCAT) {
					t   += children[j]->GetDuration();
					offset += children[j]->GetNumOfTPOIs();
				}
			}
		}
	}

	if (GetType() == MOD_ATOM) {
	  
		bool rem  = ((AtomicSequence*) this)->HasNonLinGrad();
		((AtomicSequence*) this)->SetNonLinGrad(false);

		for (int i=0; i < GetNumOfTPOIs(); ++i) {
			seqdata(0,offset+i+1) = m_tpoi.GetTime(i) + t;
			seqdata(1,offset+i+1) = m_tpoi.GetPhase(i);
			GetValue(&seqdata(2,offset+i+1), m_tpoi.GetTime(i));
		}

		((AtomicSequence*) this)->SetNonLinGrad(rem);

	}

}

/***********************************************************/
long  Sequence::GetNumOfADCs () {

	if (GetType() == MOD_CONCAT) {

		long lADC = 0;
		vector<Module*> children = GetChildren();
		ConcatSequence* pSeq     = ((ConcatSequence*) this);

		for (RepIter r=pSeq->begin(); r<pSeq->end(); ++r) {

			for (unsigned int j=0; j<children.size() ; ++j)
					lADC += ((Sequence*) children[j])->GetNumOfADCs();

		}

		return lADC;

	}

	if (GetType() == MOD_ATOM) {

		int iADC = GetNumOfTPOIs();

		for (int i=0; i<GetNumOfTPOIs(); ++i)
			if (m_tpoi.GetPhase(i) < 0.0)  iADC--;

		return iADC;

	}

	return -1;

}
