# Core Knowledge

## Pitfalls
- Using logging before core initialization
- Blocking file IO on time-critical threads

## Performance
- Avoid heavy formatting in hot paths
- Prefer async logging where feasible

## Debugging
- Enable verbose logs for early init issues
- Use profiler markers to scope spikes

## Platform Notes
- Normalize path separators when crossing OS boundaries
