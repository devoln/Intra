#include "Audio/Synth/TableAttenuation.h"
#include "Range/ArrayRange.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Construction/Take.h"

namespace Intra { namespace Audio { namespace Synth {

struct TableAttenuatorParams
{
	byte Len;
	norm8 Table[23];
};

static void TableAttenuationPassFunction(const TableAttenuatorParams& table,
	float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate)
{
	INTRA_ASSERT(table.Len>=2);
	const size_t samplesPerValue = inOutSamples.Length()/size_t(table.Len-1);

	for(uint i=0; i<table.Len-1u; i++)
	{
		double v = double(table.Table[i]);
		double dv = (double(table.Table[i+1])-double(v))/double(samplesPerValue);
		for(size_t s=0; s<samplesPerValue; s++)
		{
			inOutSamples.First() *= float(v);
			v += dv;
			inOutSamples.PopFirst();
		}
	}
	while(!inOutSamples.Empty())
	{
		inOutSamples.First() *= float(table.Table[table.Len-1]);
		inOutSamples.PopFirst();
	}

	(void)sampleRate; (void)noteDuration;
}

AttenuationPass CreateTableAttenuationPass(ArrayRange<const norm8> table)
{
	TableAttenuatorParams params;
	params.Len = byte(table.Length());
	Algo::CopyTo(table, Range::Take(params.Table, params.Len));
	return AttenuationPass(TableAttenuationPassFunction, params);
}

}}}
