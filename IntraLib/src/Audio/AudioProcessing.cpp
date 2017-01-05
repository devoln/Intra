#include "Audio/AudioProcessing.h"
#include "Range/ArrayRange.h"
#include "Math/MathEx.h"

namespace Intra {

void DiscreteFourierTransform(ArrayRange<float> outFreqs, ArrayRange<const short> samples)
{
    for(size_t i=0; i<outFreqs.Length(); i++)
    {
        const float wi = float(i)*(2.0f*float(Math::PI)/float(samples.Length()));
        const float sii = Math::Sin(wi), coi = Math::Cos(wi);

        float co=1, si=0, acco=0, acsi=0;
        for(size_t j=0; j<samples.Length(); j+=2)
        {
            const float f = float(samples[j]+samples[j+1]);
            const float oco = co;
            acco += co*f;
			co = co*coi-si*sii;
            acsi += si*f;
			si = si*coi+oco*sii;
        }
        outFreqs[i] = Math::Sqrt(acco*acco+acsi*acsi);
    }
}

}

