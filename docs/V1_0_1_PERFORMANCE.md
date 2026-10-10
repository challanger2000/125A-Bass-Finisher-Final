# Bass Finisher V1.0.1 — CPU Optimization Candidate

- Base: authoritative V1.0.0 `main` commit `6169717015fcecaf872be007aadbb6660fc8e00b`
- Same V1 identity, plug-in class IDs, state and parameter IDs. No new controls.
- 4096-tap stereo MATCH uses zero-latency 128-sample direct head with 31 frequency-domain partitions.
- New MATCH kernels are prepared outside the realtime callback and installed from SPSC mailbox using bounded copies.
- Expensive LOW CONTROL / LOW CUT / MASS / makeup coefficient regeneration is skipped ONLY when its input parameter is numerically unchanged.
- Existing adaptive bass FINISH, saturation, sub containment, MASS harmonics, automatic input handling, FINAL and output semantics retained.
- Regression: direct-vs-partitioned stereo sample null; same-runner FIR CPU ratio; Bass processor/state/GUI contracts; 125A Plugin Tester and Steinberg Validator.
- Do NOT publish FINAL until measured whole-plugin CPU A/B and approved in Studio One.
