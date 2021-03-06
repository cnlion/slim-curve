/* 
This file is part of the SLIM-curve package for exponential curve fitting of spectral lifetime data.

Copyright (c) 2010, 2011, Gray Institute University of Oxford & UW-Madison LOCI.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GCI_Phasor.h"
#include <math.h>
#include <string.h>

#ifndef NULL
#define NULL 0
#endif

#define PHASOR_ERR_NO_ERROR                         0
#define PHASOR_ERR_INVALID_DATA                    -1
#define PHASOR_ERR_INVALID_WINDOW                  -2
#define PHASOR_ERR_INVALID_MODEL                   -3
#define PHASOR_ERR_FUNCTIONALITY_NOT_SUPPORTED     -4


// See Clayton 2004 or Leray 2008
// Classic Phasor or Polar approach to FLIM

// u = integral(data(t) * cos(wt)) dt / integral(data(t)) dt
// v = integral(data(t) * sin(wt)) dt / integral(data(t)) dt
// 
// tau phi = taup = 1/w * (v/u)
// tau mod = taum = 1/w * sqrt(1/(u^2 + v^2) - 1)
// tau average = tau = (taup + taum) / 2

// Z must have been estimated previously so that it can be subtracted from the data here
// A is estimated by making the photon count in the fit the same as the data

// chisq is calculated for comparison with other methods but is not used as there is no optimisation with this method


int    GCI_Phasor(float xincr, float y[], int fit_start, int fit_end,
							  float *Z, float *U, float *V, float *taup, float *taum, float *tau, float *fitted, float *residuals,
							  float *chisq)
{
    // Z must contain a bg estimate
	
	int   i, ret = PHASOR_ERR_NO_ERROR, nBins;
	float *data, u, v, A, w, I, Ifit, bg, chisq_local, res, sigma2;

	data = &(y[fit_start]);	
	nBins = (fit_end - fit_start);
	bg = *Z;

    if (!data)
        return (PHASOR_ERR_INVALID_DATA);
    if (nBins<0)
        return (PHASOR_ERR_INVALID_WINDOW);

	// rep frequency, lets use the period of the measurement, but we can stay in the units of bins
	w = (float)2.0*(float)3.1415926535897932384626433832795028841971/(float)nBins; //2.0*PI/(float)nBins;
	
	// integral over data
	for (i=0, I=0.0; i<nBins; i++) 
		I += (data[i]-bg);

	// Phasor coords
    // Take care that values correspond to the centre of the bin, hence i+0.5
	for (i=0, u=0.0; i<nBins; i++) 
		u += (data[i]-bg) * (float)cos(w*((float)i+0.5));
	u /= I;

	for (i=0, v=0.0; i<nBins; i++) 
		v += (data[i]-bg) * (float)sin(w*((float)i+0.5));
	v /= I;

	// taus, convert now to real time with xincr
	*taup = (xincr/w) * (v/u);
	*taum = (xincr/w) * (float)sqrt(1.0/(u*u + v*v) - 1.0);

	*tau = ((*taup) + (*taum))/(float)2.0;

	*U = u;
	*V = v;

	/* Now calculate the fitted curve and chi-squared if wanted. */
	if (fitted == NULL)
		return 0;

	memset(fitted, 0, fit_end*sizeof(float));
	memset(residuals, 0, fit_end*sizeof(float));
	
	// integral over nominal fit data
	for (Ifit=0.0, i=fit_start; i<fit_end; i++) 
		Ifit += (float)exp(-(i-fit_start)*xincr/(*tau));

	// Estimate A
	A = I / Ifit;

	// Calculate fit
	for (i=fit_start; i<fit_end; i++)
		fitted[i] = bg + A * (float)exp(-(i-fit_start)*xincr/(*tau));

	// OK, so now fitted contains our data for the timeslice of interest.
	// We can calculate a chisq value and plot the graph, along with
	// the residuals.

	if (residuals == NULL && chisq == NULL)
		return 0;

	chisq_local = 0.0;
	for (i=0; i<fit_start; i++) {
		res = y[i]-fitted[i];
		if (residuals != NULL)
			residuals[i] = res;
	}


//	case NOISE_POISSON_FIT:
		/* Summation loop over all data */
		for (i=fit_start ; i<fit_end; i++) {
			res = y[i] - fitted[i];
			if (residuals != NULL)
				residuals[i] = res;
			/* don't let variance drop below 1 */
			sigma2 = (fitted[i] > 1 ? (float)1.0/fitted[i] : (float)1.0);
			chisq_local += res * res * sigma2;
		}

	if (chisq != NULL)
		*chisq = chisq_local;

    return (ret);
}

