# Precision Passport

A C++17 CPU numerical differential lab that reduces a reproducible precision mismatch to a small exact input. Apple supplies Metal and Core ML and already provides validation/comparison tools. This project explores failure reproduction and reduction around that problem; the current runner is portable CPU code, not an Apple framework integration.

## Run

```sh
make test
make demo
./build/precision-passport reduce example.hex
```

Requires GNU Make and an IEEE binary32 C++17 implementation. Input is 1..4096 eight-digit binary32 hex words. Exact bits avoid decimal parsing ambiguity. `reduce` uses explicit absolute tolerance 0.25 and relative tolerance zero. The header API accepts other nonnegative finite tolerances.

## Actual numeric paths

The single supported graph is sequential sum. The reference accumulates original float32 inputs in host `long double`, then returns double. The wide control accumulates in binary32. The low-precision candidate converts inputs and each add result to software IEEE binary16 using nearest-ties-even rounding, including subnormal, overflow, signed zero and NaN/Inf handling. These are **software arithmetic semantics**, not claims about how a Metal/Core ML backend fuses or accumulates operations. No fast-math flags are used.

Comparisons distinguish finite error, NaN classification and signed infinity classification. Finite values pass when error <= absolute tolerance + relative tolerance * absolute reference. Both NaNs match classification; that does not prove their cause or payload equivalent. Opposite infinities and one-sided NaNs fail. Signed zeros compare numerically equal. Exception-class results do not have a meaningful finite error; their stored zero error field must be interpreted together with classification.

The reducer greedily removes one input element while preserving a mismatch, repeating to a fixed point. It guarantees one-element-deletion minimality for this predicate, not globally smallest input, graph reduction or semantic correctness. The passport includes graph, runner semantics, compiler, C++ version, reference mantissa width, exact reduced input bits, tolerances, results and explicit false Metal/Core ML flags.

## Actual verification

**11 groups / 63,654 assertions pass**. This includes all 63,488 finite binary16 words round-tripping through binary32, independently authored conversion goldens, tie-to-even/subnormal/overflow boundaries, classification cases, exact stable sums, identical-run controls, invalid inputs/tolerances, and reducer preservation/minimality.

The designed demo starts with `[0,2048,1,-2048,0]` and reduces to `[2048,1]`: reference/wide control 2049, software binary16 result 2048. This shows expected loss under the stated arithmetic, not an Apple bug. Local artifacts: `demo.json`, `test-results.txt`. GitHub Actions runs the same checks; inspect Actions for the latest hosted result.

## Sources and next steps

- [Apple Metal shader validation](https://developer.apple.com/documentation/xcode/validating-your-apps-metal-shader-usage)
- [Core ML debugging/performance utilities](https://github.com/apple/coremltools/blob/main/docs-guides/source/mlmodel-debugging-perf-utilities.md)
- [Core ML Tools numerical guidance](https://apple.github.io/coremltools/docs-guides/source/faqs.html)

No Metal, Core ML, Apple GPU/Neural Engine, device compute-unit selection, model inference, normalization graph, ULP comparison, performance measurement or deployment has been validated. `long double` precision is platform-dependent and is not an exact-real oracle for arbitrary extreme cancellation; goldens are deliberately representable. The next milestone needs actual compatible Apple hardware, a pinned graph conversion/runtime, device provenance, cross-run determinism checks, and measured comparison against existing validators.
