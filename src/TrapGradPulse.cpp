/** @file TrapGradPulse.cpp
 *  @brief Implementation of JEMRIS TrapGradPulse
 */

/*
 *  JEMRIS Copyright (C) 2007-2010  Tony Stöcker, Kaveh Vahedipour
 *                                  Forschungszentrum Jülich, Germany
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

#include "TrapGradPulse.h"
#include "ConcatSequence.h"

TrapGradPulse::TrapGradPulse        (const TrapGradPulse& hrfp) {
	m_flat_top_area	    = 0.0;
	m_flat_top_time	    = 0.0;
	m_asym_sr	    = 0.0;
	m_has_flat_top_time = false;
	m_has_flat_top_area = false;
	m_has_duration	    = false;
	m_amplitude	    = 0.0;
	m_ramp_up_time	    = 0.0;
	m_time_to_ramp_dn   = 0.0;
	m_ramp_dn_time	    = 0.0;
	m_slope_up	    = 0.0;
	m_slope_dn	    = 0.0;
};

TrapGradPulse::TrapGradPulse        ()                          {};

TrapGradPulse::~TrapGradPulse       ()                          {};

TrapGradPulse* TrapGradPulse::Clone () const  { return (new TrapGradPulse(*this)); }


/***********************************************************/
bool TrapGradPulse::Prepare  (PrepareMode mode) {

	ATTRIBUTE("FlatTopArea"         , m_flat_top_area   );
	ATTRIBUTE("FlatTopTime"         , m_flat_top_time   );
	ATTRIBUTE("Asymetric"           , m_asym_sr         );
	HIDDEN_ATTRIBUTE("Amplitude"    , m_amplitude       );
	HIDDEN_ATTRIBUTE("RampUpTime"   , m_ramp_up_time    );
	HIDDEN_ATTRIBUTE("RampDnTime"   , m_ramp_dn_time    );
	HIDDEN_ATTRIBUTE("EndOfFlatTop" , m_time_to_ramp_dn ); //for convinience: m_time_to_ramp_dn = m_ramp_up_time + m_flat_top_time

	if ( mode != PREP_UPDATE )
	{
		//XML error checking
    		m_has_flat_top_time = HasDOMattribute("FlatTopTime"); // these 3 conditions have to be known
    		m_has_flat_top_area = HasDOMattribute("FlatTopArea"); // also during PREP_UPDATE, therefore
    		m_has_duration    = HasDOMattribute("Duration");    // local boolean are defined (speed)
		if ( m_has_duration && m_has_flat_top_time )
		{
			if ( mode == PREP_VERBOSE)
				cout	<< GetName() << "::Prepare() error: set only one of "
					<< "'Duration' and 'FlatTopTime' for a TrapGradPulse\n";
			return false;
		}
		if ( HasDOMattribute("Area") && m_has_flat_top_area )
		{
			if ( mode == PREP_VERBOSE)
				cout	<< GetName() << "::Prepare() error: set only one of "
					<< "'Area' and 'FlatTopArea' for a TrapGradPulse\n";
			return false;
		}
		if ( m_has_flat_top_time && !m_has_flat_top_area )
		{
			if ( mode == PREP_VERBOSE)
				cout	<< GetName() << "::Prepare() error: 'FlatTopTime' needs "
					<< "also 'FlatTopArea' for a TrapGradPulse\n";
			return false;
		}
	}

	//1st: call the base-class Prepare() to set all local variables
	//2nd: call SetShape() to compute the trapezoid
	bool btag = (GradPulse::Prepare(mode) && SetShape(mode == PREP_VERBOSE) );

        if (!btag && mode == PREP_VERBOSE)
                cout << "\n warning in Prepare(1) of TRAPGRADPULSE " << GetName() << endl;
	return btag;
};


/***********************************************************/
inline bool    TrapGradPulse::SetShape  (bool verbose){

	//predefined area defintion for the flat top
	if ( m_has_flat_top_area ) {
	       double dC = 2.0/fabs(2.0*m_slew_rate);
               SetArea( m_flat_top_area *( 1.0 + m_max_ampl*m_max_ampl*dC / fabs(m_flat_top_area) ) );
	}
	//predefined duration or flat-top time
	if ( m_has_duration || m_has_flat_top_time )
	{
		//Prepare in shortest time first and check if requested time is possible
		double requested    = (m_has_duration?m_duration:m_flat_top_time);
		SetTrapezoid();
		double min_possible = (m_has_duration?m_duration:m_flat_top_time);
		if (requested < min_possible && verbose)
		{
			cout	<< GetName() << "::SetShape() warning: requested "
				<< (m_has_duration?"duration":"FlatTopTime") << " too short for this TrapezGradPulse.\n" ;
			return false;
		}
		//change system limits (m_max_ampl) so that .Prepare in shortest time"
		//yields exactly the requested time
        	double dGmax = m_max_ampl, dC = 0.0;
		if (m_has_duration)
		{
        		dC = 1.0/fabs(2.0*m_slope_up) + 1.0/fabs(2.0*m_slope_dn);
        		m_max_ampl = ( requested - sqrt(requested*requested - 4*fabs(m_area)*dC) )/(2.0*dC);
		}
		else
		{
			m_max_ampl = fabs(m_flat_top_area/requested);
	        	dC = 2.0/fabs(2.0*m_slew_rate);
                	SetArea ( m_flat_top_area *( 1.0 + m_max_ampl*m_max_ampl*dC / fabs(m_flat_top_area) ) );
		}
		SetTrapezoid();     //Calculate the gradient shape
        	m_max_ampl = dGmax; //Reset maximum amplitude
		return true;
	}

	//standard case: Calculate trapezoid in shortest possible time
	SetTrapezoid();
	return true;
};

/***********************************************************/
inline void    TrapGradPulse::SetTrapezoid  (){

	if (m_area ==0.0)
	{
		m_ramp_up_time  = 0.0; m_flat_top_time  = 0.0; m_ramp_dn_time = 0.0;
        	m_slope_up = 0.0;  m_slope_dn = 0.0; m_amplitude   = 0.0;
        	SetDuration (0.0);
		return;
	}

        double dAbsArea = fabs(m_area);
        double dSign = m_area/dAbsArea;
        double Gmax = m_max_ampl;
        double dFlatTopArea ;

        m_slope_up = dSign*m_slew_rate;
        m_slope_dn = -1.0*dSign*m_slew_rate;
        if (m_asym_sr > 0.0) m_slope_up *= m_asym_sr;
        if (m_asym_sr < 0.0) m_slope_dn *= fabs(m_asym_sr);
        double dC = 1.0/fabs(2.0*m_slope_up) + 1.0/fabs(2.0*m_slope_dn);

        if (dAbsArea <= Gmax*Gmax*dC )  //triangle shape (no flat top)
        {
                dFlatTopArea = 0.0;
                m_amplitude   = dSign*sqrt( dAbsArea / dC ) ;
        }
        else                            //trapezoidal shape (with flat top)
        {
                dFlatTopArea = dSign* ( dAbsArea - Gmax*Gmax*dC );
                m_amplitude   = dSign*Gmax;
        }

        m_ramp_up_time    = fabs(m_amplitude/m_slope_up);
        m_ramp_dn_time    = fabs(m_amplitude/m_slope_dn);
	m_flat_top_time   = fabs(dFlatTopArea/m_amplitude);
	m_time_to_ramp_dn = m_ramp_up_time + m_flat_top_time;

        SetDuration (m_ramp_up_time + m_flat_top_time + m_ramp_dn_time);


	return;
};

/***********************************************************/
inline double  TrapGradPulse::GetGradient  (double const time){
        //on the ramp up ?
        if (time < m_ramp_up_time )
	{
		//no ramp-up for SlewRate >= 1000 (for constant gradients)
		if (fabs(m_slope_up) >999.9)
			return m_amplitude;
		else
			return (time * m_slope_up);
	}
        //on the flat top ?
        if (time < m_ramp_up_time+m_flat_top_time ) { return m_amplitude ; }

        //on the ramp down!

	//no ramp-dn for SlewRate >= 1000 (for constant gradients)
	if (fabs(m_slope_dn) >999.9)
		return m_amplitude;
	else
        	return (m_amplitude + (time - m_ramp_up_time - m_flat_top_time) * m_slope_dn );

};

/***********************************************************/
inline void  TrapGradPulse::SetTPOIs () {

	if ( !m_has_flat_top_time )	//set ADCs over total duration (standard)
		Pulse::SetTPOIs();
	else			//set ADCs only on the flat top!
	{
		m_tpoi.Reset();
    		m_tpoi + TPOI::set(TIME_ERR_TOL              , -1.0);
    		m_tpoi + TPOI::set(GetDuration()-TIME_ERR_TOL, -1.0);
		//add ADCs  only on the flat top
    		for (unsigned int i = 0; i < GetNADC(); i++)
    			m_tpoi + TPOI::set(m_ramp_up_time + (i+1)*m_flat_top_time/(GetNADC()+1),
					   (m_phase_lock?World::instance()->PhaseLock:0.0)                 );
	}

	//add TPOIs at nonlinear points of the trapezoid
    	m_tpoi + TPOI::set(m_ramp_up_time	          , -1.0);
    	m_tpoi + TPOI::set(m_ramp_up_time+m_flat_top_time , -1.0);

}

/***********************************************************/
string          TrapGradPulse::GetInfo() {

	stringstream s;
	s << GradPulse::GetInfo();
	if ( m_has_flat_top_time )
		s << " , FlatTop: (Area,time)= (" << m_flat_top_area << "," << m_flat_top_time << ")";

	return s.str();
};
