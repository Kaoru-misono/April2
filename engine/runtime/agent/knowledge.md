# Runtime Knowledge

## Pitfalls
- Doing heavy work inside the frame loop
- Breaking the tick order

## Performance
- Avoid per-frame allocations
- Keep frame timing metrics enabled

## Debugging
- Log frame time spikes with markers
- Use a fixed timestep when reproducing bugs

## Platform Notes
- Ensure the OS event pump is called on the main thread
