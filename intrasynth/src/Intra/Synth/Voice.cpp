#include "Voice.h"

#include <Cpp/Warnings.h>
#include <Math/Math.h>
#include <Range/Mutation/Transform.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

size_t Voice::RenderChannel(size_t channel, Span<float> dst)
{
    VoiceChannelState& st = Channels[channel];
    const VoiceSharedData& sh = Shared;
    if(dst.Empty() || sh.FragmentLength == 0) return 0;
    const bool preAttenuated = sh.PreAttenuated;

    size_t processed = 0;
    while(processed < dst.Length())
    {
        if(st.Env.CurrentSegment.SamplesLeft == 0) break;
        const size_t chunk = Min(size_t(st.Env.CurrentSegment.SamplesLeft), dst.Length() - processed);
        const Envelope::Segment seg = st.Env.CurrentSegment;

        float exp, expStep, lin, linStep, wrapExp, wrapStep;
        if(preAttenuated)
        {
            // Затухание ноты вшито в данные фрагмента: множитель шагается на каждой
            // обёртке периода (см. WaveFormSampler::preattenuateExponential).
            exp = seg.Exponential ? seg.Volume : 1.0f;
            expStep = seg.Exponential ? seg.DU : 1.0f;
            wrapExp = st.ExpFactor;
            wrapStep = st.ExpStep;
            lin = seg.Exponential ? 0.0f : seg.Volume;
            linStep = seg.Exponential ? 0.0f : seg.DU;
        }
        else
        {
            // Честное экспоненциальное затухание накладывается на каждый семпл
            // вместе с огибающей (как старый renderDirect: seg.Exp *= mExpAtten).
            exp = st.ExpFactor * (seg.Exponential ? seg.Volume : 1.0f);
            expStep = st.ExpStep * (seg.Exponential ? seg.DU : 1.0f);
            wrapExp = 1.0f;
            wrapStep = 1.0f;
            lin = seg.Exponential ? 0.0f : seg.Volume;
            linStep = seg.Exponential ? 0.0f : seg.DU;
        }

        SynthKernels::MultiplyAddLinearInterpolated(
            dst.Drop(processed).Take(chunk), Span<const float>(sh.Fragment, sh.FragmentLength),
            st.Offset, st.Rate, exp, expStep, wrapExp, wrapStep, lin, linStep);

        if(preAttenuated) st.ExpFactor = wrapExp;
        else st.ExpFactor *= PowInt(st.ExpStep, int(chunk));

        st.Env.CurrentSegment.Advance(chunk);
        if(st.Env.CurrentSegment.SamplesLeft == 0) st.Env.StartNextSegment();
        processed += chunk;
    }
    return processed;
}

size_t Voice::RenderAll(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
    const size_t processed = RenderChannel(0, dstLeft);
    if(!dstRight.Empty() && Channels[1].ChannelMultiplier != 0) RenderChannel(1, dstRight);
    if(!dstReverb.Empty() && Channels[2].ChannelMultiplier != 0) RenderChannel(2, dstReverb);

    Multiply(dstLeft.Take(processed), Channels[0].ChannelMultiplier);
    if(!dstRight.Empty() && Channels[1].ChannelMultiplier != 0)
        Multiply(dstRight, Channels[1].ChannelMultiplier);
    if(!dstReverb.Empty() && Channels[2].ChannelMultiplier != 0)
        Multiply(dstReverb, Channels[2].ChannelMultiplier);
    return processed;
}

INTRA_WARNING_POP
