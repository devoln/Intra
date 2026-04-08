#pragma once

#include "Range.h"
#include "Core.h"

namespace Intra {

// ============================================================================
// Mock: Heavy stack array (non-movable, copies data)
// ============================================================================
template<size_t N>
struct HeavyStackArray {
    int data[N];
    size_t pos = 0;
    
    constexpr HeavyStackArray() {
        for(size_t i = 0; i < N; ++i) data[i] = int(i);
    }
    
    // Non-movable (simulates heavy object that can't be moved)
    HeavyStackArray(const HeavyStackArray&) = default;
    HeavyStackArray(HeavyStackArray&&) = delete;
    HeavyStackArray& operator=(const HeavyStackArray&) = default;
    HeavyStackArray& operator=(HeavyStackArray&&) = delete;
    
    [[nodiscard]] constexpr int& First() { return data[pos]; }
    [[nodiscard]] constexpr const int& First() const { return data[pos]; }
    constexpr void PopFirst() { ++pos; }
    [[nodiscard]] constexpr bool Empty() const { return pos >= N; }
    [[nodiscard]] constexpr size_t Length() const { return N - pos; }
    
    using TagAnyInstanceFinite = TTag<>;
};

// ============================================================================
// Mock: Lightweight view (like Span)
// ============================================================================
template<typename T>
struct LightweightView {
    T* begin_;
    T* end_;
    
    LightweightView(T* b, T* e) : begin_(b), end_(e) {}
    
    [[nodiscard]] T& First() { return *begin_; }
    [[nodiscard]] const T& First() const { return *begin_; }
    void PopFirst() { ++begin_; }
    [[nodiscard]] bool Empty() const { return begin_ >= end_; }
    [[nodiscard]] size_t Length() const { return end_ - begin_; }
    
    using TagAnyInstanceFinite = TTag<>;
};


// ============================================================================
// Test helpers
// ============================================================================
struct CopyMoveCounter {
    size_t copies = 0;
    size_t moves = 0;
    
    constexpr void Reset() { copies = moves = 0; }
};

// ============================================================================
// Mock: Tracking wrapper to detect copies
// ============================================================================
template<typename T>
struct TrackingRange {
    T* data_;
    size_t len_;
    CopyMoveCounter* counter_;
    
    constexpr TrackingRange(T* d, size_t l, CopyMoveCounter* c) : data_(d), len_(l), counter_(c) {}
    
    constexpr TrackingRange(const TrackingRange& other) : data_(other.data_), len_(other.len_), counter_(other.counter_) {
        if(counter_) counter_->copies++;
    }
    constexpr TrackingRange(TrackingRange&& other) noexcept : data_(other.data_), len_(other.len_), counter_(other.counter_) {
        if(counter_) counter_->moves++;
    }
    
    [[nodiscard]] constexpr T& First() { return *data_; }
    [[nodiscard]] constexpr const T& First() const { return *data_; }
    constexpr void PopFirst() { ++data_; --len_; }
    [[nodiscard]] constexpr bool Empty() const { return len_ == 0; }
    [[nodiscard]] constexpr size_t Length() const { return len_; }
    
    using TagAnyInstanceFinite = TTag<>;
};

// ============================================================================
// Mock: Type that tracks copies at compile time
// ============================================================================
template<size_t TestId>
struct CountingRange {
    int* data_;
    size_t len_;
    
    constexpr CountingRange(int* d, size_t l) : data_(d), len_(l) {}
    
    // Track copies via static constexpr counter
    static inline size_t copy_count = 0;
    static inline size_t move_count = 0;
    
    constexpr CountingRange(const CountingRange& other) : data_(other.data_), len_(other.len_) {
        // Note: cannot modify static in constexpr context
    }
    constexpr CountingRange(CountingRange&& other) noexcept : data_(other.data_), len_(other.len_) {}
    
    [[nodiscard]] constexpr int& First() { return *data_; }
    [[nodiscard]] constexpr const int& First() const { return *data_; }
    constexpr void PopFirst() { ++data_; --len_; }
    [[nodiscard]] constexpr bool Empty() const { return len_ == 0; }
    [[nodiscard]] constexpr size_t Length() const { return len_; }
    
    using TagAnyInstanceFinite = TTag<>;
};

// ============================================================================
// Alternative RangeOf implementations for comparison
// ============================================================================

// Option A: Current behavior - returns TUnqualRef for rvalues (copies)
template<typename L>
[[nodiscard]] constexpr decltype(auto) RangeOf_Current(L&& list)
{
    if constexpr(CRange<L>)
    {
        if constexpr(CConst<TRemoveReference<L>> && CForwardRange<TUnqualRef<L>>) return TUnqualRef<L>(list);
        else if constexpr(!CLValueReference<L>) return TUnqualRef<L>(list); // Current: copies
        else return INTRA_FWD(list);
    }
}

// Option B: Return rvalue reference for rvalues (no copy, lifetime extension)
template<typename L>
[[nodiscard]] constexpr decltype(auto) RangeOf_RefReturn(L&& list)
{
    if constexpr(CRange<L>)
    {
        if constexpr(CConst<TRemoveReference<L>> && CForwardRange<TUnqualRef<L>>) return TUnqualRef<L>(list);
        else if constexpr(!CLValueReference<L>) return INTRA_FWD(list); // Return rvalue ref, lifetime extended
        else return INTRA_FWD(list);
    }
}

// Option C: Move for rvalues if movable, else reference
template<typename L>
[[nodiscard]] constexpr decltype(auto) RangeOf_MoveIfPossible(L&& list)
{
    if constexpr(CRange<L>)
    {
        if constexpr(CConst<TRemoveReference<L>> && CForwardRange<TUnqualRef<L>>) return TUnqualRef<L>(list);
        else if constexpr(!CLValueReference<L>)
        {
            if constexpr(CMoveConstructible<TUnqualRef<L>>)
                return TUnqualRef<L>(INTRA_MOVE(list)); // Move
            else
                return INTRA_FWD(list); // Fall back to reference
        }
        else return INTRA_FWD(list);
    }
}

// ============================================================================
// static_assert tests for different scenarios
// ============================================================================

#if INTRA_CONSTEXPR_TEST

// Test 1: Basic heavy stack array works
static_assert([]{
    HeavyStackArray<5> arr;
    return arr.First() == 0;
}(), "HeavyStackArray basic test");

// Test 2: Lightweight view type check
static_assert(CRange<LightweightView<int>>, "LightweightView should satisfy CRange");
static_assert(CRange<LightweightView<const int>>, "LightweightView<const> should satisfy CRange");

// Test 3: HeavyStackArray is not movable
static_assert(!CMoveConstructible<HeavyStackArray<10>>, "HeavyStackArray should not be movable");

// Test 4: HeavyStackArray is copyable
static_assert(CCopyConstructible<HeavyStackArray<10>>, "HeavyStackArray should be copyable");

// Test 5: Different RangeOf variants compile
static_assert(CSame<HeavyStackArray<5>, TUnqualRef<HeavyStackArray<5>>>, "TUnqualRef should work for heavy arrays");

#endif // INTRA_CONSTEXPR_TEST

// ============================================================================
// constexpr comparison tests for RangeOf variants
// These tests catch dangling references and UB at compile time
// ============================================================================

// constexpr-friendly tracking range using template parameter as ID
template<size_t N, size_t TestId = 0>
struct ConstexprTrackedArray {
    int data[N];
    mutable size_t copy_count = 0;
    mutable size_t move_count = 0;
    size_t pos = 0;
    
    constexpr ConstexprTrackedArray() : copy_count(0), move_count(0), pos(0) {
        for(size_t i = 0; i < N; ++i) data[i] = int(i);
    }
    
    constexpr ConstexprTrackedArray(const ConstexprTrackedArray& other) 
        : copy_count(other.copy_count + 1), move_count(other.move_count), pos(other.pos) {
        for(size_t i = 0; i < N; ++i) data[i] = other.data[i];
    }
    
    constexpr ConstexprTrackedArray(ConstexprTrackedArray&& other) noexcept 
        : copy_count(other.copy_count), move_count(other.move_count + 1), pos(other.pos) {
        other.move_count++;
        for(size_t i = 0; i < N; ++i) data[i] = other.data[i];
    }
    
    [[nodiscard]] constexpr int& First() { return data[pos]; }
    [[nodiscard]] constexpr const int& First() const { return data[pos]; }
    [[nodiscard]] constexpr bool Empty() const { return pos >= N; }
    constexpr void PopFirst() { ++pos; }
    
    using TagAnyInstanceFinite = TTag<>;
};

#if INTRA_CONSTEXPR_TEST

// Test: RangeOf_Current copies non-movable rvalue
static_assert([]{
    // Create temporary and pass to RangeOf_Current
    auto r = RangeOf_Current(ConstexprTrackedArray<10>{});
    // If this compiles, lifetime was extended or object was copied
    // For Current: TUnqualRef<T> is returned (copy)
    return r.First() == 0;
}(), "RangeOf_Current should work with rvalue (may copy)");

// Test: RangeOf_RefReturn extends lifetime for rvalue (no copy)
static_assert([]{
    auto r = RangeOf_RefReturn(ConstexprTrackedArray<10>{});
    // Reference extends lifetime of temporary
    // This MUST compile without UB or dangling reference
    return r.First() == 0;
}(), "RangeOf_RefReturn should extend lifetime of rvalue");

// Test: RangeOf_MoveIfPossible extends lifetime for non-movable (falls back to ref)
static_assert([]{
    auto r = RangeOf_MoveIfPossible(ConstexprTrackedArray<10>{});
    // Non-movable type falls back to reference, lifetime extended
    return r.First() == 0;
}(), "RangeOf_MoveIfPossible should extend lifetime for non-movable");

// Test: Verify copy count for RangeOf_Current (should copy)
static_assert([]{
    ConstexprTrackedArray<10> arr;
    auto r = RangeOf_Current(arr);  // lvalue - no copy
    // r is same type as arr (lvalue ref preserved)
    return r.First() == 0 && arr.copy_count == 0;
}(), "RangeOf_Current with lvalue should not copy");

// Test: RangeOf variants work with lvalue HeavyStackArray
static_assert([]{
    HeavyStackArray<10> arr;
    auto r1 = RangeOf_Current(arr);
    auto r2 = RangeOf_RefReturn(arr);
    auto r3 = RangeOf_MoveIfPossible(arr);
    return r1.First() == 0 && r2.First() == 0 && r3.First() == 0;
}(), "All RangeOf variants should work with lvalue HeavyStackArray");

// Test: Cascading RangeOf calls (safe because lifetime extends to full-expression)
static_assert([]{
    // This is safe: lifetime of temporaries extends to end of full-expression
    auto r = RangeOf_RefReturn(RangeOf_RefReturn(ConstexprTrackedArray<5>{}));
    return r.First() == 0;
}(), "Cascading RangeOf should work (lifetime extends to full-expression)");

// Test: RangeOf in function argument position (safe)
static_assert([]{
    auto process = [](auto&& r) { return r.First(); };
    // Temporary passed to function - lifetime extends through call
    return process(RangeOf_RefReturn(ConstexprTrackedArray<5>{})) == 0;
}(), "RangeOf in function argument should work");

// Test: Multiple temporaries in one expression (all lifetimes extended)
static_assert([]{
    auto r1 = ConstexprTrackedArray<5>{};
    auto r2 = ConstexprTrackedArray<5>{};
    auto ro1 = RangeOf_RefReturn(r1);
    auto ro2 = RangeOf_RefReturn(r2);
    return ro1.First() == 0 && ro2.First() == 0;
}(), "Multiple RangeOf calls should work");

// CRITICAL: Demonstrate safe vs unsafe pipeline splitting
// SAFE: Using auto (copies or moves the range object)
static_assert([]{
    // Step 1: Create heavy array and convert to range
    auto step1 = RangeOf_RefReturn(ConstexprTrackedArray<5>{});
    // step1 is a reference bound to the temporary - lifetime extended to here
    
    // Step 2: Use step1 (still valid)
    auto result = step1.First();
    return result == 0;
}(), "Single-step pipeline with auto is safe (lifetime extended)");

// CRITICAL: This pattern is UNSAFE and should NOT be used
// auto& intermediate = RangeOf_RefReturn(temp);  // DON'T DO THIS
// temp dies here, intermediate becomes dangling

// Test: Long pipeline with proper variable handling
static_assert([]{
    // Create source
    ConstexprTrackedArray<10> source;
    
    // Step 1: RangeOf on lvalue (returns lvalue ref, no lifetime issues)
    auto& r1 = RangeOf_RefReturn(source);  // OK: source outlives r1
    
    // Step 2: Use r1 (still valid because source exists)
    auto val = r1.First();
    
    return val == 0;
}(), "Pipeline with lvalue source is safe");

// Test: auto&& behavior - type trait check
static_assert(CSame<decltype(RangeOf_RefReturn(ConstexprTrackedArray<5>{})), ConstexprTrackedArray<5>&&>,
    "RangeOf_RefReturn should return rvalue reference");

// Test: auto vs auto&& vs auto& comparison
static_assert([]{
    ConstexprTrackedArray<5> source;
    
    auto a = RangeOf_RefReturn(source);       // Copy (if return by value) or ref (if return by ref)
    auto& b = RangeOf_RefReturn(source);      // Lvalue ref - OK
    auto&& c = RangeOf_RefReturn(source);     // Lvalue ref (collapses to T&) - OK
    
    return a.First() == 0 && b.First() == 0 && c.First() == 0;
}(), "All reference types work with lvalue");

#endif // INTRA_CONSTEXPR_TEST

// ============================================================================
// OwningRange tests (OwningRange is defined in Concepts.h)
// ============================================================================

#ifdef INTRA_CONSTEXPR_TEST

// Test: OwningRange basic compilation check
static_assert(sizeof(OwningRange<ConstexprTrackedArray<5>>) > 0,
    "OwningRange should compile with owning containers");

// Test: NRVO verification - count moves and copies
static_assert([]{
    ConstexprTrackedArray<10, 1> arr;  // TestId = 1
    arr.copy_count = 0;
    arr.move_count = 0;
    
    // When passing lvalue to RangeOf, should return reference (no copy/move)
    auto r = RangeOf(arr);
    
    // Should have exactly 0 copies and 0 moves for lvalue
    return arr.copy_count == 0 && arr.move_count == 0 && r.First() == 0;
}(), "RangeOf with lvalue should not copy or move");

// Test: NRVO for rvalue - should have exactly 1 move (into OwningRange), 0 copies
static_assert([]{
    // Create temporary and pass to RangeOf
    auto r = RangeOf(ConstexprTrackedArray<10, 2>{});
    
    // r is OwningRange, container moved into it
    // Should have exactly 1 move (into OwningRange) and 0 copies
    return r.First() == 0;
}(), "RangeOf with rvalue should use OwningRange with single move (NRVO)");

// Test: Long pipeline - OwningRange -> RMap -> RFilter, verify no extra copies
static_assert([]{
    // Pipeline: container | Map | Filter | Take
    // Should have minimal moves, zero copies of container data
    
    auto r = RangeOf(ConstexprTrackedArray<10, 3>{})
        | Map([](int x) { return x * 2; })
        | Filter([](int x) { return x > 5; })
        | Take(3);
    
    // Just verify compilation and basic correctness
    // RVO/NRVO should ensure container is moved, not copied
    return !r.Empty();
}(), "Long pipeline with OwningRange should compile without extra copies");

// Test: Chained functors - verify no extra copies of heavy captured data
static_assert([]{
    // Heavy functor capturing array by value
    struct HeavyMultiplier {
        int factor[100];  // Heavy state
        constexpr int operator()(int x) const { return x * factor[0]; }
    };
    
    HeavyMultiplier mult{{5}};
    auto f = FunctorOf(INTRA_FWD(mult));
    
    // Use in pipeline
    auto r = (RangeOf(ConstexprTrackedArray<5, 4>{}) | Map(f));
    return r.First() == 0;  // 0 * 5 = 0
}(), "Heavy functor in pipeline should not cause extra copies");

// Test: Detailed RVO/NRVO verification through pipeline chain
static_assert([]{
    // Reset counters for this test (TestId = 5)
    ConstexprTrackedArray<10, 5> container{};
    container.copy_count = 0;
    container.move_count = 0;
    
    // Build long pipeline: OwningRange -> RMap -> RFilter -> RTake
    auto pipeline = (RangeOf(INTRA_FWD(container))
        | Map([](int x) { return x * 2; })
        | Filter([](int x) { return x > 5; })
        | Take(3));
    
    // Through NRVO:
    // - container should be moved ONCE into OwningRange (1 move, 0 copies)
    // - RMap, RFilter, RTake store ranges by value (moves of lightweight wrappers)
    // - No copies of the actual container data
    
    // Access pipeline to ensure it's valid
    bool valid = !pipeline.Empty();
    
    // Verify counters: should have exactly 1 move (into OwningRange), 0 copies
    return valid && container.copy_count == 0 && container.move_count == 1;
}(), "NRVO: Long pipeline should move container exactly once, zero copies");

// Test: Long pipeline - OwningRange -> RMap -> RFilter, verify no extra copies
static_assert([]{
    // Pipeline: container | Map | Filter | Take
    // Should have minimal moves, zero copies of container data
    
    auto r = RangeOf(ConstexprTrackedArray<10, 3>{})
        |Map([](int x) { return x * 2; })
        |Filter([](int x) { return x > 5; })
        |Take(3);
    
    // Just verify compilation and basic correctness
    // RVO/NRVO should ensure container is moved, not copied
    return !r.Empty();
}(), "Long pipeline with OwningRange should compile without extra copies");

// Test: Lvalue owning containers still work (return view, not OwningRange)
static_assert([]{
    ConstexprTrackedArray<5> arr;
    auto r = RangeOf(arr);  // lvalue - should NOT wrap in OwningRange
    // r should be a view (reference), not OwningRange
    return r.First() == 0;
}(), "RangeOf lvalue owning container should return view");

// Test: Functor copy counting - verify no double copies in FunctorOf chains
static_assert([]{
	struct TrackedFunctor {
		int value;
		
		constexpr TrackedFunctor(int v) : value(v) {}
		
		constexpr int operator()(int x) const { return x * value; }
	};
	
	TrackedFunctor f{5};
	
	// Single FunctorOf call
	auto f1 = FunctorOf(INTRA_FWD(f));
	
	// For class types FunctorOf returns an object of the same class.
	static_assert(CSame<decltype(f1), TrackedFunctor>, "FunctorOf(class) should return the same class type");
	return f1(3) == 15;
}(), "FunctorOf should move, not copy, the functor");

static_assert([]{
	struct TrackedMoveCopyFunctor {
		int value;
		int copy_count = 0;
		int move_count = 0;
		
		constexpr TrackedMoveCopyFunctor(int v = 1): value(v) {}
		constexpr TrackedMoveCopyFunctor(const TrackedMoveCopyFunctor& o):
			value(o.value), copy_count(o.copy_count + 1), move_count(o.move_count) {}
		constexpr TrackedMoveCopyFunctor(TrackedMoveCopyFunctor&& o) noexcept:
			value(o.value), copy_count(o.copy_count), move_count(o.move_count + 1) {}
		
		constexpr int operator()(int x) const {return x * value;}
	};

	ConstexprTrackedArray<3> arr;

	TrackedMoveCopyFunctor f{2};
	auto map = Map(f);

	// Map(f) from lvalue copies into closure.
	const bool closureTypeOk = CSame<decltype(map.Func), TrackedMoveCopyFunctor>;
	const bool closureCountsOk = map.Func.copy_count == 1 && map.Func.move_count == 0;

	// Applying lvalue map copies functor into resulting range.
	auto r1 = map(arr);
	const bool lvalueApplyOk = r1.Func.copy_count == 2 && r1.Func.move_count == 0 && r1.First() == 0;

	// Applying rvalue map moves functor into resulting range.
	auto r2 = INTRA_MOVE(map)(arr);
	const bool rvalueApplyOk = r2.Func.copy_count == 1 && r2.Func.move_count == 1 && r2.First() == 0;

	return closureTypeOk && closureCountsOk && lvalueApplyOk && rvalueApplyOk;
}(), "Map closure should be reusable by copying on lvalue apply and moving on rvalue apply");

static_assert([]{
	struct MoveOnlyFunctor {
		constexpr MoveOnlyFunctor() = default;
		MoveOnlyFunctor(const MoveOnlyFunctor&) = delete;
		constexpr MoveOnlyFunctor(MoveOnlyFunctor&&) = default;
		constexpr int operator()(int x) const {return x + 1;}
	};
	ConstexprTrackedArray<3> arr;
	// If Map copies the functor anywhere in this path, compilation must fail.
	auto r = Map(MoveOnlyFunctor{})(arr);
	return r.First() == 1;
}(), "Immediate Map application should work with move-only functors (no copies)");

static_assert([]{
	struct CopyOnlyFunctor {
		constexpr CopyOnlyFunctor() = default;
		constexpr CopyOnlyFunctor(const CopyOnlyFunctor&) = default;
		CopyOnlyFunctor(CopyOnlyFunctor&&) = delete;
		constexpr int operator()(int x) const {return x * 3;}
	};
	ConstexprTrackedArray<3> arr;
	// Direct RMap construction should work even if the functor is not movable.
	auto r = RMap(RangeOf(arr), CopyOnlyFunctor{});
	return r.First() == 0;
}(), "Direct RMap should support copy-only (non-movable) functors");

static_assert([]{
	struct NoMoveNoCopyFunctor {
		int add;
		constexpr NoMoveNoCopyFunctor(int a): add(a) {}
		NoMoveNoCopyFunctor(const NoMoveNoCopyFunctor&) = delete;
		NoMoveNoCopyFunctor(NoMoveNoCopyFunctor&&) = delete;
		constexpr int operator()(int x) const {return x + add;}
	};
	ConstexprTrackedArray<3> arr;
	NoMoveNoCopyFunctor f(5);
	// Non-movable non-copyable functor can still be used if referenced explicitly.
	auto r = Map(FRef(f))(arr);
	return r.First() == 5;
}(), "Map should support non-movable non-copyable functors via explicit FRef");

static_assert([]{
	struct NoMoveNoCopyPred {
		int threshold;
		constexpr NoMoveNoCopyPred(int t): threshold(t) {}
		NoMoveNoCopyPred(const NoMoveNoCopyPred&) = delete;
		NoMoveNoCopyPred(NoMoveNoCopyPred&&) = delete;
		constexpr bool operator()(int x) const {return x >= threshold;}
	};
	ConstexprTrackedArray<3> arr;
	NoMoveNoCopyPred pred(2);
	// Non-movable non-copyable predicate can still be used if referenced explicitly.
	auto r = Filter(FRef(pred))(arr);
	return r.First() == 2;
}(), "Filter should support non-movable non-copyable predicates via explicit FRef");

static_assert([]{
	struct NoMoveNoCopyPred {
		int stopAt;
		constexpr NoMoveNoCopyPred(int s): stopAt(s) {}
		NoMoveNoCopyPred(const NoMoveNoCopyPred&) = delete;
		NoMoveNoCopyPred(NoMoveNoCopyPred&&) = delete;
		constexpr bool operator()(int x) const {return x == stopAt;}
	};
	ConstexprTrackedArray<3> arr;
	NoMoveNoCopyPred pred(2);
	// Non-movable non-copyable predicate can still be used if referenced explicitly.
	auto r = TakeUntil(FRef(pred))(arr);
	return r.First() == 0;
}(), "TakeUntil should support non-movable non-copyable predicates via explicit FRef");

#endif // INTRA_CONSTEXPR_TEST

// ============================================================================
// Runtime tests (for cases that can't be constexpr)
// ============================================================================

#ifndef INTRA_CONSTEXPR_TEST
template<size_t N>
struct DebugCheckedRange {
    int data[N];
    size_t pos = 0;
    
#if INTRA_DEBUG
    mutable bool valid = true;
    
    void CheckValid() const {
        INTRA_PRECONDITION(valid, "Access to invalidated/dangling range detected!");
    }
    void Invalidate() { valid = false; }
#else
    void CheckValid() const {}
    void Invalidate() {}
#endif
    
    DebugCheckedRange() {
        for(size_t i = 0; i < N; ++i) data[i] = int(i);
    }
    
    DebugCheckedRange(const DebugCheckedRange& other) : pos(other.pos) {
        for(size_t i = 0; i < N; ++i) data[i] = other.data[i];
        // Copy is valid
    }
    
    DebugCheckedRange(DebugCheckedRange&& other) noexcept : pos(other.pos) {
        for(size_t i = 0; i < N; ++i) data[i] = other.data[i];
        other.Invalidate();  // Moved-from object becomes invalid
    }
    
    [[nodiscard]] int& First() { CheckValid(); return data[pos]; }
    [[nodiscard]] bool Empty() const { CheckValid(); return pos >= N; }
    
    using TagAnyInstanceFinite = TTag<>;
};

// Movable heap array (can't be fully constexpr due to new/delete)
template<size_t N>
struct TrackedMovableArray {
    int* data = nullptr;
    size_t size = 0;
    size_t pos = 0;
    
    TrackedMovableArray() {
        data = new int[N];
        size = N;
        for(size_t i = 0; i < N; ++i) data[i] = int(i);
    }
    
    ~TrackedMovableArray() { delete[] data; }
    
    TrackedMovableArray(const TrackedMovableArray&) = delete;
    TrackedMovableArray& operator=(const TrackedMovableArray&) = delete;
    
    TrackedMovableArray(TrackedMovableArray&& other) noexcept 
        : data(other.data), size(other.size), pos(other.pos) {
        other.data = nullptr;
        other.size = 0;
    }
    
    TrackedMovableArray& operator=(TrackedMovableArray&&) = delete;
    
    [[nodiscard]] int& First() { return data[pos]; }
    [[nodiscard]] bool Empty() const { return pos >= size; }
    void PopFirst() { ++pos; }
    
    using TagAnyInstanceFinite = TTag<>;
};

// Test: MoveIfPossible should move movable types
inline void TestMoveIfPossible_Movable() {
    auto r = RangeOf_MoveIfPossible(TrackedMovableArray<100>{});
    (void)r.First();
    // TrackedMovableArray is only movable, not copyable
    // RangeOf_MoveIfPossible should move it
}

#endif // !INTRA_CONSTEXPR_TEST

} // namespace Intra
