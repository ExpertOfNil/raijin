# Raijin Development Plan

## Objective

Develop Raijin as a reusable, Linux-first C11 graphics and visualization
library, with particular consideration for computer vision, reverse
engineering, photogrammetry, and spatial-localization workloads.

Maintain an immediate-mode drawing model while supporting optional persistent
GPU resources where repeated CPU-to-GPU uploads would be prohibitively
expensive.

Prioritize:

1. Correctness and predictable failure behavior.
2. Experimental usability.
3. Foundational performance architecture.
4. Focused validation and measurement.
5. Enhancements justified by representative use.

## Status Conventions

- `[X]` means the stated implementation, documentation, test implementation,
  or manual observation is complete.
- Implementation checkoffs do not imply exhaustive runtime acceptance.
- Test implementation does not imply a verified passing run of the current
  source revision.
- Historical results remain evidence for their recorded scope, not for
  subsequent changes.
- Unchecked validation items require additional evidence.

## Current Capabilities

The public API currently supports:

- Headless and SDL3 windowed contexts.
- Dependency-free public headers.
- Camera look-at, look-to, view-matrix setting, and aspect-adjustable
  projection setting.
- Automatic orbit, pan, and zoom interaction.
- Native window close requests, process quit, and Escape-to-close.
- Custom indexed triangle meshes.
- Optional mesh edge indices and global edge visibility.
- Context-bound mesh handles.
- Builtin plane, disc, cube, cylinder, cone, and UV sphere meshes.
- Dependency-free TRS construction with a uniform scale parameter.
- High-level mesh and primitive drawing helpers.
- Cylinder-backed 3D line drawing.
- Bulk transient instance submission.
- Context-bound assemblies with copied local mesh instances.
- Checked public assembly creation and component insertion.
- TRS-only public assembly drawing.
- Parent/local transform and RGBA color composition.
- Repeated drawing of one assembly at independent placements.
- Logical rollback on assembly submission failure.
- Per-mesh CPU submission bins and reusable GPU instance buffers.
- Headless RGBA8 readback.
- Drawable-pixel resizing and minimized-window handling.
- Bounded surface reconfiguration and replacement.
- An external-consumer starter using manually reconstructed static linkage.

Installed CMake package consumption and general multiple-window event routing
are not yet implemented.

## Rendering and Resource Model

### Transient Submission

1. The caller submits visible instances before each frame.
2. Raijin copies instances into per-mesh CPU submission storage.
3. Raijin uploads populated instance ranges.
4. Raijin attempts rendering.
5. The engine consumes logical submissions after that attempt, including
   minimized, recovery, and failure paths.

CPU and GPU capacity is retained for reuse. Retained capacity does not make
instance contents persistent across frames.

Transient submission remains appropriate for debug geometry, short-lived
visualization, moderate instance collections, and data regenerated each frame.

### Assemblies

Assemblies are flat, reusable CPU-side definitions:

1. Create an empty assembly.
2. Add mesh references and local instances.
3. Draw the assembly before each visible frame.
4. Supply independent parent translation, rotation, uniform scale, and color
   for each placement.

Each draw expands components into ordinary transient submissions.

```text
final_transform = parent_transform * local_transform
final_color = parent_color * local_color
```

Assemblies do not provide nested hierarchy, retained visibility, persistent
GPU instance storage, or GPU draw-level transforms.

### Ownership

- Public mesh and assembly handles encode context identity and an append-only
  resource index.
- Meshes and assemblies live until context destruction.
- Assemblies reference meshes but do not own them.
- Individual mesh/assembly destruction and assembly component editing are
  unsupported.
- Operations on one context require external serialization.

### Future Persistence

Persistent instance resources will be optional and additive. They will not
replace transient submission or introduce a retained scene graph.

## Validation Evidence

### Existing Automated Coverage

The repository registers four tests:

- Public argument validation.
- Headless rendering/readback and mesh-handle checks.
- Internal mesh geometry checks.
- Internal camera checks.

Existing tests cover multiple contexts, cross-context mesh rejection,
destroyed-context mesh rejection, instance-buffer growth, basic rendering,
and readback capacity handling.

Dedicated custom-mesh contract, assembly contract, surface-state-machine,
paging, and persistent-resource tests are not yet implemented.

Retained CTest logs for `build` and `build-headless` report all four tests
passing on October 20, 2026. These logs do not identify the source revision.
Both configurations use executables under the shared source-tree `bin/`
directory, so the logs alone do not establish binary-variant provenance.

Do not treat those logs as revision-qualified assembly validation or explicit
Debug/Release package validation.

### Recorded Manual Acceptance

- [X] Verify the starter exits through the window-manager close button.
- [X] Verify close handling after resize and minimize/restore.
- [X] Construct a sphere-based point-cloud assembly in `raijin_alignment`.
- [X] Draw one assembly definition at independent placements.
- [X] Observe a translated and rotated duplicate point cloud.

Native close implementation is present in commit `1cf163a`.
The close observations above are previously recorded manual acceptance,
not new runs performed during this status review.

The external point-cloud consumer uses a manually vendored library bundle,
not the intended installed CMake package.

Manual success does not establish allocation-failure correctness, exhaustive
handle validation, general multiple-window support, or large-scale performance.

## Priority Definitions

- **P0:** Required before experimental package readiness.
- **P1:** Important validation and contract coverage, grouped with broader
  testing unless an issue blocks earlier work.
- **P2:** Foundational architecture required before scaling or persistence.
- **P3:** Improvements justified by representative use.
- **Deferred:** Do not begin without evidence of need.

# Ordered Development Tasks

## 1. P0 — Complete Surface Recovery Acceptance

### Implemented Ownership

- [X] Create native surfaces in the window backend.
- [X] Keep surface configuration and adoption in the renderer.
- [X] Unconfigure and release the old surface before replacement creation.
- [X] Validate replacement support for the existing format and render-attachment
      usage before adoption.
- [X] Leave the replacement path detached if creation or adoption fails.
- [X] Reject windowed rendering when no surface is attached.
- [X] Release the surface before native-window destruction.
- [X] Commit the release-before-replacement correction separately:
      `637d45c` — `Release old surface before creating its replacement`.

Implementation references:

- Replacement orchestration: `include/raijin.h:584-595`
- Surface release/adoption: `src/renderer.c:1748-1838`
- Destruction ordering: `include/raijin.h:328-342`

### Implemented Recovery State Machine

Recovery is bounded per configured drawable extent:

1. First outdated acquisition requests reconfiguration.
2. A subsequent outdated acquisition requests one replacement.
3. An outdated or lost replacement returns failure.

- [X] Explicit resize configuration resets recovery to `IDLE`.
- [X] Optimal/suboptimal acquisition resets recovery to `IDLE`.
- [X] Outdated reconfiguration advances to `RECONFIGURED`.
- [X] Replacement adoption advances to `REPLACED`.
- [X] Explicit resize does not preserve prior escalation state.
- [X] Zero-extent recovery/configuration paths avoid configuring zero-sized
      textures or resetting recovery state.
- [X] Zero-extent recovery preserves the last configured extent.
- [X] Engine render attempts consume transient submissions on normal,
      minimized, recovery, and failure paths.

Implementation references:

- Acquisition transitions: `src/renderer.c:1607-1666`
- Resize/reconfiguration: `src/renderer.c:1729-1745`
- Submission clearing: `include/raijin.h:515-522,612-614`

### Remaining Work

- [ ] Explicitly validate initial surface usage support, consistently with
      replacement adoption.
- [ ] Add deterministic state-machine or fake-backend tests.
- [ ] Verify ordinary resize begins a new recovery episode.
- [ ] Verify first outdated acquisition requests reconfiguration.
- [ ] Verify repeated outdated acquisition requests replacement.
- [ ] Verify outdated/lost replacement returns a bounded failure.
- [ ] Verify resize after escalation starts a new episode.
- [ ] Verify replacement failure leaves rendering safely detached.
- [ ] Verify zero-extent and minimized behavior under real window-system events.
- [ ] Complete sustained rapid-resize acceptance.

Surface configuration calls currently advance state without independently
observing backend configuration success. Improve GPU error reporting under
Section 11 before making stronger guarantees.

### Open Rapid-Resize Blocker

On the tested SDL3 X11/Vulkan stack, sustained rapid resizing can leave the
replacement surface outdated. Raijin returns a render error after bounded
recovery rather than looping indefinitely.

During one captured run, Vulkan reported `VK_ERROR_OUT_OF_DATE_KHR`.
The Vulkan surface extent was reported as 1999 × 783 while the X11 client
window measured 2027 × 849.

The responsible layer is unestablished. These measurements are one observation,
not a proven root cause. Evaluate a coherent WGPU-native dependency update
before claiming successful rapid-resize recovery.

## 2. P0 — Resolve Public Vertex-Color Behavior

Vertex RGB is public and uploaded, but the solid shader currently uses only
instance color.

Implementation reference: `assets/shaders/solid_shader.wgsl:70-74`.

- [ ] Choose an explicit vertex-color policy.
- [ ] Update the solid shader.
- [ ] Make builtin vertex colors consistent with that policy.
- [ ] Verify custom vertex colors affect rendered output.
- [ ] Document vertex color encoding and interaction with instance color.
- [ ] Re-evaluate vertex bandwidth before performance baselines.

Recommended initial policy:

```text
final_color = vertex_color * instance_color
```

Most generated primitives use white vertex RGB, but the builtin cube has
colored vertices. Decide whether those colors are intentional before enabling
multiplication.

Assembly parent/component color multiplication is already implemented on the
CPU. It does not resolve vertex-color shading.

## 3. P0 — Resolve Transform and Normal Correctness

The solid shader transforms normals directly with the model matrix, which is
incorrect for arbitrary nonuniform scaling and shear.

Implementation reference: `assets/shaders/solid_shader.wgsl:73`.

Choose a transform contract:

1. Restrict transforms to rigid motion and uniform scaling.
2. Compute inverse-transpose normal transforms in the shader.
3. Store additional normal-transform data per instance.
4. Adopt a decomposed transform representation.

Favor correctness without increasing every instance's size unless measurements
justify it.

Existing TRS validation checks finite inputs and positive scale, but does not
verify that the supplied rotation is orthonormal or nonsingular. Raw submission
and assembly insertion accept arbitrary local matrices.

- [ ] Define the supported transform class.
- [ ] Define singular and non-finite transform behavior.
- [ ] Handle nonuniform scaling under the selected policy.
- [ ] Apply validation consistently to helpers, raw submission, and assemblies.
- [ ] Consider overflow in constructed/composed transforms and colors.
- [ ] Correct cube normals if flat faces are intended.
- [ ] Separate cylinder side and cap vertices/normals.
- [ ] Separate cone side and base vertices/normals.
- [ ] Verify line-helper lighting.
- [ ] Document the final transform and normal contract.

Existing cylinder/cone tests encode shared-ring topology and must be updated
when cap vertices are separated.

## 4. Completed — Native Window Close Requests

- [X] Translate `SDL_EVENT_WINDOW_CLOSE_REQUESTED`.
- [X] Check the native window ID before applying the request.
- [X] Translate matching close requests into the existing quit event.
- [X] Set the context close state through the existing engine handler.
- [X] Preserve process-wide `SDL_EVENT_QUIT`.
- [X] Preserve Escape-to-close.
- [X] Commit implementation:
      `1cf163a` — `Handle native window close requests`.
- [X] Record manual starter close-button acceptance.
- [X] Record manual close acceptance after resize and minimize/restore.

Implementation references:

- Backend translation and window-ID check: `src/raijin_sdl3.c:88-97`
- Engine close-state handling: `include/raijin.h:220-224`

This completes controlled single-window close support, not general
multiple-window routing.

The backend still drains the process-global queue. A close request for another
window can be consumed and discarded rather than routed to its owner.
Other window/input events also lack comprehensive routing. Address those
limitations under Section 7.

## 5. Build a Minimal Experimental Package

Target a local, Linux-first, static experimental package:

- One backend variant per installation prefix initially.
- SDL3-enabled package first.
- Experimental `0.x` version.
- No stable ABI or production binary-compatibility claim.

Intended consumer interface:

```cmake
find_package(Raijin CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE Raijin::Raijin)
```

### 5.1 Existing Build Foundations

- [X] Build a static `raijin` target.
- [X] Provide an in-tree `Raijin::Raijin` alias.
- [X] Require C11 for the library.
- [X] Support SDL3-enabled and headless backend selection.
- [X] Keep public headers free of third-party dependencies.
- [X] Embed shader sources for binary use.
- [X] Propagate the logging target's thread dependency in the in-tree build.

These are build foundations, not installed-package support.

### 5.2 Normalize CMake

- [ ] Make the root project C-only unless C++ becomes necessary.
- [ ] Add an experimental project version.
- [ ] Include `GNUInstallDirs` and `CMakePackageConfigHelpers`.
- [ ] Remove the global source-tree executable output directory.
- [ ] Prevent executable overwrites between build trees.
- [ ] Set `EXPORT_NAME Raijin` on the real library target.
- [ ] Give exported internal dependencies stable target names.

### 5.3 Establish the Install Boundary

- [ ] Add an install include interface.
- [ ] Install only the public `include/raijin/raijin.h`.
- [ ] Preserve `<raijin/raijin.h>` as the consumer include path.
- [ ] Exclude private headers, tests, internal examples, development assets,
      and shader sources from binary installation.
- [ ] Explain that `include/raijin.h` is private despite its similar name.

### 5.4 Package Static Dependencies

Current dependencies include:

- `libraijin.a`
- `liblogging.a`
- `libwgpu_native.a`
- `libcglm.a`, unless clean-link verification establishes it is unnecessary
- `Threads::Threads`
- SDL3 for the SDL-enabled variant
- Platform math and dynamic-loader libraries

- [ ] Replace raw source-tree archive paths with package targets.
- [ ] Install bundled WebGPU in a package-owned location.
- [ ] Verify cglm archive necessity across clean Debug/Release and SDL/headless
      links.
- [ ] Remove the cglm archive dependency if unnecessary; otherwise provide a
      package-owned imported target.
- [ ] Install/export logging with its thread dependency.
- [ ] Use `CMAKE_DL_LIBS` instead of literal `dl` where practical.
- [ ] Preserve static link ordering through target dependencies.

### 5.5 Export and Configure the Package

- [ ] Install/export library targets under `Raijin::`.
- [ ] Generate `RaijinTargets.cmake`.
- [ ] Add `cmake/RaijinConfig.cmake.in`.
- [ ] Generate `RaijinConfigVersion.cmake`.
- [ ] Eliminate source-tree absolute paths from installed interfaces.
- [ ] Document Linux/static constraints and Vulkan/driver requirements.

The generated configuration must:

1. Run `@PACKAGE_INIT@`.
2. Include `CMakeFindDependencyMacro`.
3. Find Threads.
4. Find SDL3 when required by the installed variant.
5. Define package-relative imported targets for bundled archives.
6. Include `RaijinTargets.cmake`.
7. Run `check_required_components(Raijin)`.

### 5.6 Convert the Starter

The current starter manually reconstructs imported targets and archive linkage.

Implementation reference: `examples/starter/CMakeLists.txt:9-55`.

- [ ] Replace that reconstruction with `find_package(Raijin CONFIG REQUIRED)`.
- [ ] Link only `Raijin::Raijin`.
- [ ] Remove knowledge of source/build directories, individual archives, and
      transitive dependency linkage.

### 5.7 Installed-Package Smoke Sequence

- [ ] Configure and build Raijin.
- [ ] Install to a temporary prefix.
- [ ] Configure the starter using only `CMAKE_PREFIX_PATH`.
- [ ] Build the starter.
- [ ] Run windowed acceptance manually.
- [ ] Scan installed interfaces for source-tree paths.
- [ ] Add a headless consumer when package variants are formalized.

Manually vendored consumption does not satisfy this milestone.

## 6. Complete Public Contract Documentation

Track public-facing documentation separately from implementation notes in this
plan.

### 6.1 Transient Submission

The implementation copies synchronously and clears submissions after engine
render attempts. The general public submission declaration still lacks the
complete contract.

- [ ] Document synchronous copying and no retained caller pointers.
- [ ] Document one-render-attempt lifetime.
- [ ] Document consumption after success, failure, minimization, and recovery.
- [ ] Document per-frame resubmission beside the general submission/render API.

The assembly drawing comment and starter already explain per-frame submission
for their respective paths.

### 6.2 Mesh Ownership

- [X] Document synchronous geometry borrowing and no retained geometry pointers
      in the public header.
- [ ] Correct the header comment's obsolete `raijin_create_mesh()` spelling to
      `raijin_mesh_create()`.
- [ ] Document context-bound handles and cross-context rejection.
- [ ] Document context-lifetime resources and lack of individual destruction.
- [ ] Document the limitations for repeated dataset loading/unloading.

Public borrowing comment: `include/raijin/raijin.h:65-72`.

### 6.3 Readback

- [ ] Document headless-only behavior.
- [ ] Specify row orientation and tightly packed output.
- [ ] Specify RGBA format, color encoding, and alpha behavior.
- [ ] Specify blocking, timeout, and device-loss behavior.
- [ ] Define behavior before the first render.
- [ ] Define which rendered texture contents are returned.
- [ ] Document required capacity and failure behavior.

There is no completed-frame validity flag enforcing a prior-render requirement.
Choose the contract rather than implying an existing check.

### 6.4 Camera Projection

- [X] Document column-major matrices.
- [X] Document the aspect-adjustable projection requirement.
- [X] Document preserved vertical scale and adapted horizontal scale.
- [X] Document adaptation immediately on setting and after resize.

Public documentation: `include/raijin/raijin.h:109-118`.

### 6.5 Diagnostics

- [ ] Add `raijin_result_string()`.
- [ ] Return stable strings for every public result.

Configurable logging/diagnostic callbacks remain deferred until needed.

### 6.6 Assemblies

- [X] Document context ownership and lifetime.
- [X] Document synchronous local-instance copying and mesh nonownership.
- [X] Document parent/local transform and RGBA composition.
- [X] Document logical all-or-nothing submission failure.
- [X] Document per-attempt resubmission.

Public documentation: `include/raijin/raijin.h:198-249`.

Remaining consumer guidance:

- [ ] Add a concise create/add/draw example.
- [ ] Explain reuse of one definition versus independent definitions.
- [ ] Explain that CPU definitions do not eliminate expansion or GPU uploads.
- [ ] Explain local-origin rotation versus centroid rotation.
- [ ] Document unsupported component editing and individual destruction.
- [ ] Publish the finalized transform-validity policy after Section 3.
- [ ] Publish threading/external-serialization requirements.

## 7. Decide Interactive Event Ownership

Current behavior:

- SDL's process-global event queue is drained internally.
- Recognized events are applied automatically.
- Unrecognized events are discarded.
- Escape and Tab are hardcoded.
- Input is not returned to applications.
- There is no controller-disable switch.
- Native close requests are filtered by window ID, but there is no general
  per-window event routing or queue.
- Events belonging to other windows can be consumed rather than delivered
  to their owners.

Potential models:

1. Public SDL-independent event polling.
2. Caller-supplied translated events.
3. Automatic and application-controlled modes.
4. Backend routing with per-window queues.

- [X] Keep the current public API SDL-independent.
- [ ] Define event-polling ownership.
- [ ] Define whether automatic controls can be disabled.
- [ ] Define remapping and application-visible input.
- [ ] Define comprehensive window-ID routing.
- [ ] Document single-window limitations until routing exists.

This does not block a controlled single-window experiment, but does block
general embedding claims.

## 8. Add Camera Operations Only as Needed

The current public API already provides look-at, look-to, view-matrix setting,
and projection-matrix setting.

Potential additions remain evidence-driven:

- [ ] View/projection getters.
- [ ] Semantic orbit, pan, and zoom.
- [ ] Orbit-distance access.
- [ ] Controller enable/disable.
- [ ] Sensitivity and remapping.

Private orbit/pan/zoom operations already exist; do not expose them wholesale.

Clarify terminology before adding focal-distance APIs: the private focal
distance currently represents projection scale, not orbit distance.

## 9. Evaluate External Experimental Use

The `raijin_alignment` experiment demonstrates that reusable assemblies and
independent placements are useful.

- [X] Obtain initial manual assembly usability feedback.
- [X] Exercise one definition at multiple placements.
- [ ] Repeat consumption through the installed CMake package.
- [ ] Record representative point counts and workload characteristics.
- [ ] Record remaining usability problems.

Evaluate:

- Package and public API ergonomics.
- Window/event and camera-control limitations.
- Point counts and static/changing ratios.
- Mesh choices and sphere geometry cost.
- Submission time, assembly expansion, and upload bandwidth.
- Coordinate precision.
- Need for resource statistics, persistence, culling, LOD, or screen-space sizing.
- Need for assembly editing or individual resource destruction.

Record evidence before adding abstractions.

## 10. Establish a Minimal Performance Baseline

Begin after vertex-color and transform contracts are stable.

- [ ] Measure CPU submission and total frame time.
- [ ] Record steady-state CPU/GPU utilization.
- [ ] Test 10K, 100K, and 1M instances where practical.
- [ ] Compare cubes, low-detail spheres, and the existing UV sphere.
- [ ] Compare bulk submission and assembly expansion.
- [ ] Record CPU/GPU capacity growth and retained memory.
- [ ] Measure with edges disabled.
- [ ] Record compiler, OS, CPU, GPU, driver, and Vulkan details.
- [ ] Preserve results for paging/persistence comparisons.

A small reproducible experiment is sufficient; a permanent benchmark framework
is not required.

## 11. P2 — Device Limits and GPU Errors

Before paging:

- [ ] Query adapter-supported and device-supported limits.
- [ ] Store relevant limits, including `maxBufferSize`.
- [ ] Validate mesh and instance allocations against device limits.
- [ ] Install an uncaptured-error callback.
- [ ] Distinguish validation failures from allocation failures where practical.
- [ ] Return defined public failures instead of relying on GPU validation.
- [ ] Define explicit requested-limit policy.

Existing foundations:

- [X] Check mesh byte-size arithmetic and submission-count overflow.
- [X] Check dynamic-array allocation arithmetic.
- [X] Map timeout and device-loss results in applicable public paths.

Preserve arithmetic checks independently of device-limit checks. Existing
checks do not imply complete GPU limit/error handling.

## 12. P2 — Local Mesh Bounds

- [ ] Validate finite vertex positions.
- [ ] Calculate a local AABB.
- [ ] Optionally calculate a local bounding sphere.
- [ ] Store bounds with each mesh.
- [ ] Define bounds for degenerate geometry.
- [ ] Use bounds later for framing, selection, queries, culling, and LOD.

Existing behavior rejects empty required geometry and does not retain full
caller geometry. Preserve that ownership model.

Do not implement full culling here. Add aggregate assembly bounds only when
representative use requires them.

## 13. P2 — CPU/GPU Instance Paging

The current implementation has one CPU array and one GPU instance buffer per
mesh. Capacity growth is not paging.

- [ ] Derive bounded page capacity from actual device limits.
- [ ] Page CPU submissions and GPU instance storage.
- [ ] Fill existing page tails before allocating more pages.
- [ ] Allocate lazily and retain capacity.
- [ ] Upload populated ranges only.
- [ ] Draw populated pages inside the existing solid pass.
- [ ] Reuse uploaded pages for the edge pass.
- [ ] Clear logical counts after every engine render attempt.
- [ ] Release all pages at destruction.
- [ ] Return defined allocation/limit failures.
- [ ] Keep page boundaries private.
- [ ] Preserve transient submission behavior.
- [ ] Preserve logical all-or-nothing assembly submission across pages.

Preserve existing single-buffer strengths: capacity reuse, populated-range
uploads, and solid/edge upload reuse.

The current assembly rollback directly decrements append-only per-mesh counts.
Redesign it as necessary for pages rather than carrying those assumptions
forward blindly.

Paging addresses allocation limits and capacity robustness, not total rendering
or upload cost.

## 14. Re-run the Baseline After Paging

- [ ] Compare submission time and frame time.
- [ ] Measure extra draw-call overhead.
- [ ] Measure retained memory after peak workloads.
- [ ] Check unused page capacity.
- [ ] Exercise alternating large/small frames and multiple mesh types.
- [ ] Verify solid/edge upload reuse.
- [ ] Include representative assembly workloads.
- [ ] Adjust page sizing only from measurements.

## 15. P2 — Shared Resource Lifetime Infrastructure

Current mesh/assembly handles already use:

- Nonzero context identity.
- Append-only index encoding.
- Zero invalid public values.
- Cross-context rejection.
- Non-reused context identities within the supported process lifetime.

Reusable slots and per-slot generations do not exist.

Before individually destroyable or reusable resources:

- [ ] Define shared slot representation and per-slot generations.
- [ ] Define migration from existing handle encoding.
- [ ] Carry forward invalid-value and cross-context conventions.
- [ ] Define stale rejection after slot reuse.
- [ ] Define slot reuse and generation-overflow policies.
- [ ] Define resource behavior after device loss.
- [ ] Define destruction timing and queued-reference behavior.
- [ ] Generalize threading requirements.
- [ ] Decide whether handles need resource-type tagging.

Mesh and assembly typedefs are both integers and have no resource-type
discriminator. Accidental interchange is not reliably rejected when encoded
indices overlap.

Use compatible lifetime conventions for meshes, future assembly destruction,
persistent buffers, textures, and materials.

## 16. Add Individual Mesh Destruction

After shared lifetime infrastructure:

- [ ] Add public mesh destruction.
- [ ] Release GPU buffers and retained CPU submission storage.
- [ ] Reuse slots with incremented generations.
- [ ] Reject stale/cross-context handles.
- [ ] Define behavior with queued submissions.
- [ ] Define behavior for builtin meshes.
- [ ] Define behavior for meshes referenced by assemblies.
- [ ] Preserve complete context cleanup.

Slot reuse must never redirect an assembly component silently to an unrelated
mesh.

Assembly destruction/editing remains evidence-driven follow-up work.

## 17. Design Generic Persistent Instance Buffers

Keep persistence optional and mesh-independent.

Define:

- Context-bound handles, invalid values, generations, and explicit destruction.
- Synchronous borrowed creation/update inputs.
- Zero-length behavior.
- Whole/range updates and range submission.
- Growth/shrink behavior.
- Cross-context/stale rejection and device-loss behavior.
- CPU shadow-copy policy.
- Bounds update/invalidation policy.
- Use with multiple meshes.
- Interaction with transient submissions.
- Partial failure across pages.
- Ordering requirements.

Recommended initial model:

- Use the same internal GPU page abstraction as transient storage.
- Avoid a complete CPU shadow copy by default.
- Require immediate visibility submission every visible frame.
- Do not introduce retained scene visibility.
- Do not couple persistence automatically to CPU assemblies.

## 18. Implement Persistence Incrementally

### 18.1 Creation and Destruction

- [ ] Allocate paged GPU storage.
- [ ] Upload initial contents once.
- [ ] Return checked context-bound handles.
- [ ] Destroy explicitly and reject stale references.

### 18.2 Range Submission

- [ ] Submit a mesh with a persistent range.
- [ ] Validate range arithmetic.
- [ ] Draw across internal pages.
- [ ] Avoid CPU copies and unchanged-data uploads.
- [ ] Keep visibility immediate.

### 18.3 Updates

- [ ] Support whole-buffer and range updates.
- [ ] Preserve untouched ranges across pages.
- [ ] Implement defined growth/shrink behavior.
- [ ] Maintain or conservatively invalidate bounds.
- [ ] Report partial failures predictably.

### 18.4 Mixed Submission

- [ ] Allow transient and persistent instances of one mesh in a frame.
- [ ] Keep ownership distinct.
- [ ] Preserve required ordering.
- [ ] Use correct buffers in solid and edge passes.
- [ ] Avoid unrelated synchronization.

## 19. Add a Persistent Point-Cloud Example

After persistence exists:

- [ ] Generate/load data once.
- [ ] Create a persistent buffer.
- [ ] Move the camera without updating instance data.
- [ ] Submit visibility each frame.
- [ ] Update a small transform/color range.
- [ ] Compare transient and persistent behavior.
- [ ] Report uploaded bytes where practical.
- [ ] Demonstrate destruction and optional use.

The current CPU-assembly experiment does not satisfy this milestone.

## 20. P1 — Consolidated Validation

### 20.1 Existing Test Implementations

- [X] Public null/argument smoke checks.
- [X] Builtin mesh lookup and invalid-handle checks.
- [X] Multiple contexts and cross-context mesh rejection.
- [X] Destroyed-context mesh rejection in a replacement context.
- [X] Headless context creation/destruction.
- [X] Instance-buffer growth submission.
- [X] Basic geometry rendering/readback smoke check.
- [X] Readback size and undersized-buffer rejection.
- [X] Internal cylinder/cone geometry checks.
- [X] Internal camera initialization/view checks.

These are implemented cases, not exhaustive contracts or revision-qualified
passing results. In particular:

- Buffer growth is not page-boundary coverage.
- A nonuniform image is not deterministic shading validation.
- Geometry topology checks do not establish correct normals.
- An empty frame before submission does not prove post-render consumption.

### 20.2 Public Custom-Mesh Contract

- [ ] Create from stack-backed geometry arrays.
- [ ] Modify/release arrays after creation and render successfully.
- [ ] Cover absent and caller-provided edge indices.
- [ ] Reject empty arrays and pointer/count mismatches.
- [ ] Reject invalid triangle/edge count divisibility.
- [ ] Reject out-of-range indices.
- [ ] Cover vertex validity under the selected policy.
- [ ] Confirm failed creation clears the output.
- [ ] Confirm one-attempt transient consumption.

### 20.3 Transform and Shading Contract

- [ ] Rotation and uniform scaling.
- [ ] Nonuniform scaling under the selected contract.
- [ ] Singular/non-finite transforms.
- [ ] Vertex/instance color interaction.
- [ ] Correct cube, cylinder, and cone normals.
- [ ] Correct line lighting.
- [ ] Deterministic rendered expectations where practical.

### 20.4 Window and Surface Contract

Native close has implementation and recorded manual acceptance in Section 4.
The following remain automated/regression coverage goals:

- [ ] Native close translation and window-ID filtering.
- [ ] Escape and process-quit preservation.
- [ ] Ordinary and rapid resize.
- [ ] Minimize/restore and zero extent.
- [ ] Outdated reconfiguration and replacement escalation.
- [ ] Replacement failure and detached rendering.
- [ ] High-DPI drawable changes.
- [ ] Repeated windowed context lifecycles.
- [ ] Fake-backend/state-machine coverage for deterministic failure paths.

### 20.5 Assembly Contract

- [ ] Null arguments and failed-creation output reset.
- [ ] Invalid/out-of-range assembly handles.
- [ ] Cross-context assembly and component mesh handles.
- [ ] Destroyed-context assembly handles.
- [ ] Empty assembly no-op.
- [ ] Copied stack-instance data and source mutation after insertion.
- [ ] Parent/local transform order.
- [ ] Parent translation independent of rotation.
- [ ] Parent/component RGBA multiplication.
- [ ] Repeated mesh references and multiple mesh types.
- [ ] Multiple placements without stored-component mutation.
- [ ] One-attempt submission consumption.
- [ ] Cleanup of nonempty assemblies.

Failure-path cases:

- [ ] Assembly-array growth failure.
- [ ] Component-array growth failure with contents preserved.
- [ ] Failure before the first draw submission.
- [ ] Failure after successful earlier components.
- [ ] Rollback with repeated mesh references.
- [ ] Rollback with preexisting submissions.
- [ ] Restored active-mesh count/order and unchanged earlier values.
- [ ] Successful retry after failure.
- [ ] Count-overflow handling.

Inject failures into the actual aligned-allocation path. Do not assume generic
allocator override macros intercept dynamic-array growth.

### 20.6 Paging and Persistence

After implementation:

- [ ] Exact page fill and boundary-crossing submission.
- [ ] Multiple calls filling one page.
- [ ] Multiple meshes/pages and page reuse.
- [ ] Later-page allocation failure.
- [ ] Solid/edge upload reuse.
- [ ] Device-limit and arithmetic-overflow rejection.
- [ ] Assembly rollback across pages.
- [ ] Persistent slot reuse and stale rejection.
- [ ] Whole/range updates across pages.
- [ ] Mixed transient/persistent submission.
- [ ] Device-loss and partial-failure cleanup.

### 20.7 Broader Quality and Build Validation

- [ ] Sanitizer builds.
- [ ] Multiple-window tests after event routing.
- [ ] API timeout/cancellation behavior tests where applicable.
- [ ] Installed-package and relocated-install tests.
- [ ] Explicit Debug/Release and SDL/headless build/test records.
- [ ] Revision-qualified results for the assembly changes.
- [ ] CI configuration.

Build-harness improvements:

- [ ] Use separate executable output directories for build variants.
- [ ] Explicitly select build type/backend for each validation tree.
- [ ] Run Release tests rather than only building Release.
- [ ] Add installed-package smoke validation.

The existing `compile-all.sh` runs tests in the default and headless trees,
builds Release, runs the Zig build, and builds the manually linked starter.
It is not an installed-package acceptance sequence.

# Evidence-Gated Future Work

## 21. P3 — Frustum Culling

- [ ] Conservative transient culling.
- [ ] Resource/page-level persistent culling.
- [ ] Disable switch and configurable tolerance.
- [ ] Conservative retention of uncertain instances.
- [ ] Measure CPU cost against saved GPU work.

Without spatial partitioning, per-instance culling remains an `O(N)` scan.

## 22. P3 — Mesh LOD Groups

- [ ] Multiple representations per logical object.
- [ ] Projected-bounds selection and per-mesh binning.
- [ ] User thresholds and hysteresis.
- [ ] Manual LOD fallback.

## 23. P3 — Spatial Partitioning and Chunks

- [ ] Chunk metadata independent of GPU pages.
- [ ] User-provided chunks and optional generated chunks.
- [ ] Chunk bounds/ranges and coarse culling.
- [ ] Evaluate Morton ordering, BVHs, octrees, and supplied hierarchies.
- [ ] Streaming/residency.

## 24. P3 — Billboards and Impostors

- [ ] Camera-facing billboards.
- [ ] Analytic sphere impostors with correct fragment depth.
- [ ] Particle/marker/annotation reuse.
- [ ] Explicit transparency and picking behavior.
- [ ] Retain mesh spheres for nearby/large objects.

## 25. P3 — GPU-Driven Culling

Only after measured CPU chunk-culling limitations:

- [ ] Storage-buffer input.
- [ ] Compute visibility/compaction.
- [ ] Visible-index indirection.
- [ ] Indirect drawing and GPU LOD bins.
- [ ] Portable fallback.

## 26. P3 — Large-World Precision

- [ ] CPU double-precision authoritative coordinates.
- [ ] Chunk-relative GPU positions.
- [ ] Camera-relative rendering and origin rebasing.
- [ ] Precision validation at representative scales.
- [ ] Consider owned double-precision math only if existing tools are inadequate.

## 27. P3 — GPU Group Transforms

Move persistent collections without updating every instance:

```text
view_projection * group_transform * instance_transform * vertex
```

Potential uses include coordinate frames, spatial localization, moving
datasets, assemblies, and chunk-relative rendering.

Current assembly parent transforms are CPU-composed and transient.
They do not complete this GPU-level task.

## 28. P3 — Primitive Catalog and Helper Hardening

- [ ] Add a deliberately low-detail sphere when needed.
- [ ] Consider configurable icosphere subdivision.
- [ ] Retain the UV sphere.
- [ ] Harden internal assembly helpers before public reuse.
- [ ] Keep axis/frustum constructors private until allocation, handles,
      transforms, and failure behavior are robust.

Generic public assemblies are already implemented. Do not add clone, nesting,
or component-editing APIs without demonstrated need.

## 29. P3 — Edge Controls

- [ ] Per-mesh or per-submission visibility.
- [ ] Configurable edge color.
- [ ] Avoid unnecessary edge-buffer allocation.
- [ ] Preserve the global toggle as the simple path.

## 30. Production Packaging and Distribution

Do not block local experimental installation on production distribution work.

Before redistribution:

- [ ] Project licensing and third-party notices.
- [ ] Exact WebGPU-native/cglm provenance.
- [ ] Supported architectures, libc, distributions, and runtime requirements.
- [ ] Backend package components or explicit prefix restrictions.
- [ ] Shared-library support only with an ABI policy.
- [ ] Symbol visibility, exports, `VERSION`, and `SOVERSION`.
- [ ] Dependency acquisition policy.
- [ ] Multi-config and relocation validation.
- [ ] Source package generation.
- [ ] Decide Zig build support status.
- [ ] Installation/deployment documentation.

## 31. Other Deferred Features

- Textures/materials.
- HDR.
- Selection/picking.
- GUI integration.
- Stable ABI.
- Windows, macOS, and WASM.
- Additional backends.
- Configurable diagnostic callbacks.
- Brown-Conrady distortion.
- Complete cimpl replacement.
- Owned double-precision math.
- Nested assemblies/scene graph.
- Assembly editing, cloning, and destruction unless use justifies them.

# Non-Goals for Initial Experimental Use

The first package does not require:

- Stable ABI or production binary compatibility.
- Retained scene graph or nested assemblies.
- Individual assembly destruction/component editing.
- Persistent GPU instance resources or GPU group transforms.
- Automatic culling, LOD, or spatial partitioning.
- Cross-platform support or shared libraries.
- Multiple simultaneous windows or general SDL-loop embedding.
- CI binary distribution or production packaging guarantees.

# Immediate Milestone

The immediate milestone remains **Experimental Package Readiness**:

1. Complete surface acceptance and characterize the rapid-resize blocker.
2. Resolve vertex-color behavior.
3. Resolve normal transforms and builtin normals.
4. Add minimal install/export/package configuration.
5. Convert the starter to installed-package consumption.
6. Repeat representative external use through that package.

Native single-window close support is implemented and has recorded manual
acceptance. Preserve it through regression coverage rather than treating it
as unfinished implementation work.

The public assembly feature is implemented and manually exercised.
Its remaining work is focused validation and consumer documentation—not a
larger assembly architecture.

Keep persistence, paging, culling, and further API expansion tied to the
dependencies and evidence described above.
