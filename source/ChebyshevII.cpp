#include "DspFilters/Common.h"
#include "DspFilters/ChebyshevII.h"

namespace Dsp {

namespace ChebyshevII {

namespace detail {

// "Chebyshev Filter Properties"
// http://cnx.org/content/m16906/latest/

void AnalogLowPass::design (int numPoles,
                            double rippleDb,
                            LayoutBase& proto)
{
  proto.reset ();

  const double eps = std::sqrt (1. / (std::exp (rippleDb * 0.1 * doubleLn10) - 1));
  const double v0 = asinh (1 / eps) / numPoles;
  const double sinh_v0 = -sinh (v0);
  const double cosh_v0 = cosh (v0);
  const double fn = doublePi / (2 * numPoles);

  int k = 1;
  for (int i = numPoles / 2; --i >= 0; k+=2)
  {
    const double a = sinh_v0 * cos ((k - numPoles) * fn);
    const double b = cosh_v0 * sin ((k - numPoles) * fn);
    const double d2 = a * a + b * b;
    const double im = 1 / cos (k * fn);
    proto.addPoleZeroConjugatePairs (complex_t (a / d2, b / d2),
                                     complex_t (0, im));
  }

  if (numPoles & 1)
  {
    proto.addPoleZero (1 / sinh_v0, infinity());
    //proto.addPoleZero (complex_t (sinh_v0, 0), infinity());
  }

  proto.setNormal (0, 1);
}

//
// Chebyshev Type I low pass shelf prototype
// From "High-Order Digital Parametric Equalizer Design"
// Sophocles J. Orfanidis
// http://www.ece.rutgers.edu/~orfanidi/ece521/hpeq.pdf
//
void AnalogLowShelf::design (int numPoles,
                             double gainDb,
                             double rippleDb,
                             LayoutBase& proto)
{
  proto.reset ();

  gainDb = -gainDb;

  if (rippleDb >= abs(gainDb))
    rippleDb = abs (gainDb);
  if (gainDb<0)
    rippleDb = -rippleDb;

  const double G  = std::pow (10., gainDb / 20.0 );
  const double Gb = std::pow (10., (gainDb - rippleDb) / 20.0);
  const double G0 = 1;
  const double g0 = pow (G0 , 1. / numPoles);

  double eps;
  if (Gb != G0 )
    eps = sqrt((G*G-Gb*Gb)/(Gb*Gb-G0*G0));
  else
    eps = G-1; // This is surely wrong

  const double b = pow (G/eps+Gb*sqrt(1+1/(eps*eps)), 1./numPoles);
  const double u = log (b / g0);
  const double v = log (pow (1. / eps+sqrt(1+1/(eps*eps)),1./numPoles));
  
  const double sinh_u = sinh (u);
  const double sinh_v = sinh (v);
  const double cosh_u = cosh (u);
  const double cosh_v = cosh (v);
  const double n2 = 2 * numPoles;
  const int pairs = numPoles / 2;
  for (int i = 1; i <= pairs; ++i)
  {
    const double a = doublePi * (2 * i - 1) / n2;
    const double sn = sin (a);
    const double cs = cos (a);
    proto.addPoleZeroConjugatePairs (complex_t (-sn * sinh_u, cs * cosh_u),
                                     complex_t (-sn * sinh_v, cs * cosh_v));
  }

  if (numPoles & 1)
    proto.addPoleZero (-sinh_u, -sinh_v);

  proto.setNormal (doublePi, 1);
}

//------------------------------------------------------------------------------

void LowPassBase::setup (int order,
                         double sampleRate,
                         double cutoffFrequency,
                         double rippleDb)
{
  AnalogLowPass::design (order, rippleDb, m_analogProto);

  LowPassTransform::transform (cutoffFrequency / sampleRate,
                               m_digitalProto,
                               m_analogProto);

  Cascade::setup (m_digitalProto);
}

void HighPassBase::setup (int order,
                          double sampleRate,
                          double cutoffFrequency,
                          double rippleDb)
{
  AnalogLowPass::design (order, rippleDb, m_analogProto);

  HighPassTransform::transform (cutoffFrequency / sampleRate,
                                m_digitalProto,
                                m_analogProto);

  Cascade::setup (m_digitalProto);
}

void BandPassBase::setup (int order,
                          double sampleRate,
                          double centerFrequency,
                          double widthFrequency,
                          double rippleDb)
{
  AnalogLowPass::design (order, rippleDb, m_analogProto);

  BandPassTransform::transform (centerFrequency / sampleRate,
                                widthFrequency / sampleRate,
                                m_digitalProto,
                                m_analogProto);

  Cascade::setup (m_digitalProto);
}

void BandStopBase::setup (int order,
                          double sampleRate,
                          double centerFrequency,
                          double widthFrequency,
                          double rippleDb)
{
  AnalogLowPass::design (order, rippleDb, m_analogProto);

  BandStopTransform::transform (centerFrequency / sampleRate,
                                widthFrequency / sampleRate,
                                m_digitalProto,
                                m_analogProto);

  Cascade::setup (m_digitalProto);
}

void LowShelfBase::setup (int order,
                          double sampleRate,
                          double cutoffFrequency,
                          double gainDb,
                          double rippleDb)
{
  AnalogLowShelf::design (order, gainDb, rippleDb, m_analogProto);

  LowPassTransform::transform (cutoffFrequency / sampleRate,
                               m_digitalProto,
                               m_analogProto);

  Cascade::setup (m_digitalProto);
}

void HighShelfBase::setup (int order,
                           double sampleRate,
                           double cutoffFrequency,
                           double gainDb,
                           double rippleDb)
{
  AnalogLowShelf::design (order, gainDb, rippleDb, m_analogProto);

  HighPassTransform::transform (cutoffFrequency / sampleRate,
                                m_digitalProto,
                                m_analogProto);

  Cascade::setup (m_digitalProto);
}

void BandShelfBase::setup (int order,
                           double sampleRate,
                           double centerFrequency,
                           double widthFrequency,
                           double gainDb,
                           double rippleDb)
{
  AnalogLowShelf::design (order, gainDb, rippleDb, m_analogProto);

  BandPassTransform::transform (centerFrequency / sampleRate,
                                widthFrequency / sampleRate,
                                m_digitalProto,
                                m_analogProto);

  m_digitalProto.setNormal (((centerFrequency/sampleRate) < 0.25) ? doublePi : 0, 1);

  Cascade::setup (m_digitalProto);
}

}

}

}
