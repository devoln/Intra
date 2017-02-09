#include "Math/Shapes.h"

namespace Math
{

#if INTRA_DISABLED
template<> bool AABB<float>::CheckRayIntersection(const Ray<float>& ray, float* near, float* far) const
{
	using namespace Simd;
	using Simd::Set;

	const float4 plus_inf = Set(Infinity);
	const float4 minus_inf = Set(-Infinity);

	// use whatever's apropriate to load.
	const float4 box_min = Set(min.x, min.y, min.z, 0),
		box_max = Set(max.x, max.y, max.z, 0),
		pos = Set(ray.origin.x, ray.origin.y, ray.origin.z, 0),
		inv_dir = Set(1/ray.dir.x, 1/ray.dir.y, 1/ray.dir.z, 0);

	// use a div if inverted directions aren't available
	const float4 l1 = Mul(Sub(box_min, pos), inv_dir);
	const float4 l2 = Mul(Sub(box_max, pos), inv_dir);

	// the order we use for those Min/Max is vital to filter out
	// NaNs that happens when an inv_dir is +/- inf and
	// (box_min - pos) is 0. inf * 0 = NaN
	const float4 filtered_l1a = Min(l1, plus_inf);
	const float4 filtered_l2a = Min(l2, plus_inf);

	const float4 filtered_l1b = Max(l1, minus_inf);
	const float4 filtered_l2b = Max(l2, minus_inf);

	// now that we're back on our feet, test those slabs.
	float4 lmax = Max(filtered_l1a, filtered_l2a);
	float4 lmin = Min(filtered_l1b, filtered_l2b);

	// unfold back. try to hide the latency of the shufps & co.
	const float4 lmax0 = Rotate(lmax);
	const float4 lmin0 = Rotate(lmin);
	lmax = MinSingle(lmax, lmax0);
	lmin = MaxSingle(lmin, lmin0);

	const float4 lmax1 = Z1W1Z2W2(lmax, lmax);
	const float4 lmin1 = Z1W1Z2W2(lmin, lmin);
	lmax = MinSingle(lmax, lmax1);
	lmin = MaxSingle(lmin, lmin1);

	const bool ret = GreaterEqualX(lmax, Set(0.0f)) && GreaterEqualX(lmax, lmin);

	if(near) *near = GetX(lmin);
	if(far) *far = GetX(lmax);

	return ret;
}
#endif

}
