#pragma once


inline byte calcCurve(Curve curve, int Value, int Form)
{
	int ret = 0;

	float x = Value * 8.0;
	float f = ((float)Form) / 64.0; //[1;127]->[0.;2.0]

	switch(curve) {
		//[0-1023]x[0-127]
		case 0:
			ret = x * f / 16.0;
			break;

		case 1:
			ret = (127.0 / (exp(2.0 * f) - 1)) * (exp(f * x / 512.0) - 1.0);
			break; //Exp 4*(exp(x/256)-1)

		case 2:
			ret = log(1.0 + (f * x / 128.0)) * (127.0 / log((8 * f) + 1));
			break; //Log 64*log(1+x/128)

		case 3:
			ret = (127.0 / (1.0 + exp(f * (512.0 - x) / 64.0)));
			break; //Sigma

		case 4:
			ret = (64.0 - ((8.0 / f) * log((1024 / (1 + x)) - 1)));
			break; //Flat

		case eXTRA:
			ret = (x + 0x20) * f / 16.0;

	}

	if(ret <= 0) { return 0; }

	if(ret >= 127) { return 127; } //127

	return ret;
}